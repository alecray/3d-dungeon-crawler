#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopNPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

/**
 * A graybox shopkeeper. Interacting (E) opens the shop UI. The wares it sells are a code-defined list
 * (item ids); selling uses each item's value from the ItemDatabase.
 */
UCLASS()
class DUNGEONCRAWLER_API AShopNPC : public AActor
{
	GENERATED_BODY()

public:
	AShopNPC();

	/** Item ids this shop offers for sale (defaults to a basic starter stock if left empty). */
	UPROPERTY(EditAnywhere, Category = "Shop")
	TArray<FName> Wares;

	/** Returns the wares, falling back to a default stock when none are set. */
	const TArray<FName>& GetWares() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UCapsuleComponent> Body; // blocks Visibility so the interact trace can target it

	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	mutable TArray<FName> DefaultStock;
};
