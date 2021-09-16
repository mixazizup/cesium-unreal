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
 * The delegate for the ACesiumGeoreference::OnGeoreferenceUpdated,
 * which is triggered from UpdateGeoreference
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FGlobePositionChanged,
    const FQuat&,
    DeltaRotation);

/**
 * This component can be added to movable actors to globally georeference them
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Eath-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the ellipsoid.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGeoreferenceComponent : public UActorComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGeoreferenceComponent();

  /**
   * A delegate that will be called whenever the Actor's position on the globe
   * changes, either because the globe position is set directly or because its
   * Transform changes.
   *
   * The delegate will be called with a Rotator that specifies change in the
   * local "up" direction moving from the old position to the new position.
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium")
  FGlobePositionChanged OnGlobePositionChanged;

  /**
   * The georeference actor controlling how the owning actor's coordinate system
   * relates to the coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumGeoreference* Georeference = nullptr;

  /**
   * Using the teleport flag will move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool TeleportWhenUpdatingTransform = true;

  /**
   * The latitude in degrees of this component, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The longitude in degrees of this component, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The height in meters (above the ellipsoid) of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Height = 0.0;

  /**
   * The Earth-Centered Earth-Fixed X-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Y-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Z-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
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
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  void MoveToLongitudeLatitudeHeight(
      const glm::dvec3& TargetLongitudeLatitudeHeight,
      bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight,
      bool MaintainRelativeOrientation = true);

  /**
   * Returns the longitude in degrees (x), latitude in degrees (y),
   * and height in meters (z) of the actor, downcasted to a
   * single-precision floating point vector.
   *
   * The returned value may be invalid if this component is not yet registered.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetLongitudeLatitudeHeight() const;

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  void MoveToECEF(
      const glm::dvec3& TargetEcef,
      bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToECEF(
      const FVector& TargetEcef,
      bool MaintainRelativeOrientation = true);

  /**
   * Returns the Earth-Centered, Earth-Fixed (ECEF) coordinates of the actor,
   * downcasted to a single-precision floating point vector.
   *
   * The returned value may be invalid if this component is not yet registered.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetECEF() const;

  /**
   * Returns the Earth-Centered, Earth-Fixed (ECEF) coordinates of the actor.
   *
   * The returned value may be invalid if this component is not yet registered.
   */
  glm::dvec3 GetECEF() const;

  /**
   * Called by the owner actor when the world's OriginLocation changes (i.e.
   * during origin rebasing). The Component will recompute the Actor's
   * Location property based on the new OriginLocation and on this component's
   * high-precision representation of the position.
   *
   * This is scenario (5) in `georeference-component-scenarios.md`.
   */
  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

protected:
  /**
   * Called when a component is registered.
   *
   * In fact, this is called on many other occasions (including
   * changes to properties in the editor). Here, it us used to
   * attach the callback to `HandleActorTransformUpdated` to
   * the `TransformUpdated` delegate of the owner root, so that
   * movements in the actor can be used for updating the
   * ECEF coordinates.
   */
  virtual void OnRegister() override;

  /**
   * Called when a component is unregistered.
   *
   * In fact, this is called on many other occasions (including
   * changes to properties in the editor). Here, it us used to
   * DEtach the callback to `HandleActorTransformUpdated` from
   * the `TransformUpdated` delegate of the owner root.
   */
  virtual void OnUnregister() override;

  /**
   * Handle updates in the transform of the owning actor.
   *
   * This will be attached to the `TransformUpdated` delegate of
   * the owner root, and just call `_updateFromActor`, to
   * include the new actor transform in the ECEF coordinates.
   */
  void HandleActorTransformUpdated(
      USceneComponent* InRootComponent,
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport);

  /**
   * Called when a component is created (not loaded).
   * This can happen in the editor or during gameplay.
   *
   * This is overriden for initializing the Georeference by calling
   * _initGeoreference. This indeed seems to be called only exactly
   * once, when the component is created in the editor.
   */
  virtual void OnComponentCreated() override;

  /**
   * Do any object-specific cleanup required immediately after
   * loading an object.
   *
   * This is overriden for initializing the Georeference by calling
   * _initGeoreference. This indeed seems to  be called only exactly
   * once, when the component is loaded as part of a level in the editor.
   */
  virtual void PostLoad() override;

#if WITH_EDITOR

  /**
   * Called when a property on this object has been modified externally
   *
   * This is called every time that a value is modified in the editor UI.
   *
   * When a cartographic value is modified, calls
   * `MoveToLongitudeLatitudeHeight` with the new values.
   *
   * When an ECEF coordinate is modified, it calls `MoveToECEF` with the
   * new values.
   *
   * When the georeference has been be modified, then this will
   * attach the `HandleGeoreferenceUpdated` callback to the
   * `OnGeoreferenceUpdated` delegate of the new georeference,
   * and call `_updateActorTransform` to take the new georeference
   * into account.
   */
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

  virtual void PreEditUndo() override;
  virtual void PostEditUndo() override;
#endif

private:
  /**
   * A function that is attached to the `OnGeoreferenceUpdated` delegate,
   * of the Georeference, and just calls `_updateActorTransform`.
   */
  UFUNCTION()
  void HandleGeoreferenceUpdated();

  /**
   * Initializes the `Georeference`.
   *
   * If there is no `Georeference`, then a default one is obtained.
   * In any case, `HandleGeoreferenceUpdated` callback will be attached
   * to the `OnGeoreferenceChanged` delegate, to be notified about
   * changes in the georeference that may affect this component.
   */
  void _initGeoreference();

  /**
   * Updates the ECEF coordinates of this instance, based on an update
   * of the transform of the owning actor.
   *
   * This will compute the new ECEF coordinates, based on the actor
   * transform, and assign them to this instance by calling `_setECEF`.
   */
  void _updateFromActor();

  /**
   * Obtains the absolute location of the owner actor.
   *
   * This is the sum of the world origin location and the relative location
   * (as of `this->GetOwner()->GetRootComponent()->GetComponentLocation()`),
   * as a GLM vector, with the appropriate validity checks.
   */
  glm::dvec3 _getAbsoluteLocationFromActor();

  /**
   * Obtains the current rotation matrix from the transform of the actor.
   */
  glm::dmat3 _getRotationFromActor();

  /**
   * Set the position of this component, in ECEF coordinates.
   *
   * This will perform the necessary updates of the `ECEF_X`, `ECEF_Y`,
   * and `ECEF_Z` properties as well as the cartographic coordinate
   * properties, and call `_updateActorTransform`.
   *
   * @param targetEcef The ECEF coordinates.
   * @param maintainRelativeOrientation Whether the actor should be
   * rotated during this movement, so that the orientation relative
   * to the earth surface remains the same.
   */
  void _setECEF(const glm::dvec3& targetEcef, bool maintainRelativeOrientation);

  /**
   * Computes the relative location, from the given ECEF location
   * and the world origin location.
   */
  glm::dvec3 _computeRelativeLocation(const glm::dvec3& ecef);

  /**
   * Computes the normal of the ellipsoid, for the given ECEF position,
   * and returns it in the absolute unreal coordinate system
   */
  glm::dvec3 _computeEllipsoidNormalUnreal(const glm::dvec3& ecef);

  /**
   * Updates the transform of the owning actor.
   *
   * This is intended to be called when the underlying Georeference was
   * updated. It will use the (high-precision) `ECEF_X`, `ECEF_Y`, and
   * `ECEF_Z` coordinates of this instance, as well as the rotation
   * component of the current actor transform, and pass them to
   * `_updateActorTransform(rotation, translation)`
   */
  void _updateActorTransform();

  /**
   * Updates the transform of the owning actor.
   *
   * This will compute an updated transform matrix that is applied
   * to the owner by calling `SetWorldTransform`.
   *
   * @param rotation The rotation component
   * @param translation The translation component
   */
  void _updateActorTransform(
      const glm::dmat3& rotation,
      const glm::dvec3& translation);

  /**
   * Updates the `Longitude`, `Latitude` and `Height` properties
   * of this instance, based on the current ECEF_X/Y/Z coordinates.
   */
  void _updateDisplayLongitudeLatitudeHeight();

  /**
   * A function to print some debug message about the state
   * (absolute and relative location) of this component.
   */
  void _debugLogState();

  /**
   * Whether an update of the actor transform is currently in progress,
   * and further calls that are issued by HandleActorTransformUpdated
   * should be ignored
   */
  bool _updatingActorTransform;

  /**
   * The current ECEF coordinates.
   *
   * This reflects the `ECEF_X`, `ECEF_Y`, and `ECEF_Z` properties and is
   * updated whenever `_setECEF` is called.
   *
   * It is only used for tracking a change in the ECEF properties, so
   * that when `_setECEF` is called due to a change in the editor
   * and `maintainRelativeOrientation` is `true`, the orientation
   * can be updated based on this (previous) ECEF position, before
   * it is assigned with the new `ECEF_X`, `ECEF_Y`, and `ECEF_Z`
   * property values.
   */
  glm::dvec3 _currentEcef;

  /**
   * The current Unreal-to-ECEF rotation matrix.
   *
   * This reflects the state that is obtained from the Georeference,
   * as the rotation component of the matrix that is returned from
   * `geoTransforms.GetAbsoluteUnrealWorldToEllipsoidCenteredTransform()`.
   *
   * Whenever the Georeference is modified and `HandleGeoreferenceUpdated`
   * is called, this is used to compute the change in rotation from the
   * old state of the Georeference and the new, updated state. The change
   * between the old and the new rotation is applied to the actor
   * rotation, to keep its orientation consistent despite the change
   * of the underlying Georeference.
   *
   * This property is only valid while the Component is registered (i.e. when
   * IsRegistered() returns true).
   */
  glm::dmat3 _currentUnrealToEcef;

  UPROPERTY()
  bool _ecefIsValid = false;
};
