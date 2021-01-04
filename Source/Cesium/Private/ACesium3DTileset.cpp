// Fill out your copyright notice in the Description page of Project Settings.

#include "ACesium3DTileset.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGltfComponent.h"
#include "CesiumTransforms.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HttpModule.h"
#include "IPhysXCookingModule.h"
#include "PhysicsPublicCore.h"
#include "Math/UnrealMathUtility.h"
#include "UnrealAssetAccessor.h"
#include "UnrealConversions.h"
#include "UnrealTaskProcessor.h"
#include "CesiumRasterOverlay.h"
#include "Cesium3DTilesetRoot.h"
#include "Cesium3DTiles/Tileset.h"
#include "SpdlogUnrealLoggerSink.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Slate/SceneViewport.h"
#endif

// Sets default values
ACesium3DTileset::ACesium3DTileset() :
	Georeference(nullptr),
	_pTileset(nullptr),
	_lastTilesRendered(0),
	_lastTilesLoadingLowPriority(0),
	_lastTilesLoadingMediumPriority(0),
	_lastTilesLoadingHighPriority(0),

	_lastTilesVisited(0),
	_lastTilesCulled(0),
	_lastMaxDepthVisited(0),

	_updateGeoreferenceOnBoundingVolumeReady(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->SetActorEnableCollision(true);

	this->RootComponent = CreateDefaultSubobject<UCesium3DTilesetRoot>(TEXT("Tileset"));
	this->RootComponent->SetMobility(EComponentMobility::Static);
}

ACesium3DTileset::~ACesium3DTileset() {
	this->DestroyTileset();
}

const glm::dmat4& ACesium3DTileset::GetCesiumTilesetToUnrealRelativeWorldTransform() const
{
	return Cast<UCesium3DTilesetRoot>(this->RootComponent)->GetCesiumTilesetToUnrealRelativeWorldTransform();
}

bool ACesium3DTileset::IsBoundingVolumeReady() const
{
	// TODO: detect failures that will cause the root tile to never exist.
	// That counts as "ready" too.
	return this->_pTileset && this->_pTileset->getRootTile();
}

std::optional<Cesium3DTiles::BoundingVolume> ACesium3DTileset::GetBoundingVolume() const
{
	if (!this->IsBoundingVolumeReady()) {
		return std::nullopt;
	}

	return this->_pTileset->getRootTile()->getBoundingVolume();
}

void ACesium3DTileset::UpdateTransformFromCesium(const glm::dmat4& cesiumToUnreal)
{
	TArray<UCesiumGltfComponent*> gltfComponents;
	this->GetComponents<UCesiumGltfComponent>(gltfComponents);

	for (UCesiumGltfComponent* pGltf : gltfComponents) {
		pGltf->UpdateTransformFromCesium(cesiumToUnreal);
	}
}

// TODO: This parameter is unused. 
// I think pRoot->RecalculateTransform() is recalculating ellipsoidCenteredToGeoreferencedTransform,
// should pRoot->RecalculateTransform() be changed to take ellipsoidCenteredToGeoreferencedTransform as a parameter?
// If so, we will need to save it along with the boolean in the case that the bounding volume is not ready
void ACesium3DTileset::UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform)
{
	// If the bounding volume is ready, we can update the georeference transform as wanted
	if (IsBoundingVolumeReady()) {
		UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
		pRoot->RecalculateTransform();
	} else {
	// Otherwise, update the transform later in Tick when the bounding volume is ready 
		this->_updateGeoreferenceOnBoundingVolumeReady = true;
	}
}

// Called when the game starts or when spawned
void ACesium3DTileset::BeginPlay()
{
	Super::BeginPlay();

	this->LoadTileset();
}

void ACesium3DTileset::OnConstruction(const FTransform& Transform)
{
	this->LoadTileset();
}

void ACesium3DTileset::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	//std::cout << "Hit face index: " << Hit.FaceIndex << std::endl;

	//FHitResult detailedHit;
	//FCollisionQueryParams params;
	//params.bReturnFaceIndex = true;
	//params.bTraceComplex = true;
	//MyComp->LineTraceComponent(detailedHit, Hit.TraceStart, Hit.TraceEnd, params);

	//std::cout << "Hit face index 2: " << detailedHit.FaceIndex << std::endl;
}

class UnrealResourcePreparer : public Cesium3DTiles::IPrepareRendererResources {
public:
	UnrealResourcePreparer(ACesium3DTileset* pActor) :
		_pActor(pActor)
#if PHYSICS_INTERFACE_PHYSX
		,_pPhysXCooking(GetPhysXCookingModule()->GetPhysXCooking())
#endif
	{}

	virtual void* prepareInLoadThread(const tinygltf::Model& model, const glm::dmat4& transform) {
		std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf = UCesiumGltfComponent::CreateOffGameThread(
			model,
			transform
#if PHYSICS_INTERFACE_PHYSX
			,this->_pPhysXCooking
#endif
		);
		return pHalf.release();
	}

	virtual void* prepareInMainThread(Cesium3DTiles::Tile& tile, void* pLoadThreadResult) {
		const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
		if (!pContent) {
			return nullptr;
		}

		if (pContent->model) {
			std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf(reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(pLoadThreadResult));
			return UCesiumGltfComponent::CreateOnGameThread(
				this->_pActor,
				std::move(pHalf),
				_pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
				this->_pActor->Material
			);
		}

		return nullptr;
	}

	virtual void free(Cesium3DTiles::Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) noexcept {
		if (pLoadThreadResult) {
			UCesiumGltfComponent::HalfConstructed* pHalf = reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(pLoadThreadResult);
			delete pHalf;
		} else if (pMainThreadResult) {
			UCesiumGltfComponent* pGltf = reinterpret_cast<UCesiumGltfComponent*>(pMainThreadResult);
			if (pGltf) {
				this->destroyRecursively(pGltf);
			}
		}
	}

	virtual void* prepareRasterInLoadThread(const tinygltf::Image& image) override {
		return nullptr;
	}

	virtual void* prepareRasterInMainThread(const Cesium3DTiles::RasterOverlayTile& rasterTile, void* pLoadThreadResult) {
		const tinygltf::Image& image = rasterTile.getImage();
		if (image.width <= 0 || image.height <= 0) {
			return nullptr;
		}

		UTexture2D* pTexture = UTexture2D::CreateTransient(image.width, image.height, PF_R8G8B8A8);
		pTexture->AddToRoot();
		pTexture->AddressX = TextureAddress::TA_Clamp;
		pTexture->AddressY = TextureAddress::TA_Clamp;

		unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		FMemory::Memcpy(pTextureData, image.image.data(), image.image.size());
		pTexture->PlatformData->Mips[0].BulkData.Unlock();

		pTexture->UpdateResource();

		return pTexture;
	}

	virtual void freeRaster(const Cesium3DTiles::RasterOverlayTile& rasterTile, void* pLoadThreadResult, void* pMainThreadResult) noexcept override {
		UTexture2D* pTexture = static_cast<UTexture2D*>(pMainThreadResult);
		if (!pTexture) {
			return;
		}

		pTexture->RemoveFromRoot();
	}

	virtual void attachRasterInMainThread(
		const Cesium3DTiles::Tile& tile,
		uint32_t overlayTextureCoordinateID,
		const Cesium3DTiles::RasterOverlayTile& rasterTile,
		void* pMainThreadRendererResources,
		const CesiumGeometry::Rectangle& textureCoordinateRectangle,
		const glm::dvec2& translation,
		const glm::dvec2& scale
	) {
		const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
		if (!pContent) {
			return;
		}

		if (pContent->model) {
			UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
			if (pGltfContent) {
				pGltfContent->AttachRasterTile(tile, rasterTile, static_cast<UTexture2D*>(pMainThreadRendererResources), textureCoordinateRectangle, translation, scale);
			}
		}
	}

	virtual void detachRasterInMainThread(
		const Cesium3DTiles::Tile& tile,
		uint32_t overlayTextureCoordinateID,
		const Cesium3DTiles::RasterOverlayTile& rasterTile,
		void* pMainThreadRendererResources,
		const CesiumGeometry::Rectangle& textureCoordinateRectangle
	) noexcept override {
		const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
		if (!pContent) {
			return;
		}

		if (pContent->model) {
			UCesiumGltfComponent* pGltfContent = reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
			if (pGltfContent) {
				pGltfContent->DetachRasterTile(tile, rasterTile, static_cast<UTexture2D*>(pMainThreadRendererResources), textureCoordinateRectangle);
			}
		}
	}

private:
	// static int32 GetObjReferenceCount(UObject* Obj, TArray<UObject*>* OutReferredToObjects = nullptr)
	// {
	// 	if (!Obj ||
	// 		!Obj->IsValidLowLevelFast())
	// 	{
	// 		return -1;
	// 	}

	// 	TArray<UObject*> ReferredToObjects;             //req outer, ignore archetype, recursive, ignore transient
	// 	FReferenceFinder ObjectReferenceCollector(ReferredToObjects, nullptr, false, true, true, false);
	// 	ObjectReferenceCollector.FindReferences(Obj);

	// 	if (OutReferredToObjects)
	// 	{
	// 		OutReferredToObjects->Append(ReferredToObjects);
	// 	}
	// 	return ReferredToObjects.Num();
	// }

	void destroyRecursively(USceneComponent* pComponent) {
		// FString newName = TEXT("Destroyed_") + pComponent->GetName();
		// pComponent->Rename(*newName);

		// TArray<UObject*> before;
		// GetObjReferenceCount(pComponent, &before);

		if (pComponent->IsRegistered()) {
			pComponent->UnregisterComponent();
		}

		TArray<USceneComponent*> children = pComponent->GetAttachChildren();
		for (USceneComponent* pChild : children) {
			this->destroyRecursively(pChild);
		}

		pComponent->DestroyPhysicsState();
		pComponent->DestroyComponent();

		// TArray<UObject*> after;
		// GetObjReferenceCount(pComponent, &after);

		// UE_LOG(LogActor, Warning, TEXT("Before %d, After %d"), before.Num(), after.Num());
	}

	ACesium3DTileset* _pActor;
#if PHYSICS_INTERFACE_PHYSX
	IPhysXCooking* _pPhysXCooking;
#endif
};

void ACesium3DTileset::LoadTileset()
{
	Cesium3DTiles::Tileset* pTileset = this->_pTileset;
	if (pTileset)
	{
		if (this->Url.Len() > 0)
		{
			if (pTileset->getUrl() && wstr_to_utf8(this->Url) == pTileset->getUrl())
			{
				// Already using this URL.
				return;
			}
		}
		else
		{
			if (pTileset->getIonAssetID() && pTileset->getIonAccessToken() && this->IonAssetID == pTileset->getIonAssetID() && wstr_to_utf8(this->IonAccessToken) == pTileset->getIonAccessToken())
			{
				// Already using this asset ID and access token.
				return;
			}
		}

		this->DestroyTileset();
	}

	if (!this->Georeference) {
		this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
	}

	this->Georeference->AddGeoreferencedObject(this);

	Cesium3DTiles::TilesetExternals externals{
		std::make_shared<UnrealAssetAccessor>(),
		std::make_shared<UnrealResourcePreparer>(this),
		std::make_shared<UnrealTaskProcessor>(),
		spdlog::default_logger()
	};

	if (this->Url.Len() > 0)
	{
		pTileset = new Cesium3DTiles::Tileset(externals, wstr_to_utf8(this->Url));
	}
	else
	{
		pTileset = new Cesium3DTiles::Tileset(externals, this->IonAssetID, wstr_to_utf8(this->IonAccessToken));
	}

	TArray<UCesiumRasterOverlay*> rasterOverlays;
	this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

	this->_pTileset = pTileset;

	for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
		if (pOverlay->IsActive()) {
			pOverlay->AddToTileset();
		}
	}
}

void ACesium3DTileset::DestroyTileset() {
	if (!this->_pTileset) {
		return;
	}

	delete this->_pTileset;
	this->_pTileset = nullptr;
}

std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetCamera() const {
	std::optional<UnrealCameraParameters> camera = this->GetPlayerCamera();

#if WITH_EDITOR
	if (!camera) {
		camera = this->GetEditorCamera();
	}
#endif

	return camera;
}

std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetPlayerCamera() const
{
	UWorld* pWorld = this->GetWorld();
	if (!pWorld) {
		return std::optional<UnrealCameraParameters>();
	}

	APlayerController* pPlayerController = pWorld->GetFirstPlayerController();
	if (!pPlayerController) {
		return std::optional<UnrealCameraParameters>();
	}

	APlayerCameraManager* pCameraManager = pPlayerController->PlayerCameraManager;
	if (!pCameraManager) {
		return std::optional<UnrealCameraParameters>();
	}

	UGameViewportClient* pViewport = pWorld->GetGameViewport();
	if (!pViewport) {
		return std::optional<UnrealCameraParameters>();
	}

	const FMinimalViewInfo& pov = pCameraManager->ViewTarget.POV;
	const FVector& location = pov.Location;
	const FRotator& rotation = pCameraManager->ViewTarget.POV.Rotation;
	double fov = pov.FOV;

	FVector2D size;
	pViewport->GetViewportSize(size);

	if (size.X < 1.0 || size.Y < 1.0) {
		return std::nullopt;
	}

	return UnrealCameraParameters {
		size,
		location,
		rotation,
		fov
	};
}

Cesium3DTiles::Camera ACesium3DTileset::CreateCameraFromViewParameters(
	const FVector2D& viewportSize,
	const FVector& location,
	const FRotator& rotation,
	double fieldOfViewDegrees
) const {
	double horizontalFieldOfView = FMath::DegreesToRadians(fieldOfViewDegrees);

	double aspectRatio = viewportSize.X / viewportSize.Y;
	double verticalFieldOfView = atan(tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;

	FVector direction = rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	FVector up = rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	glm::dmat4 unrealWorldToTileset = glm::affineInverse(this->GetCesiumTilesetToUnrealRelativeWorldTransform());

	return Cesium3DTiles::Camera(
		unrealWorldToTileset * glm::dvec4(location.X, location.Y, location.Z, 1.0),
		glm::normalize(unrealWorldToTileset * glm::dvec4(direction.X, direction.Y, direction.Z, 0.0)),
		glm::normalize(unrealWorldToTileset * glm::dvec4(up.X, up.Y, up.Z, 0.0)),
		glm::dvec2(viewportSize.X, viewportSize.Y),
		horizontalFieldOfView,
		verticalFieldOfView
	);
}

#if WITH_EDITOR
std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetEditorCamera() const
{
	FViewport* pViewport = GEditor->GetActiveViewport();
	FViewportClient* pViewportClient = pViewport->GetClient();
	FEditorViewportClient* pEditorViewportClient = static_cast<FEditorViewportClient*>(pViewportClient);
	const FVector& location = pEditorViewportClient->GetViewLocation();
	const FRotator& rotation = pEditorViewportClient->GetViewRotation();
	double fov = pEditorViewportClient->FOVAngle;
	FVector2D size = pViewport->GetSizeXY();

	if (size.X < 1.0 || size.Y < 1.0) {
		return std::nullopt;
	}

	return UnrealCameraParameters{
		size,
		location,
		rotation,
		fov
	};
}
#endif

bool ACesium3DTileset::ShouldTickIfViewportsOnly() const {
	return this->ShowInEditor;
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
	if (pRoot->IsTransformChanged()) {
		this->UpdateTransformFromCesium(this->GetCesiumTilesetToUnrealRelativeWorldTransform());
		pRoot->MarkTransformUnchanged();
	}

	// If a georeference update is waiting on the bounding volume being ready, update when ready
	if (this->_updateGeoreferenceOnBoundingVolumeReady && this->IsBoundingVolumeReady()) {
		this->_updateGeoreferenceOnBoundingVolumeReady = false;
		// TODO: since the parameter is unused anyways, is this the right way to do this?
		glm::dmat4 tmp;
		this->UpdateGeoreferenceTransform(tmp);
	}

	if (this->SuspendUpdate) {
		return;
	}

	Cesium3DTiles::TilesetOptions& options = this->_pTileset->getOptions();
	options.maximumScreenSpaceError = this->MaximumScreenSpaceError;

	std::optional<UnrealCameraParameters> camera = this->GetCamera();
	if (!camera) {
		return;
	}

	Cesium3DTiles::Camera tilesetCamera = this->CreateCameraFromViewParameters(
		camera.value().viewportSize,
		camera.value().location,
		camera.value().rotation,
		camera.value().fieldOfViewDegrees
	);

	const Cesium3DTiles::ViewUpdateResult& result = this->_pTileset->updateView(tilesetCamera);

	if (
		result.tilesToRenderThisFrame.size() != this->_lastTilesRendered ||
		result.tilesLoadingLowPriority != this->_lastTilesLoadingLowPriority ||
		result.tilesLoadingMediumPriority != this->_lastTilesLoadingMediumPriority ||
		result.tilesLoadingHighPriority != this->_lastTilesLoadingHighPriority ||
		result.tilesVisited != this->_lastTilesVisited ||
		result.tilesCulled != this->_lastTilesCulled ||
		result.maxDepthVisited != this->_lastMaxDepthVisited
	) {
		this->_lastTilesRendered = result.tilesToRenderThisFrame.size();
		this->_lastTilesLoadingLowPriority = result.tilesLoadingLowPriority;
		this->_lastTilesLoadingMediumPriority = result.tilesLoadingMediumPriority;
		this->_lastTilesLoadingHighPriority = result.tilesLoadingHighPriority;

		this->_lastTilesVisited = result.tilesVisited;
		this->_lastTilesCulled = result.tilesCulled;
		this->_lastMaxDepthVisited = result.maxDepthVisited;

		UE_LOG(
			LogActor,
			Warning,
			TEXT("%s: Visited %d, Rendered %d, Culled %d, Max Depth Visited: %d, Loading-Low %d, Loading-Medium %d, Loading-High %d"),
			*this->GetName(),
			result.tilesVisited,
			result.tilesToRenderThisFrame.size(),
			result.tilesCulled,
			result.maxDepthVisited,
			result.tilesLoadingLowPriority,
			result.tilesLoadingMediumPriority,
			result.tilesLoadingHighPriority
		);
	}

	for (Cesium3DTiles::Tile* pTile : result.tilesToNoLongerRenderThisFrame) {
		if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
			continue;
		}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (Gltf && Gltf->IsVisible()) {
			Gltf->SetVisibility(false, true);
			Gltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		} else {
			// TODO: why is this happening?
		}
	}

	for (Cesium3DTiles::Tile* pTile : result.tilesToRenderThisFrame) {
		if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
			continue;
		}

		//const Cesium3DTiles::TileID& id = pTile->getTileID();
		//const CesiumGeometry::QuadtreeTileID* pQuadtreeID = std::get_if<CesiumGeometry::QuadtreeTileID>(&id);
		//if (!pQuadtreeID || pQuadtreeID->level != 14 || pQuadtreeID->x != 5503 || pQuadtreeID->y != 11626) {
		//	continue;
		//}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (!Gltf) {
			// TODO: Not-yet-renderable tiles shouldn't be here.
			continue;
		}

		if (Gltf->GetAttachParent() == nullptr) {
			Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		if (!Gltf->IsVisible()) {
			Gltf->SetVisibility(true, true);
			Gltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

void ACesium3DTileset::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	this->DestroyTileset();
	AActor::EndPlay(EndPlayReason);
}

void ACesium3DTileset::BeginDestroy() {
	this->DestroyTileset();
	AActor::BeginDestroy();
}