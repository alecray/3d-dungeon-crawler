#include "DungeonGameInstance.h"
#include "StatsComponent.h"
#include "InventoryComponent.h"
#include "HotbarComponent.h"
#include "SkillTreeComponent.h"
#include "EquipmentComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"

void UDungeonGameInstance::Init()
{
	Super::Init();
	LoadProfile();
}

FString UDungeonGameInstance::GetGameVersion()
{
	FString Version;
	if (GConfig)
	{
		GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),
			TEXT("ProjectVersion"), Version, GGameIni);
	}
	if (Version.IsEmpty()) { Version = TEXT("0.0.0"); }
	return FString::Printf(TEXT("v%s"), *Version);
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

void UDungeonGameInstance::CaptureHotbar(const UHotbarComponent* Hotbar)
{
	if (Hotbar)
	{
		Profile.bInitialized = true;
		Profile.Hotbar = Hotbar->GetSlots();
		Profile.ActiveSlot = Hotbar->GetActiveIndex();
	}
}

void UDungeonGameInstance::ApplyHotbar(UHotbarComponent* Hotbar) const
{
	if (Hotbar && Profile.bInitialized && Profile.Hotbar.Num() > 0)
	{
		Hotbar->LoadFrom(Profile.Hotbar, Profile.ActiveSlot);
	}
}

void UDungeonGameInstance::CaptureSkills(const USkillTreeComponent* Skills)
{
	if (Skills)
	{
		Profile.bInitialized = true;
		Profile.SkillNodes = Skills->GetAllocationList();
	}
}

void UDungeonGameInstance::ApplySkills(USkillTreeComponent* Skills) const
{
	if (Skills && Profile.bInitialized)
	{
		Skills->LoadFrom(Profile.SkillNodes);
	}
}

void UDungeonGameInstance::CaptureEquipment(const UEquipmentComponent* Equipment)
{
	if (Equipment)
	{
		Profile.bInitialized = true;
		Profile.Equipment = Equipment->GetSlots();
	}
}

void UDungeonGameInstance::ApplyEquipment(UEquipmentComponent* Equipment) const
{
	if (Equipment && Profile.bInitialized && Profile.Equipment.Num() > 0)
	{
		Equipment->LoadFrom(Profile.Equipment);
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

void UDungeonGameInstance::MarkBossIntroSeen(FName BossId)
{
	if (!BossId.IsNone() && !Profile.SeenBossIntros.Contains(BossId))
	{
		Profile.bInitialized = true;
		Profile.SeenBossIntros.Add(BossId);
		SaveProfile(); // persist immediately so the cinematic never replays, even without a manual save
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
