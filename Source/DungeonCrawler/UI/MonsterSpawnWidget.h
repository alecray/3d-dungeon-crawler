#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "MonsterSpawnWidget.generated.h"

class UButton;
class UTextBlock;
class UUniformGridPanel;
class UHorizontalBox;
class AMonsterSpawnPedestal;
class AMonsterCharacter;
class UMonsterSpawnWidget;

/** Per-button click handler (UButton::OnClicked carries no sender). */
UCLASS()
class USpawnClickProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnClicked();

	enum class EAction : uint8 { Pick, Filter, Clear };
	EAction Action = EAction::Pick;

	FName TypeId;                                  // monster type to spawn (Pick, non-boss)
	bool bBoss = false;                            // Pick: spawn BossClass instead of TypeId
	TSubclassOf<AMonsterCharacter> BossClass;      // Pick: boss class to spawn
	int32 FilterTab = 0;                           // Filter: which tab to switch to (0=enemies, 1=bosses)

	UPROPERTY() TWeakObjectPtr<UMonsterSpawnWidget> Owner;
};

/**
 * Pure-C++ monster spawn menu (opened from the town spawn pedestal). A grid of every registered enemy and
 * boss, split by two filter tabs (Enemies / Bosses); clicking an entry spawns it as a passive test dummy.
 * A Clear button removes everything the pedestal has spawned.
 */
UCLASS()
class DUNGEONCRAWLER_API UMonsterSpawnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Set by the controller before the widget is shown. */
	void SetPedestal(AMonsterSpawnPedestal* InPedestal) { Pedestal = InPedestal; }

	void SetFilter(int32 Tab);
	void Pick(FName TypeId, bool bBoss, TSubclassOf<AMonsterCharacter> BossClass);
	void ClearDummies();
	void Refresh();

private:
	TWeakObjectPtr<AMonsterSpawnPedestal> Pedestal;
	int32 FilterTab = 0; // 0 = enemies, 1 = bosses

	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UButton> EnemiesTab;
	UPROPERTY() TObjectPtr<UButton> BossesTab;
	UPROPERTY() TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY() TArray<TObjectPtr<USpawnClickProxy>> Proxies; // kept alive for OnClicked
};
