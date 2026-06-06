#include "MainMenuWidget.h"
#include "DungeonPlayerController.h"
#include "DungeonGameInstance.h"
#include "CharacterClass.h"

#include "FirstPersonCharacter.h"
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
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Engine/World.h"

bool UMainMenuWidget::Initialize()
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

	// Full-screen opaque backdrop (so nothing behind the menu shows through).
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.f));
	if (UCanvasPanelSlot* BS = Root->AddChildToCanvas(Backdrop))
	{
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Backdrop->SetContent(Box);
	Box->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("DUNGEON CRAWLER")));
	Title->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo Font = Title->GetFont(); true)
	{
		Font.Size = 48;
		Title->SetFont(Font);
	}
	if (UVerticalBoxSlot* TS = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Title)))
	{
		TS->SetHorizontalAlignment(HAlign_Center);
		TS->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));
	}

	// Buttons live in their own box so Settings can hide them (the title stays).
	MenuButtons = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuButtons"));
	Box->AddChildToVerticalBox(MenuButtons);

	auto AddButton = [&](const TCHAR* Label, void (UMainMenuWidget::*Handler)())
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Label));
		T->SetJustification(ETextJustify::Center);
		if (FSlateFontInfo Font = T->GetFont(); true) { Font.Size = 22; T->SetFont(Font); }
		Btn->AddChild(T);
		if (UVerticalBoxSlot* BSlot = Cast<UVerticalBoxSlot>(MenuButtons->AddChildToVerticalBox(Btn)))
		{
			BSlot->SetHorizontalAlignment(HAlign_Center);
			BSlot->SetPadding(FMargin(0.f, 10.f));
		}
		return Btn;
	};

	// A fresh character picks a class; a returning save just continues.
	bool bFresh = true;
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		bFresh = !GI->HasProfile();
	}

	if (bFresh)
	{
		UTextBlock* Prompt = WidgetTree->ConstructWidget<UTextBlock>();
		Prompt->SetText(FText::FromString(TEXT("Choose your class")));
		Prompt->SetJustification(ETextJustify::Center);
		if (FSlateFontInfo F = Prompt->GetFont(); true) { F.Size = 18; Prompt->SetFont(F); }
		if (UVerticalBoxSlot* PS2 = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Prompt)))
		{
			PS2->SetHorizontalAlignment(HAlign_Center);
			PS2->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		}

		AddButton(TEXT("  Warrior  (sword / strength)  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnWarriorClicked);
		AddButton(TEXT("  Ranger  (crossbow / dexterity)  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnRangerClicked);
		AddButton(TEXT("  Mage  (wand / intelligence)  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnMageClicked);
	}
	else
	{
		AddButton(TEXT("  Continue  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStartClicked);
	}
	AddButton(TEXT("  Stats  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStatsClicked);
	AddButton(TEXT("  Settings  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
	AddButton(TEXT("  Quit  "), nullptr)->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);

	// --- Settings panel (hidden until "Settings" is pressed) ---
	SettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanel"));
	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	Box->AddChildToVerticalBox(SettingsPanel);

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

	// Seed slider positions + labels from the saved profile.
	float Sens = 1.f, Volume = 1.f;
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		Sens = GI->GetProfile().MouseSensitivity;
		Volume = GI->GetProfile().MasterVolume;
	}
	if (SensSlider)
	{
		SensSlider->SetValue(FMath::Clamp((Sens - MinSens) / (MaxSens - MinSens), 0.f, 1.f));
		SensSlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::OnSensitivityChanged);
	}
	if (SensValueText) { SensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Sens))); }
	if (VolumeSlider)
	{
		VolumeSlider->SetValue(FMath::Clamp(Volume, 0.f, 1.f));
		VolumeSlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::OnVolumeChanged);
	}
	if (VolumeValueText) { VolumeValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Volume * 100.f)))); }

	// Version stamp, bottom-right corner (matches the in-game HUD placement).
	UTextBlock* Version = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Version"));
	Version->SetText(FText::FromString(UDungeonGameInstance::GetGameVersion()));
	Version->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.65f, 0.85f)));
	if (FSlateFontInfo F = Version->GetFont(); true) { F.Size = 14; Version->SetFont(F); }
	if (UCanvasPanelSlot* VerS = Root->AddChildToCanvas(Version))
	{
		VerS->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		VerS->SetAlignment(FVector2D(1.f, 1.f));
		VerS->SetAutoSize(true);
		VerS->SetPosition(FVector2D(-14.f, -10.f));
	}

	UButton* Back = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>();
	BackText->SetText(FText::FromString(TEXT("  Back  ")));
	BackText->SetJustification(ETextJustify::Center);
	if (FSlateFontInfo F = BackText->GetFont(); true) { F.Size = 20; BackText->SetFont(F); }
	Back->AddChild(BackText);
	Back->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsBackClicked);
	if (UVerticalBoxSlot* BkS = Cast<UVerticalBoxSlot>(SettingsPanel->AddChildToVerticalBox(Back)))
	{
		BkS->SetHorizontalAlignment(HAlign_Center);
		BkS->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}

	return true;
}

void UMainMenuWidget::OnStartClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer()))
	{
		PC->StartGameFromMenu();
	}
}

void UMainMenuWidget::OnWarriorClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer())) { PC->StartGameAsClass(ECharacterClass::Warrior); }
}

void UMainMenuWidget::OnRangerClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer())) { PC->StartGameAsClass(ECharacterClass::Ranger); }
}

void UMainMenuWidget::OnMageClicked()
{
	if (ADungeonPlayerController* PC = Cast<ADungeonPlayerController>(GetOwningPlayer())) { PC->StartGameAsClass(ECharacterClass::Mage); }
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}

void UMainMenuWidget::OnStatsClicked()
{
	if (UStatsScreenWidget* Stats = CreateWidget<UStatsScreenWidget>(GetOwningPlayer(), UStatsScreenWidget::StaticClass()))
	{
		Stats->AddToViewport(100); // overlays the menu; its Back button removes it
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	if (MenuButtons) { MenuButtons->SetVisibility(ESlateVisibility::Collapsed); }
	if (SettingsPanel) { SettingsPanel->SetVisibility(ESlateVisibility::Visible); }
}

void UMainMenuWidget::OnSettingsBackClicked()
{
	if (SettingsPanel) { SettingsPanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (MenuButtons) { MenuButtons->SetVisibility(ESlateVisibility::Visible); }
}

void UMainMenuWidget::OnSensitivityChanged(float Value)
{
	const float Sens = MinSens + Value * (MaxSens - MinSens);
	if (SensValueText) { SensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Sens))); }
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		FPlayerProfile& Profile = GI->GetProfile();
		Profile.bInitialized = true;
		Profile.MouseSensitivity = Sens;
		GI->SaveProfile();
	}
	// Apply live if a pawn already exists (usually none on the title screen — it applies on spawn).
	if (AFirstPersonCharacter* Player = Cast<AFirstPersonCharacter>(GetOwningPlayerPawn()))
	{
		Player->SetLookSensitivity(Sens);
	}
}

void UMainMenuWidget::OnVolumeChanged(float Value)
{
	if (VolumeValueText) { VolumeValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)))); }
	if (UWorld* W = GetWorld())
	{
		if (FAudioDevice* Audio = W->GetAudioDeviceRaw())
		{
			Audio->SetTransientPrimaryVolume(Value);
		}
	}
	if (UDungeonGameInstance* GI = Cast<UDungeonGameInstance>(GetGameInstance()))
	{
		FPlayerProfile& Profile = GI->GetProfile();
		Profile.bInitialized = true;
		Profile.MasterVolume = Value;
		GI->SaveProfile();
	}
}
