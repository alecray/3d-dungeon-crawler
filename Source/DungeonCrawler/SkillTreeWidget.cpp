#include "SkillTreeWidget.h"
#include "SkillTypes.h"
#include "SkillTreeComponent.h"
#include "StatsComponent.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void USkillNodeClickProxy::OnClicked()
{
	if (Owner.IsValid())
	{
		Owner->AllocateNode(NodeId);
	}
}

bool USkillTreeWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		return true; // already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Centered dark panel.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.06f, 0.94f));
	Panel->SetPadding(FMargin(18.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetSize(FVector2D(660.f, 480.f));
	}

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Outer"));
	Panel->SetContent(Outer);

	// Header with the points counter.
	PointsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PointsText"));
	PointsText->SetText(FText::FromString(TEXT("Skill Tree")));
	Outer->AddChildToVerticalBox(PointsText);

	// Three branch columns side by side.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Columns"));
	if (UVerticalBoxSlot* CS = Cast<UVerticalBoxSlot>(Outer->AddChildToVerticalBox(Columns)))
	{
		CS->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	const ESkillBranch Branches[] = { ESkillBranch::Melee, ESkillBranch::Ranged, ESkillBranch::Mage };
	for (ESkillBranch Branch : Branches)
	{
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(Columns->AddChildToHorizontalBox(Col)))
		{
			HS->SetPadding(FMargin(8.f, 0.f));
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
		Heading->SetText(FText::FromString(SkillDatabase::BranchName(Branch)));
		Heading->SetJustification(ETextJustify::Center);
		Col->AddChildToVerticalBox(Heading);

		// Nodes in this branch, ordered by tier.
		TArray<const FSkillNode*> BranchNodes;
		for (const FSkillNode& N : SkillDatabase::All())
		{
			if (N.Branch == Branch) { BranchNodes.Add(&N); }
		}
		BranchNodes.Sort([](const FSkillNode& A, const FSkillNode& B) { return A.Tier < B.Tier; });

		for (const FSkillNode* Node : BranchNodes)
		{
			UButton* Btn = WidgetTree->ConstructWidget<UButton>();
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetJustification(ETextJustify::Center);
			Btn->AddChild(Label);

			if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(Col->AddChildToVerticalBox(Btn)))
			{
				BS->SetPadding(FMargin(0.f, 6.f));
			}

			USkillNodeClickProxy* Proxy = NewObject<USkillNodeClickProxy>(this);
			Proxy->NodeId = Node->Id;
			Proxy->Owner = this;
			Btn->OnClicked.AddDynamic(Proxy, &USkillNodeClickProxy::OnClicked);

			NodeIds.Add(Node->Id);
			NodeButtons.Add(Btn);
			NodeLabels.Add(Label);
			Proxies.Add(Proxy);
		}
	}

	return true;
}

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (USkillTreeComponent* Skills = GetSkills())
	{
		SkillsChangedHandle = Skills->OnSkillsChanged.AddUObject(this, &USkillTreeWidget::HandleSkillsChanged);
	}
	Refresh();
}

void USkillTreeWidget::NativeDestruct()
{
	if (USkillTreeComponent* Skills = GetSkills())
	{
		Skills->OnSkillsChanged.Remove(SkillsChangedHandle);
	}
	Super::NativeDestruct();
}

void USkillTreeWidget::HandleSkillsChanged()
{
	Refresh();
}

void USkillTreeWidget::AllocateNode(FName NodeId)
{
	if (USkillTreeComponent* Skills = GetSkills())
	{
		Skills->Allocate(NodeId); // fires OnSkillsChanged -> Refresh()
	}
}

void USkillTreeWidget::Refresh()
{
	USkillTreeComponent* Skills = GetSkills();
	const UStatsComponent* Stats = GetStats();
	const int32 Points = Stats ? Stats->GetSkillPoints() : 0;

	if (PointsText)
	{
		PointsText->SetText(FText::FromString(
			FString::Printf(TEXT("Skill Tree      Points: %d"), Points)));
	}

	for (int32 i = 0; i < NodeIds.Num(); ++i)
	{
		if (!SkillDatabase::Contains(NodeIds[i]))
		{
			continue;
		}
		const FSkillNode& Node = SkillDatabase::Get(NodeIds[i]);
		const int32 Rank = Skills ? Skills->GetRank(NodeIds[i]) : 0;
		const bool bMaxed = Rank >= Node.MaxRank;
		const bool bCanAllocate = Skills && Skills->CanAllocate(NodeIds[i]);

		if (UTextBlock* Label = NodeLabels[i])
		{
			Label->SetText(FText::FromString(FString::Printf(
				TEXT("%s  %d/%d\n%s"), *Node.DisplayName, Rank, Node.MaxRank, *Node.Description)));

			// Allocated = green, available = white, locked/maxed = gray.
			const FLinearColor TextColor = bMaxed ? FLinearColor(0.55f, 0.9f, 0.55f)
				: bCanAllocate ? FLinearColor::White
				: FLinearColor(0.5f, 0.5f, 0.55f);
			Label->SetColorAndOpacity(FSlateColor(TextColor));
		}
		if (UButton* Btn = NodeButtons[i])
		{
			Btn->SetIsEnabled(bCanAllocate);
			const FLinearColor BtnColor = (Rank > 0)
				? FLinearColor(0.12f, 0.28f, 0.14f, 1.f)  // allocated: green tint
				: FLinearColor(0.12f, 0.12f, 0.16f, 1.f); // unallocated: dark
			Btn->SetBackgroundColor(BtnColor);
		}
	}
}

USkillTreeComponent* USkillTreeWidget::GetSkills() const
{
	if (const AFirstPersonCharacter* FPChar = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		return FPChar->GetSkillTreeComponent();
	}
	return nullptr;
}

UStatsComponent* USkillTreeWidget::GetStats() const
{
	if (const AFirstPersonCharacter* FPChar = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		return FPChar->GetStatsComponent();
	}
	return nullptr;
}
