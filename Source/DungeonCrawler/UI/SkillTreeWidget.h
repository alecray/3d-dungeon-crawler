#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTreeWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class USkillTreeComponent;
class UStatsComponent;
class USkillTreeWidget;

/**
 * One-per-node click handler. UButton::OnClicked is a dynamic delegate with no sender argument, so we
 * bind a small proxy object that remembers which node it belongs to and allocates it on click.
 */
UCLASS()
class USkillNodeClickProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnClicked();

	FName NodeId;
	UPROPERTY() TWeakObjectPtr<USkillTreeWidget> Owner;
};

/**
 * Pure-C++ skill-tree screen (toggled with K): three branch columns (Melee/Ranged/Mage), each a
 * stack of node buttons. Clicking an available node spends a skill point and allocates a rank.
 * Reads/writes the owning player's USkillTreeComponent and refreshes on changes.
 */
UCLASS()
class DUNGEONCRAWLER_API USkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Allocates a node (called by a node's click proxy), then refreshes. */
	void AllocateNode(FName NodeId);

	/** Updates every node's label/color/enabled state and the points header. */
	void Refresh();

private:
	USkillTreeComponent* GetSkills() const;
	UStatsComponent* GetStats() const;
	void HandleSkillsChanged();

	UPROPERTY() TObjectPtr<UTextBlock> PointsText;

	// Parallel arrays, one entry per node (same order as SkillDatabase::All()).
	UPROPERTY() TArray<FName> NodeIds;
	UPROPERTY() TArray<TObjectPtr<UButton>> NodeButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> NodeLabels;
	UPROPERTY() TArray<TObjectPtr<USkillNodeClickProxy>> Proxies;

	FDelegateHandle SkillsChangedHandle;
};
