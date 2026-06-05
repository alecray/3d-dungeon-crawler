#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UVerticalBox;
class AFirstPersonCharacter;
class UDungeonGameInstance;

/**
 * Pure-C++ pause menu (toggled with Esc): Resume / Settings / Quit, plus a settings panel with mouse
 * sensitivity and master volume sliders. Changes apply live and persist in the player profile.
 */
UCLASS()
class DUNGEONCRAWLER_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSensitivityChanged(float Value);
	UFUNCTION() void OnVolumeChanged(float Value);

	// Dev menu actions.
	UFUNCTION() void OnDevMenuClicked();
	UFUNCTION() void OnDevNoClip();
	UFUNCTION() void OnDevGodMode();
	UFUNCTION() void OnDevReveal();
	UFUNCTION() void OnDevKill();
	UFUNCTION() void OnDevHome();
	UFUNCTION() void OnDevBoss();
	UFUNCTION() void OnDevGold();
	UFUNCTION() void OnDevDebugOverlays();

	AFirstPersonCharacter* GetPlayer() const;
	UDungeonGameInstance* GetGI() const;
	void SaveSettings();

	// Sensitivity slider (0..1) maps to this range.
	static constexpr float MinSens = 0.1f;
	static constexpr float MaxSens = 4.0f;

	UPROPERTY() TObjectPtr<USlider> SensSlider;
	UPROPERTY() TObjectPtr<USlider> VolumeSlider;
	UPROPERTY() TObjectPtr<UTextBlock> SensValueText;
	UPROPERTY() TObjectPtr<UTextBlock> VolumeValueText;
	UPROPERTY() TObjectPtr<UVerticalBox> SettingsPanel;

	UPROPERTY() TObjectPtr<UVerticalBox> DevPanel;
	UPROPERTY() TObjectPtr<UTextBlock> NoClipLabel;
	UPROPERTY() TObjectPtr<UTextBlock> GodModeLabel;
	UPROPERTY() TObjectPtr<UTextBlock> DebugOverlayLabel;
};
