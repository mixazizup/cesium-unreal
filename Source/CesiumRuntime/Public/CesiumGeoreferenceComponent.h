// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>

#include "CesiumGeoreferenceComponent.generated.h"

/**
 * This component can be added to movable actors to globally georeference them
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Eath-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the WGS84 ellipsoid.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGeoreferenceComponent : public USceneComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGeoreferenceComponent();

  /**
   * The georeference actor controlling how the owning actor's coordinate system
   * relates to the coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumGeoreference* Georeference;

  /**
   * Whether to automatically restore the precision of the Unreal transform from
   * the source Earth-Centered, Earth-Fixed (ECEF) transform during
   * origin-rebase. This is useful for maintaining high-precision for fixed
   * objects like buildings. This may need to be disabled for objects where the
   * Unreal transform is to be treated as the ground truth, e.g. Unreal physics
   * objects, cameras, etc.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool FixTransformOnOriginRebase = true;

  /**
   * Using the teleport flag will move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool TeleportWhenUpdatingTransform = true;

  /**
   * The latitude in degrees of the Georeference of this component, in the range [-90, 90]
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium", meta = (ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The longitude in degrees of the Georeference of this component, in the range [-180, 180]
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium", meta = (ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The height in meters (above the ellipsoid) of the Georeference of this component.
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  double Height = 0.0;

  /**
   * The Earth-Centered Earth-Fixed X-coordinate of the Georeference of this component.
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  double ECEF_X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Y-coordinate of the Georeference of this component.
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  double ECEF_Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Z-coordinate of the Georeference of this component.
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  double ECEF_Z = 0.0;

  /**
   * Aligns the local up direction with the ellipsoid normal at the current
   * location.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapLocalUpToEllipsoidNormal();

  /**
   * Turns the actor's local coordinate system into a East-South-Up tangent
   * space in centimeters.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapToEastSouthUp();


  /**
   * Move the actor to the specified WGS84 longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  // TODO GEOREF_REFACTORING This was only used in GlobeAnchorPanrent, 
  // and should be removed
  //void MoveToLongitudeLatitudeHeight(
  //    const glm::dvec3& TargetLongitudeLatitudeHeight,
  //    bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified WGS84 longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  // TODO GEOREF_REFACTORING This was only used in GlobeAnchorPanrent, 
  // and should be removed
  //UFUNCTION(BlueprintCallable, Category = "Cesium")
  //void InaccurateMoveToLongitudeLatitudeHeight(
  //    const FVector& TargetLongitudeLatitudeHeight,
  //    bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  // TODO GEOREF_REFACTORING This was not used at all. Movement should
  // be done on the Georeference
  //void MoveToECEF(
  //    const glm::dvec3& TargetEcef,
  //    bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  // TODO GEOREF_REFACTORING This was not used at all. Movement should
  // be done on the Georeference
  //UFUNCTION(BlueprintCallable, Category = "Cesium")
  //void InaccurateMoveToECEF(
  //    const FVector& TargetEcef,
  //    bool MaintainRelativeOrientation = true);

  /**
   * Delegate implementation to recieve a notification when the owner's root
   * component has changed.
   */
  UFUNCTION()
  void OnRootComponentChanged(
      USceneComponent* UpdatedComponent,
      bool bIsRootComponent);

  void SetAutoSnapToEastSouthUp(bool bValue);

  bool CheckCoordinatesChanged() const { return this->_dirty; }

  void MarkCoordinatesUnchanged() { this->_dirty = false; }

  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

  virtual void OnUpdateTransform(
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport) override;

  virtual void OnRegister() override;
  virtual void OnComponentCreated() override;
  virtual void OnAttachmentChanged() override;
  virtual void PostLoad() override;

  UFUNCTION()
  void HandleGeoreferenceUpdated();

  /**
   * Initializes the component. Occurs at level startup or actor spawn. 
   * Requires component to be registered, and bWantsInitializeComponent to be true.
   */
  void InitializeComponent() override;

protected:
  // Called when the game starts
  virtual bool MoveComponentImpl(
      const FVector& Delta,
      const FQuat& NewRotation,
      bool bSweep,
      FHitResult* OutHit = NULL,
      EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags,
      ETeleportType Teleport = ETeleportType::None) override;

  /**
   * Called after the C++ constructor and after the properties have
   * been initialized, including those loaded from config.
   */
  void PostInitProperties() override;
  
#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  void _initRootComponent();
  void _initWorldOriginLocation();
  void _updateAbsoluteLocation();
  void _updateRelativeLocation();
  void _updateActorToECEF();
  void _updateActorToUnrealRelativeWorldTransform();
  void _setTransform(const glm::dmat4& transform);
  void _setECEF(const glm::dvec3& targetEcef, bool maintainRelativeOrientation);
  void _updateDisplayLongitudeLatitudeHeight();
  void _updateDisplayECEF();

  void _initGeoreference();

  // TODO GEOREF_REFACTORING I'm somewhat (but not entirely) sure
  // that these properties are redundant and/or not updated when
  // the corresponding value in UE changes. Review this...
  glm::dvec3 _worldOriginLocation;
  glm::dvec3 _absoluteLocation;
  glm::dvec3 _relativeLocation;

  // TODO GEOREF_REFACTORING
  // I'm relatively sure that serializing these should not be
  // becessary, because they are derived from the GeoRef
  // Note: this is done to allow Unreal to recognize and serialize _actorToECEF
  UPROPERTY()
  double _actorToECEF_Array[16];

  glm::dmat4& _actorToECEF = *(glm::dmat4*)_actorToECEF_Array;

  glm::dmat4 _actorToUnrealRelativeWorld;
  USceneComponent* _ownerRoot;

  bool _ignoreOnUpdateTransform;
  UPROPERTY()
  bool _autoSnapToEastSouthUp;
  bool _dirty;
};
