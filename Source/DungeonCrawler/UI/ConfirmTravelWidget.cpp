#include "ConfirmTravelWidget.h"
#include "DungeonPlayerController.h"
#include "FirstPersonCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"

bool UConfirmTravelWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree || Cast<UCanvasPanel>(WidgetTree->RootWidget)) { return true; }

	// Full-screen dim.
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.70f));
	if (UCanvasPanelSlot* DS = Root->AddChildToCanvas(Dim))
	{
		DS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DS->SetOffsets(FMargin(0.f));
	}

	// Centred panel.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.97f));
	Panel->SetPadding(FMargin(32.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetAutoSize(true);
	}

	USizeBox* Sizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Sizer"));
	Sizer->SetWidthOverride(420.f);
	Panel->SetContent(Sizer);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Sizer->AddChild(Box);

	// Title.
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Leave Dungeon?")));
	Title->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo F = Title->GetFont();
		F.Size = 24;
		Title->SetFont(F);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	}

	// Body message.
	UTextBlock* Body = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
	Body->SetText(FText::FromString(TEXT("Abandon your current run and return to town?")));
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	{
		FSlateFontInfo F = Body->GetFont();
		F.Size = 14;
		Body->SetFont(F);
	}
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Body)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	}

	// Leave / Stay button row.
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(ActionRow)))
	{
		VS->SetHorizontalAlignment(HAlign_Center);
	}

	auto MakeBtn = [&](const TCHAR* Label) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		USizeBox* BtnSizer = WidgetTree->ConstructWidget<USizeBox>();
		BtnSizer->SetWidthOverride(160.f);
		BtnSizer->SetHeightOverride(42.f);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSizer->AddChild(T)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSizer);
		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(ActionRow->AddChildToHorizontalBox(Btn)))
		{
			HS->SetPadding(FMargin(10.f, 0.f));
			HS->SetVerticalAlignment(VAlign_Center);
		}
		return Btn;
	};

	UButton* LeaveBtn = MakeBtn(TEXT("Leave"));
	UButton* StayBtn  = MakeBtn(TEXT("Stay"));

	// Tint the Leave button red to signal the destructive action.
	if (LeaveBtn) { LeaveBtn->SetColorAndOpacity(FLinearColor(1.f, 0.35f, 0.35f, 1.f)); }

	if (LeaveBtn) { LeaveBtn->OnClicked.AddDynamic(this, &UConfirmTravelWidget::OnLeaveClicked); }
	if (StayBtn)  { StayBtn->OnClicked.AddDynamic(this, &UConfirmTravelWidget::OnStayClicked);  }

	return true;
}

void UConfirmTravelWidget::Init(FName InTargetMap)
{
	TargetMap = InTargetMap;
}

void UConfirmTravelWidget::OnLeaveClicked()
{
	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Player->SaveNow();
	}
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->CloseConfirmReturnMenu();
		PC->FadeToBlackAndTravel(TargetMap.IsNone() ? FName(TEXT("L_Town")) : TargetMap);
	}
}

void UConfirmTravelWidget::OnStayClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->CloseConfirmReturnMenu();
	}
}
