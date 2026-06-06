#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UVerticalBox;
class USlider;
class UTextBlock;

/**
 * The start screen. A code-built (graybox) full-screen opaque menu shown at launch: a title and
 * Start / Quit buttons + a Settings panel (mouse sensitivity / master volume, persisted to the profile).
 * Start hands off to the player controller to begin play (fade in from black); Quit exits.
 */
UCLASS()
class DUNGEONCRAWLER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	UFUNCTION()
	void OnStartClicked(); // returning save: continue

	UFUNCTION()
	void OnWarriorClicked();
	UFUNCTION()
	void OnRangerClicked();
	UFUNCTION()
	void OnMageClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnStatsClicked();
	UFUNCTION()
	void OnSettingsClicked();
	UFUNCTION()
	void OnSettingsBackClicked();
	UFUNCTION()
	void OnSensitivityChanged(float Value);
	UFUNCTION()
	void OnVolumeChanged(float Value);

private:
	UPROPERTY() TObjectPtr<UVerticalBox> MenuButtons;  // class/continue/settings/quit (hidden while Settings is open)
	UPROPERTY() TObjectPtr<UVerticalBox> SettingsPanel; // sliders + back (hidden until Settings is pressed)
	UPROPERTY() TObjectPtr<USlider> SensSlider;
	UPROPERTY() TObjectPtr<USlider> VolumeSlider;
	UPROPERTY() TObjectPtr<UTextBlock> SensValueText;
	UPROPERTY() TObjectPtr<UTextBlock> VolumeValueText;

	static constexpr float MinSens = 0.1f;
	static constexpr float MaxSens = 4.0f;
};
