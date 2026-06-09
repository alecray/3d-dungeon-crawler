#include "MonsterSpawnWidget.h"
#include "MonsterSpawnPedestal.h"
#include "MonsterTypes.h"
#include "BossMonster.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// ---- Boss registry --------------------------------------------------------------------------------
// Bosses aren't part of MonsterDatabase (they're AMonsterCharacter subclasses with their own setup), so
// the Bosses tab is driven by this small code-defined list. Add a row here when a new boss is created.
namespace
{
	struct FBossEntry
	{
		const TCHAR* DisplayName;
		TSubclassOf<AMonsterCharacter> Class;
	};

	static TArray<FBossEntry> BossEntries()
	{
		return {
			{ TEXT("Hermit Crab"), ABossMonster::StaticClass() },
		};
	}

	// Tidy a type id ("HermitCrab") for display; monster ids are already friendly enough as-is.
	static FString PrettyName(FName Id)
	{
		return Id.ToString();
	}
}

void USpawnClickProxy::OnClicked()
{
	if (!Owner.IsValid())
	{
		return;
	}
	switch (Action)
	{
	case EAction::Pick:   Owner->Pick(TypeId, bBoss, BossClass); break;
	case EAction::Filter: Owner->SetFilter(FilterTab); break;
	case EAction::Clear:  Owner->ClearDummies(); break;
	}
}

bool UMonsterSpawnWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.06f, 0.94f));
	Panel->SetPadding(FMargin(18.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetSize(FVector2D(720.f, 560.f));
	}

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Outer"));
	Panel->SetContent(Outer);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetText(FText::FromString(TEXT("Spawn Monster")));
	Outer->AddChildToVerticalBox(TitleText);

	// Filter tab row: Enemies | Bosses.
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tabs"));
	if (UVerticalBoxSlot* TS = Cast<UVerticalBoxSlot>(Outer->AddChildToVerticalBox(Tabs)))
	{
		TS->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));
	}

	auto MakeTab = [&](const TCHAR* Label, int32 Tab) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		Btn->AddChild(T);
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(Tabs->AddChildToHorizontalBox(Btn)))
		{
			HS->SetPadding(FMargin(6.f, 0.f));
		}
		USpawnClickProxy* Proxy = NewObject<USpawnClickProxy>(this);
		Proxy->Action = USpawnClickProxy::EAction::Filter;
		Proxy->FilterTab = Tab;
		Proxy->Owner = this;
		Btn->OnClicked.AddDynamic(Proxy, &USpawnClickProxy::OnClicked);
		Proxies.Add(Proxy);
		return Btn;
	};
	EnemiesTab = MakeTab(TEXT("Enemies"), 0);
	BossesTab  = MakeTab(TEXT("Bosses"), 1);

	// The grid of spawnable entries.
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
	if (UVerticalBoxSlot* GS = Cast<UVerticalBoxSlot>(Outer->AddChildToVerticalBox(Grid)))
	{
		GS->SetPadding(FMargin(0.f, 4.f));
	}

	// Clear-dummies button along the bottom.
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(TEXT("Clear Spawned")));
		T->SetJustification(ETextJustify::Center);
		Btn->AddChild(T);
		if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(Outer->AddChildToVerticalBox(Btn)))
		{
			BS->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
		}
		USpawnClickProxy* Proxy = NewObject<USpawnClickProxy>(this);
		Proxy->Action = USpawnClickProxy::EAction::Clear;
		Proxy->Owner = this;
		Btn->OnClicked.AddDynamic(Proxy, &USpawnClickProxy::OnClicked);
		Proxies.Add(Proxy);
	}

	Refresh();
	return true;
}

void UMonsterSpawnWidget::SetFilter(int32 Tab)
{
	FilterTab = FMath::Clamp(Tab, 0, 1);
	Refresh();
}

void UMonsterSpawnWidget::Pick(FName TypeId, bool bBoss, TSubclassOf<AMonsterCharacter> BossClass)
{
	AMonsterSpawnPedestal* P = Pedestal.Get();
	if (!P)
	{
		return;
	}
	if (bBoss) { P->SpawnBossClass(BossClass); }
	else       { P->SpawnMonsterType(TypeId); }
}

void UMonsterSpawnWidget::ClearDummies()
{
	if (AMonsterSpawnPedestal* P = Pedestal.Get())
	{
		P->ClearSpawned();
	}
}

void UMonsterSpawnWidget::Refresh()
{
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(FilterTab == 1 ? TEXT("Spawn Boss") : TEXT("Spawn Monster")));
	}
	if (!Grid)
	{
		return;
	}

	Grid->ClearChildren();
	// Drop the tab proxies are kept; rebuild only the entry-button proxies. Simplest: keep all proxies
	// (filter/clear ones are reused across refreshes) and just append fresh Pick proxies each rebuild.

	const int32 Columns = 3;
	int32 Index = 0;
	auto AddEntry = [&](const FString& Label, FName TypeId, bool bBoss, TSubclassOf<AMonsterCharacter> BossClass)
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		Btn->AddChild(T);

		if (UUniformGridSlot* Slot = Grid->AddChildToUniformGrid(Btn, Index / Columns, Index % Columns))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		USpawnClickProxy* Proxy = NewObject<USpawnClickProxy>(this);
		Proxy->Action = USpawnClickProxy::EAction::Pick;
		Proxy->TypeId = TypeId;
		Proxy->bBoss = bBoss;
		Proxy->BossClass = BossClass;
		Proxy->Owner = this;
		Btn->OnClicked.AddDynamic(Proxy, &USpawnClickProxy::OnClicked);
		Proxies.Add(Proxy);
		++Index;
	};

	if (FilterTab == 1)
	{
		for (const FBossEntry& B : BossEntries())
		{
			AddEntry(B.DisplayName, NAME_None, /*bBoss*/ true, B.Class);
		}
	}
	else
	{
		for (const FMonsterDef& Def : MonsterDatabase::All())
		{
			AddEntry(PrettyName(Def.Id), Def.Id, /*bBoss*/ false, nullptr);
		}
	}
}
