#include "DungeonGameInstance.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

void UDungeonGameInstance::Init()
{
	Super::Init();
	LoadProfile();
}

void UDungeonGameInstance::CaptureFromStats(const UStatsComponent* Stats, int32 Gold)
{
	if (!Stats)
	{
		return;
	}
	Profile.bInitialized = true;
	Profile.Attributes = Stats->GetAttributes();
	Profile.Level = Stats->GetLevel();
	Profile.XP = Stats->GetXP();
	Profile.SkillPoints = Stats->GetSkillPoints();
	Profile.AttributePoints = Stats->GetAttributePoints();
	Profile.Gold = Gold;
}

void UDungeonGameInstance::ApplyToStats(UStatsComponent* Stats) const
{
	if (!Stats || !Profile.bInitialized)
	{
		return; // fresh game: leave the component's default attributes
	}
	Stats->LoadFrom(Profile.Attributes, Profile.Level, Profile.XP, Profile.SkillPoints, Profile.AttributePoints);
}

void UDungeonGameInstance::CaptureInventory(const UInventoryComponent* Inventory)
{
	if (Inventory)
	{
		Profile.bInitialized = true;
		Profile.Inventory = Inventory->GetSlots();
	}
}

void UDungeonGameInstance::ApplyInventory(UInventoryComponent* Inventory) const
{
	if (Inventory && Profile.bInitialized && Profile.Inventory.Num() > 0)
	{
		Inventory->SetSlots(Profile.Inventory);
	}
}

void UDungeonGameInstance::AddDiscovered(FName ItemId)
{
	if (!ItemId.IsNone() && !Profile.DiscoveredItems.Contains(ItemId))
	{
		Profile.bInitialized = true;
		Profile.DiscoveredItems.Add(ItemId);
	}
}

bool UDungeonGameInstance::SaveProfile()
{
	UDungeonSaveGame* Save = Cast<UDungeonSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDungeonSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->Profile = Profile;
	return UGameplayStatics::SaveGameToSlot(Save, SlotName(), /*UserIndex*/ 0);
}

bool UDungeonGameInstance::LoadProfile()
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
	{
		return false;
	}
	if (UDungeonSaveGame* Save = Cast<UDungeonSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
	{
		Profile = Save->Profile;
		return true;
	}
	return false;
}
