#pragma once 

#include <memory>
#include "GameFramework/Actor.h"

namespace Cesium3DTiles {
    class CreditSystem;
}

UCLASS()
class ACesiumCreditSystem : AActor {

public:
	static ACesiumCreditSystem* GetDefaultForActor(AActor* Actor);

	ACesiumCreditSystem();
	virtual ~ACesiumCreditSystem();

    /**
	 * Credits text
	 *
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Credits = "";

    // Called every frame
    virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
    std::shared_ptr<Cesium3DTiles::CreditSystem> _pCreditSystem;
};