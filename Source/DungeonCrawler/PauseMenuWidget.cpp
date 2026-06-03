#include "PauseMenuWidget.h"
#include "FirstPersonCharacter.h"
#include "DungeonGameInstance.h"
#include "DungeonPlayerController.h"

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
#include "Components/Slider.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Engine/World.h"

bool UPauseMenuWidget::Initialize()
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

	// Full-screen dim behind the menu.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	if (UCanvasPanelSlot* DS = Root->AddChildToCanvas(Dim))
	{
		DS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DS->SetOffsets(FMargin(0.f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.07f, 0.97f));
	Panel->SetPadding(FMargin(22.f));
	if (UCanvasPanelSlot* PS = Root->AddChildToCanvas(Panel))
	{
		PS->SetAnchors(FAnchors(0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D::ZeroVector);
		PS->SetSize(FVector2D(420.f, 460.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Paused")));
	Title->SetJustification(ETextJustify::Center);
	Box->AddChildToVerticalBox(Title);

	auto AddButton = [&](const TCHAR* Label, void (UPauseMenuWidget::*Handler)())
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		Btn->AddChild(T);
		if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Btn)))
		{
			BS->SetPadding(FMargin(0.f, 8.f));
		}
		return Btn;
	};

	AddButton(TEXT("Resume"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	AddButton(TEXT("Settings"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
	AddButton(TEXT("Quit"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);

	// Settings sub-panel (hidden until "Settings" is pressed).
	SettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanel"));
	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* SS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(SettingsPanel)))
	{
		SS->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	auto AddSliderRow = [&](const TCHAR* Label, TObjectPtr<USlider>& OutSlider, TObjectPtr<UTextBlock>& OutValue,
		void (UPauseMenuWidget::*Handler)(float))
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		SettingsPanel->AddChildToVerticalBox(Row);

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(FText::FromString(Label));
		if (UHorizontalBoxSlot* NS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(Name)))
		{
			NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NS->SetVerticalAlignment(VAlign_Center);
		}

		OutSlider = WidgetTree->ConstructWidget<USlider>();
		if (UHorizontalBoxSlot* SLS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(OutSlider)))
		{
			SLS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SLS->SetVerticalAlignment(VAlign_Center);
		}
		OutSlider->OnValueChanged.AddDynamic(this, Handler);

		OutValue = WidgetTree->ConstructWidget<UTextBlock>();
		OutValue->SetText(FText::FromString(TEXT("--")));
		if (UHorizontalBoxSlot* VS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(OutValue)))
		{
			VS->SetVerticalAlignment(VAlign_Center);
			VS->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
	};

	AddSliderRow(TEXT("Mouse Sensitivity"), SensSlider, SensValueText, &UPauseMenuWidget::OnSensitivityChanged);
	AddSliderRow(TEXT("Master Volume"), VolumeSlider, VolumeValueText, &UPauseMenuWidget::OnVolumeChanged);

	return true;
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize sliders from current settings.
	const float Sens = GetPlayer() ? GetPlayer()->GetLookSensitivity() : 1.f;
	const float Volume = GetGI() ? GetGI()->GetProfile().MasterVolume : 1.f;

	if (SensSlider)
	{
		SensSlider->SetValue(FMath::Clamp((Sens - MinSens) / (MaxSens - MinSens), 0.f, 1.f));
	}
	if (VolumeSlider)
	{
		VolumeSlider->SetValue(FMath::Clamp(Volume, 0.f, 1.f));
	}
	if (SensValueText) { SensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Sens))); }
	if (VolumeValueText) { VolumeValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Volume * 100.f)))); }
}

void UPauseMenuWidget::OnResumeClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
}

void UPauseMenuWidget::OnSettingsClicked()
{
	if (SettingsPanel)
	{
		const bool bVisible = SettingsPanel->GetVisibility() != ESlateVisibility::Collapsed;
		SettingsPanel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}

void UPauseMenuWidget::OnSensitivityChanged(float Value)
{
	const float Sens = MinSens + Value * (MaxSens - MinSens);
	if (AFirstPersonCharacter* Player = GetPlayer())
	{
		Player->SetLookSensitivity(Sens);
	}
	if (SensValueText) { SensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Sens))); }
	SaveSettings();
}

void UPauseMenuWidget::OnVolumeChanged(float Value)
{
	if (UWorld* W = GetWorld())
	{
		if (FAudioDevice* Audio = W->GetAudioDeviceRaw())
		{
			Audio->SetTransientPrimaryVolume(Value);
		}
	}
	if (VolumeValueText) { VolumeValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)))); }
	SaveSettings();
}

void UPauseMenuWidget::SaveSettings()
{
	if (UDungeonGameInstance* GI = GetGI())
	{
		FPlayerProfile& Profile = GI->GetProfile();
		Profile.bInitialized = true;
		if (GetPlayer()) { Profile.MouseSensitivity = GetPlayer()->GetLookSensitivity(); }
		if (VolumeSlider) { Profile.MasterVolume = VolumeSlider->GetValue(); }
		GI->SaveProfile();
	}
}

AFirstPersonCharacter* UPauseMenuWidget::GetPlayer() const
{
	return Cast<AFirstPersonCharacter>(GetOwningPlayerPawn());
}

UDungeonGameInstance* UPauseMenuWidget::GetGI() const
{
	return GetWorld() ? Cast<UDungeonGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
}
