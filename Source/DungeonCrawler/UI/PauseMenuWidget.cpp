#include "PauseMenuWidget.h"
#include "FirstPersonCharacter.h"
#include "DungeonGameInstance.h"
#include "DungeonPlayerController.h"
#include "StatsScreenWidget.h"

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
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

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
		PS->SetAutoSize(true); // height grows to fit content (width is fixed by the inner sizer) — never squished
	}

	// Fixed width, auto height: the panel wraps however tall its content is, so adding the controls list
	// can't compress or clip the rest of the menu.
	USizeBox* PanelSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizer"));
	PanelSizer->SetWidthOverride(620.f);
	Panel->SetContent(PanelSizer);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	PanelSizer->AddChild(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Paused")));
	Title->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo Font = Title->GetFont(); true) { Font.Size = 28; Title->SetFont(Font); }
	if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	}

	auto AddButton = [&](const TCHAR* Label, void (UPauseMenuWidget::*Handler)())
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();

		// Give the button a consistent height so it doesn't render as a thin sliver.
		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetHeightOverride(38.f);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSize->AddChild(T)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSize);

		if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Btn)))
		{
			BS->SetPadding(FMargin(0.f, 6.f));
		}
		return Btn;
	};

	AddButton(TEXT("Resume"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	AddButton(TEXT("Stats"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnStatsClicked);
	AddButton(TEXT("Settings"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
	AddButton(TEXT("Dev Menu"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevMenuClicked);
	AddButton(TEXT("Quit"), nullptr)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);

	// Settings sub-panel (hidden until "Settings" is pressed).
	SettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanel"));
	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* SS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(SettingsPanel)))
	{
		SS->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// Builds a "label  [slider]  value" row. NOTE: the slider's OnValueChanged is bound by the CALLER,
	// not here — AddDynamic stringizes its literal function-name token, so a Handler passed through a
	// variable registers as the name "Handler" and asserts at runtime.
	auto AddSliderRow = [&](const TCHAR* Label, TObjectPtr<USlider>& OutSlider, TObjectPtr<UTextBlock>& OutValue)
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

		OutValue = WidgetTree->ConstructWidget<UTextBlock>();
		OutValue->SetText(FText::FromString(TEXT("--")));
		if (UHorizontalBoxSlot* VS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(OutValue)))
		{
			VS->SetVerticalAlignment(VAlign_Center);
			VS->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
	};

	AddSliderRow(TEXT("Mouse Sensitivity"), SensSlider, SensValueText);
	AddSliderRow(TEXT("Master Volume"), VolumeSlider, VolumeValueText);
	if (SensSlider)   { SensSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::OnSensitivityChanged); }
	if (VolumeSlider) { VolumeSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::OnVolumeChanged); }

	// --- Controls reference (every player binding) ---
	UTextBlock* ControlsHeader = WidgetTree->ConstructWidget<UTextBlock>();
	ControlsHeader->SetText(FText::FromString(TEXT("Controls")));
	if (UVerticalBoxSlot* CHS = Cast<UVerticalBoxSlot>(SettingsPanel->AddChildToVerticalBox(ControlsHeader)))
	{
		CHS->SetPadding(FMargin(0.f, 18.f, 0.f, 6.f));
	}

	USizeBox* ControlsSize = WidgetTree->ConstructWidget<USizeBox>();
	ControlsSize->SetHeightOverride(230.f);
	SettingsPanel->AddChildToVerticalBox(ControlsSize);

	UScrollBox* ControlsScroll = WidgetTree->ConstructWidget<UScrollBox>();
	ControlsSize->AddChild(ControlsScroll);

	auto AddControlRow = [&](const TCHAR* Keys, const TCHAR* Action)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		ControlsScroll->AddChild(Row);

		UTextBlock* K = WidgetTree->ConstructWidget<UTextBlock>();
		K->SetText(FText::FromString(Keys));
		if (UHorizontalBoxSlot* KS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(K)))
		{
			KS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* A = WidgetTree->ConstructWidget<UTextBlock>();
		A->SetText(FText::FromString(Action));
		A->SetJustification(ETextJustify::Right);
		if (UHorizontalBoxSlot* AS = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(A)))
		{
			AS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			AS->SetPadding(FMargin(8.f, 2.f, 0.f, 2.f));
		}
	};

	AddControlRow(TEXT("W  A  S  D"),       TEXT("Move"));
	AddControlRow(TEXT("Mouse"),            TEXT("Look"));
	AddControlRow(TEXT("Space"),            TEXT("Jump"));
	AddControlRow(TEXT("Left Shift"),       TEXT("Dash / dodge (costs stamina)"));
	AddControlRow(TEXT("Left Mouse"),       TEXT("Attack"));
	AddControlRow(TEXT("Q"),                TEXT("Use Ability"));
	AddControlRow(TEXT("1 - 8"),            TEXT("Hotbar slots (use / equip)"));
	AddControlRow(TEXT("E"),                TEXT("Interact (open / pick up / portal / shop)"));
	AddControlRow(TEXT("I"),                TEXT("Inventory"));
	AddControlRow(TEXT("C"),                TEXT("Collection Log"));
	AddControlRow(TEXT("K"),                TEXT("Skill Tree"));
	AddControlRow(TEXT("Esc"),              TEXT("Pause / Settings"));

	// --- Dev panel (hidden until "Dev Menu" is pressed) ---
	DevPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DevPanel"));
	DevPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* DPS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(DevPanel)))
	{
		DPS->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	UTextBlock* DevHeader = WidgetTree->ConstructWidget<UTextBlock>();
	DevHeader->SetText(FText::FromString(TEXT("Dev Menu")));
	DevHeader->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* DHS = Cast<UVerticalBoxSlot>(DevPanel->AddChildToVerticalBox(DevHeader)))
	{
		DHS->SetHorizontalAlignment(HAlign_Center);
		DHS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// Creates a dev button in DevPanel; caller binds OnClicked by literal name (AddDynamic macro limitation).
	auto MakeDevButton = [&](const TCHAR* Label, UTextBlock*& OutText) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetHeightOverride(34.f);
		OutText = WidgetTree->ConstructWidget<UTextBlock>();
		OutText->SetText(FText::FromString(Label));
		OutText->SetJustification(ETextJustify::Center);
		if (USizeBoxSlot* TS = Cast<USizeBoxSlot>(BtnSize->AddChild(OutText)))
		{
			TS->SetHorizontalAlignment(HAlign_Fill);
			TS->SetVerticalAlignment(VAlign_Center);
		}
		Btn->AddChild(BtnSize);
		if (UVerticalBoxSlot* BS = Cast<UVerticalBoxSlot>(DevPanel->AddChildToVerticalBox(Btn)))
		{
			BS->SetPadding(FMargin(0.f, 4.f));
		}
		return Btn;
	};

	UTextBlock* NoClipText = nullptr;
	UTextBlock* GodModeText = nullptr;
	UTextBlock* Throwaway = nullptr;
	UTextBlock* DebugOverlayText = nullptr;
	MakeDevButton(TEXT("No Clip: OFF"),  NoClipText )->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevNoClip);
	MakeDevButton(TEXT("God Mode: OFF"), GodModeText)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevGodMode);
	MakeDevButton(TEXT("Reveal Map"),       Throwaway)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevReveal);
	MakeDevButton(TEXT("Teleport to Boss"), Throwaway)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevBoss);
	MakeDevButton(TEXT("Kill Player"),      Throwaway)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevKill);
	MakeDevButton(TEXT("Teleport Home"),    Throwaway)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevHome);
	MakeDevButton(TEXT("Give 1,000,000 Gold"), Throwaway)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevGold);
	MakeDevButton(TEXT("Debug Overlays: OFF"), DebugOverlayText)->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnDevDebugOverlays);
	NoClipLabel = NoClipText;
	GodModeLabel = GodModeText;
	DebugOverlayLabel = DebugOverlayText;

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

void UPauseMenuWidget::OnStatsClicked()
{
	if (UStatsScreenWidget* Stats = CreateWidget<UStatsScreenWidget>(GetOwningPlayer(), UStatsScreenWidget::StaticClass()))
	{
		Stats->AddToViewport(100); // overlays the pause menu; its Back button removes it
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

void UPauseMenuWidget::OnDevMenuClicked()
{
	if (DevPanel)
	{
		const bool bVisible = DevPanel->GetVisibility() != ESlateVisibility::Collapsed;
		DevPanel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UPauseMenuWidget::OnDevNoClip()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		const bool bOn = PC->DevToggleNoClip();
		if (NoClipLabel) { NoClipLabel->SetText(FText::FromString(bOn ? TEXT("No Clip: ON") : TEXT("No Clip: OFF"))); }
	}
}

void UPauseMenuWidget::OnDevGodMode()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		const bool bOn = PC->DevToggleGodMode();
		if (GodModeLabel) { GodModeLabel->SetText(FText::FromString(bOn ? TEXT("God Mode: ON") : TEXT("God Mode: OFF"))); }
	}
}

void UPauseMenuWidget::OnDevReveal()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->DevRevealMap();
	}
}

void UPauseMenuWidget::OnDevKill()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu(); // unpause so the death flow can run
		PC->DevKill();
	}
}

void UPauseMenuWidget::OnDevHome()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu(); // unpause so the fade/travel timer runs
		PC->DevTeleportHome();
	}
}

void UPauseMenuWidget::OnDevGold()
{
	if (AFirstPersonCharacter* P = GetPlayer()) { P->AddGold(1000000); }
}

void UPauseMenuWidget::OnDevDebugOverlays()
{
	// Toggle the dc.DebugOverlays CVar that the monsters read to draw their melee hit zones.
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("dc.DebugOverlays")))
	{
		const bool bOn = (CVar->GetInt() == 0);
		CVar->Set(bOn ? 1 : 0);
		if (DebugOverlayLabel)
		{
			DebugOverlayLabel->SetText(FText::FromString(bOn ? TEXT("Debug Overlays: ON") : TEXT("Debug Overlays: OFF")));
		}
	}
}

void UPauseMenuWidget::OnDevBoss()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->DevTeleportToBoss();
		PC->ClosePauseMenu();
	}
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
