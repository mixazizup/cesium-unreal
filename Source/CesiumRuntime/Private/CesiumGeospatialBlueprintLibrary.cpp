// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeospatialBlueprintLibrary.h"

#include "CesiumGeospatialLibrary.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
#include "VecMath.h"

TWeakObjectPtr<ACesiumGeoreference>
    UCesiumGeospatialBlueprintLibrary::_defaultGeorefPtr;

ACesiumGeoreference* UCesiumGeospatialBlueprintLibrary::_getDefaultGeoref(
    const UObject* WorldContextObject) {
  if (_defaultGeorefPtr.Get()) {
    return _defaultGeorefPtr.Get();
  }
  _defaultGeorefPtr =
      ACesiumGeoreference::GetDefaultGeoreference(WorldContextObject);
  return _defaultGeorefPtr.Get();
}

FVector UCesiumGeospatialBlueprintLibrary::TransformLongLatHeightToUnreal(
    const FVector& LongLatHeight,
    const ACesiumGeoreference* Georef) {
  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in TransformLongLatHeightToUnreal call"));
    return FVector::ZeroVector;
  }

  glm::dvec3 ue = UCesiumGeospatialLibrary::TransformLongLatHeightToUnreal(
      VecMath::createVector3D(LongLatHeight),
      Georef->GetEllipsoidCenteredToUnrealWorldTransform(),
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation));

  return FVector(ue.x, ue.y, ue.z);
}

FVector UCesiumGeospatialBlueprintLibrary::
    TransformLongLatHeightToUnrealUsingDefaultGeoref(
        const UObject* WorldContextObject,
        const FVector& LongLatHeight) {
  return TransformLongLatHeightToUnreal(
      LongLatHeight,
      _getDefaultGeoref(WorldContextObject));
}

FVector UCesiumGeospatialBlueprintLibrary::TransformUnrealToLongLatHeight(
    const FVector& UeLocation,
    const ACesiumGeoreference* Georef) {
  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in TransformUnrealToLongLatHeight call"));
    return FVector::ZeroVector;
  }

  glm::dvec3 longLatHeight =
      UCesiumGeospatialLibrary::TransformUnrealToLongLatHeight(
          VecMath::createVector3D(UeLocation),
          Georef->GetUnrealWorldToEllipsoidCenteredTransform(),
          VecMath::createVector3D(Georef->GetWorld()->OriginLocation));
  return FVector(longLatHeight.x, longLatHeight.y, longLatHeight.z);
}

FVector UCesiumGeospatialBlueprintLibrary::
    TransformUnrealToLongLatHeightUsingDefaultGeoref(
        const UObject* WorldContextObject,
        const FVector& UeLocation) {
  return TransformUnrealToLongLatHeight(
      UeLocation,
      _getDefaultGeoref(WorldContextObject));
}

FVector UCesiumGeospatialBlueprintLibrary::TransformLongLatHeightToEcef(
    const FVector& LongLatHeight) {

  glm::dvec3 ecef = UCesiumGeospatialLibrary::TransformLongLatHeightToEcef(
      VecMath::createVector3D(LongLatHeight));
  return FVector(ecef.x, ecef.y, ecef.z);
}

FVector UCesiumGeospatialBlueprintLibrary::TransformEcefToLongLatHeight(
    const FVector& Ecef) {
  glm::dvec3 llh = UCesiumGeospatialLibrary::TransformEcefToLongLatHeight(
      VecMath::createVector3D(Ecef));
  return FVector(llh.x, llh.y, llh.z);
}

FRotator UCesiumGeospatialBlueprintLibrary::TransformRotatorEastNorthUpToUnreal(
    const FRotator& EnuRotator,
    const FVector& UeLocation,
    ACesiumGeoreference* Georef) {

  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in TransformRotatorEastNorthUpToUnreal call"));
    return FRotator::ZeroRotator;
  }

  const glm::dmat3 adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorEastNorthUpToUnreal(
          VecMath::createRotationMatrix4D(EnuRotator),
          VecMath::createVector3D(UeLocation),
          Georef->GetUnrealWorldToEllipsoidCenteredTransform(),
          VecMath::createVector3D(Georef->GetWorld()->OriginLocation),
          Georef->GetEllipsoidCenteredToGeoreferencedTransform());

  return VecMath::createRotator(adjustedRotation);
}

FRotator UCesiumGeospatialBlueprintLibrary::
    TransformRotatorEastNorthUpToUnrealUsingDefaultGeoref(
        const UObject* WorldContextObject,
        const FRotator& EnuRotator,
        const FVector& UeLocation) {
  return TransformRotatorEastNorthUpToUnreal(
      EnuRotator,
      UeLocation,
      _getDefaultGeoref(WorldContextObject));
}

FRotator UCesiumGeospatialBlueprintLibrary::TransformRotatorUnrealToEastNorthUp(
    const FRotator& UeRotator,
    const FVector& UeLocation,
    ACesiumGeoreference* Georef) {

  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in TransformRotatorUnrealToEastNorthUp call"));
    return FRotator::ZeroRotator;
  }

  const glm::dmat3& adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorUnrealToEastNorthUp(
          VecMath::createRotationMatrix4D(UeRotator),
          VecMath::createVector3D(UeLocation),
          Georef->GetUnrealWorldToEllipsoidCenteredTransform(),
          VecMath::createVector3D(Georef->GetWorld()->OriginLocation),
          Georef->GetEllipsoidCenteredToGeoreferencedTransform());

  return VecMath::createRotator(adjustedRotation);
}

FRotator UCesiumGeospatialBlueprintLibrary::
    TransformRotatorUnrealToEastNorthUpUsingDefaultGeoref(
        const UObject* WorldContextObject,
        const FRotator& UeRotator,
        const FVector& UeLocation) {
  return TransformRotatorUnrealToEastNorthUp(
      UeRotator,
      UeLocation,
      _getDefaultGeoref(WorldContextObject));
}

FMatrix UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToUnreal(
    const FVector& UeLocation,
    ACesiumGeoreference* Georef) {

  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in ComputeEastNorthUpToUnreal call"));
    return FMatrix::Identity;
  }

  const glm::dmat3& enuToUnreal =
      UCesiumGeospatialLibrary::ComputeEastNorthUpToUnreal(
          VecMath::createVector3D(UeLocation),
          Georef->GetUnrealWorldToEllipsoidCenteredTransform(),
          VecMath::createVector3D(Georef->GetWorld()->OriginLocation),
          Georef->GetEllipsoidCenteredToGeoreferencedTransform());

  return VecMath::createMatrix(enuToUnreal);
}

FMatrix
UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToUnrealUsingDefaultGeoref(
    const UObject* WorldContextObject,
    const FVector& UeLocation) {
  return ComputeEastNorthUpToUnreal(
      UeLocation,
      _getDefaultGeoref(WorldContextObject));
}

FMatrix UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToEcef(
    const FVector& Ecef) {
  glm::dmat3 enuToEcef = UCesiumGeospatialLibrary::ComputeEastNorthUpToEcef(
      glm::dvec3(Ecef.X, Ecef.Y, Ecef.Z));

  return VecMath::createMatrix(enuToEcef);
}
