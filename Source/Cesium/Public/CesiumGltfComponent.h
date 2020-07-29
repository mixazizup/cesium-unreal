// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include <memory>
#include "CesiumGltfComponent.generated.h"

class UMaterial;
class IPhysXCooking;

namespace tinygltf{
	class Model;
}

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CESIUM_API UCesiumGltfComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	class HalfConstructed {
	public:
		virtual ~HalfConstructed() = 0 {}
	};

	/// <summary>
	/// Constructs a UCesiumGltfComponent from the provided glTF model. This method does as much of the
	/// work in the calling thread as possible, and the calling thread need not be the game thread.
	/// The final component creation is done in the game thread (as required by Unreal Engine) and
	/// the provided callback is raised in the game thread with the result.
	/// </summary>
	static void CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, const glm::dmat4x4& transform, TFunction<void (UCesiumGltfComponent*)>);
	static std::unique_ptr<HalfConstructed> CreateOffGameThread(const tinygltf::Model& model, const glm::dmat4x4& transform, IPhysXCooking* pPhysXCooking = nullptr);
	static UCesiumGltfComponent* CreateOnGameThread(AActor* pParentActor, std::unique_ptr<HalfConstructed> pHalfConstructed);

	UCesiumGltfComponent();

	UPROPERTY(EditAnywhere)
	UMaterial* BaseMaterial;

	UFUNCTION(BlueprintCallable)
	void LoadModel(const FString& Url);

protected:
	//virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FString LoadedUrl;
	class UStaticMeshComponent* Mesh;

private:
	void ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
};
