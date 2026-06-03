#include "ItemTypes.h"
#include "Math/RandomStream.h"

FLinearColor RarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:    return FLinearColor(0.55f, 0.55f, 0.55f); // gray
	case EItemRarity::Uncommon:  return FLinearColor(0.25f, 0.7f, 0.25f);  // green
	case EItemRarity::Rare:      return FLinearColor(0.2f, 0.45f, 0.95f);  // blue
	case EItemRarity::Epic:      return FLinearColor(0.65f, 0.3f, 0.9f);   // purple
	case EItemRarity::Legendary: return FLinearColor(0.95f, 0.6f, 0.15f);  // orange
	default:                     return FLinearColor::Gray;
	}
}

namespace ItemDatabase
{
	static TArray<FItemDef> BuildItems()
	{
		auto MakeItem = [](const TCHAR* Id, const TCHAR* Name, EItemType Type, EItemRarity Rarity, int32 MaxStack, int32 Value)
		{
			FItemDef D;
			D.Id = FName(Id);
			D.DisplayName = Name;
			D.Type = Type;
			D.Rarity = Rarity;
			D.MaxStack = MaxStack;
			D.Value = Value;
			return D;
		};

		TArray<FItemDef> Items;
		Items.Add(MakeItem(TEXT("HealthPotion"), TEXT("Health Potion"), EItemType::Consumable, EItemRarity::Common, 10, 15));
		Items.Add(MakeItem(TEXT("ManaPotion"),   TEXT("Mana Potion"),   EItemType::Consumable, EItemRarity::Common, 10, 15));
		Items.Add(MakeItem(TEXT("Bone"),         TEXT("Old Bone"),      EItemType::Material,   EItemRarity::Common, 20, 2));
		Items.Add(MakeItem(TEXT("IronShard"),    TEXT("Iron Shard"),    EItemType::Material,   EItemRarity::Common, 20, 5));
		Items.Add(MakeItem(TEXT("IronSword"),    TEXT("Iron Sword"),    EItemType::Weapon,     EItemRarity::Uncommon, 1, 60));
		Items.Add(MakeItem(TEXT("LeatherArmor"), TEXT("Leather Armor"), EItemType::Armor,      EItemRarity::Uncommon, 1, 55));
		Items.Add(MakeItem(TEXT("RubyGem"),      TEXT("Ruby Gem"),      EItemType::Treasure,   EItemRarity::Rare,     5, 120));
		Items.Add(MakeItem(TEXT("RunedBlade"),   TEXT("Runed Blade"),   EItemType::Weapon,     EItemRarity::Epic,     1, 400));
		Items.Add(MakeItem(TEXT("AncientRelic"), TEXT("Ancient Relic"), EItemType::Treasure,   EItemRarity::Legendary, 1, 1000));
		return Items;
	}

	static const TArray<FItemDef>& Items()
	{
		static const TArray<FItemDef> Cached = BuildItems();
		return Cached;
	}

	static const TMap<FName, int32>& IndexById()
	{
		static const TMap<FName, int32> Map = []()
		{
			TMap<FName, int32> M;
			const TArray<FItemDef>& List = Items();
			for (int32 i = 0; i < List.Num(); ++i)
			{
				M.Add(List[i].Id, i);
			}
			return M;
		}();
		return Map;
	}

	const FItemDef& Get(FName Id)
	{
		if (const int32* Index = IndexById().Find(Id))
		{
			return Items()[*Index];
		}
		static const FItemDef Invalid;
		return Invalid;
	}

	bool Contains(FName Id)
	{
		return IndexById().Contains(Id);
	}

	const TArray<FItemDef>& All()
	{
		return Items();
	}

	FName RollRandomItem(FRandomStream& Rng)
	{
		// Weight by rarity: commons are far more likely than legendaries.
		const TArray<FItemDef>& List = Items();
		float Total = 0.f;
		TArray<float> Weights;
		Weights.Reserve(List.Num());
		for (const FItemDef& D : List)
		{
			float W = 1.f;
			switch (D.Rarity)
			{
			case EItemRarity::Common:    W = 50.f; break;
			case EItemRarity::Uncommon:  W = 22.f; break;
			case EItemRarity::Rare:      W = 9.f;  break;
			case EItemRarity::Epic:      W = 3.f;  break;
			case EItemRarity::Legendary: W = 1.f;  break;
			}
			Weights.Add(W);
			Total += W;
		}

		float Pick = Rng.FRandRange(0.f, Total);
		for (int32 i = 0; i < List.Num(); ++i)
		{
			Pick -= Weights[i];
			if (Pick <= 0.f)
			{
				return List[i].Id;
			}
		}
		return List.Num() > 0 ? List[0].Id : NAME_None;
	}
}
