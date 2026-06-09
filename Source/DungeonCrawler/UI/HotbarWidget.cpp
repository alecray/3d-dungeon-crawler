#include "HotbarWidget.h"
#include "HotbarSlotWidget.h"
#include "HotbarComponent.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

bool UHotbarWidget::Initialize()
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

	Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	if (UCanvasPanelSlot* RS = Root->AddChildToCanvas(Row))
	{
		RS->SetAnchors(FAnchors(0.5f, 1.f));        // bottom-center
		RS->SetAlignment(FVector2D(0.5f, 1.f));
		RS->SetPosition(FVector2D(0.f, -16.f));
		RS->SetAutoSize(true);
	}

	return true;
}

void UHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Hotbar = Player->GetHotbarComponent();
	}
	BuildSlots();

	if (Hotbar.IsValid())
	{
		Hotbar->OnHotbarChanged.AddUObject(this, &UHotbarWidget::RefreshAll);
		RefreshAll();
	}
}

void UHotbarWidget::NativeDestruct()
{
	if (Hotbar.IsValid())
	{
		Hotbar->OnHotbarChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UHotbarWidget::BuildSlots()
{
	if (!Row || SlotWidgets.Num() > 0 || !Hotbar.IsValid())
	{
		return;
	}
	for (int32 i = 0; i < UHotbarComponent::NumSlots; ++i)
	{
		UHotbarSlotWidget* SlotW = CreateWidget<UHotbarSlotWidget>(this, UHotbarSlotWidget::StaticClass());
		if (!SlotW)
		{
			continue;
		}
		SlotW->Setup(Hotbar.Get(), i);
		if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(SlotW))
		{
			HS->SetPadding(FMargin(3.f, 0.f));
		}
		SlotWidgets.Add(SlotW);
	}
}

void UHotbarWidget::RefreshAll(UHotbarComponent* /*Changed*/)
{
	for (UHotbarSlotWidget* SlotW : SlotWidgets)
	{
		if (SlotW)
		{
			SlotW->Refresh();
		}
	}
}
