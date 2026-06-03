#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CollectionLogWidget.generated.h"

class UVerticalBox;

/**
 * Collection log (pure C++): lists every rare+ item in the database, showing discovered ones in
 * their rarity color and undiscovered ones greyed out as "???".
 */
UCLASS()
class DUNGEONCRAWLER_API UCollectionLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

private:
	void Rebuild();

	UPROPERTY() TObjectPtr<UVerticalBox> List;
};
