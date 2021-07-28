#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumGltfPrimitiveComponent.h"

FCesiumMetadataPrimitive
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return FCesiumMetadataPrimitive();
  }

  return pGltfComponent->Metadata;
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumMetadataFeatureTable>& featureTables =
      metadata.GetFeatureTables();
  if (featureTables.Num() == 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataFeatureTable& featureTable = featureTables[0];
  int64 featureID = GetFeatureIDForFace(metadata, featureTable, faceID);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  return featureTable.GetPropertiesForFeatureID(featureID);
}

TMap<FString, FString>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesAsStringForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumMetadataFeatureTable>& featureTables =
      metadata.GetFeatureTables();
  if (featureTables.Num() == 0) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataFeatureTable& featureTable = featureTables[0];
  int64 featureID = GetFeatureIDForFace(metadata, featureTable, faceID);
  if (featureID < 0) {
    return TMap<FString, FString>();
  }

  return featureTable.GetPropertiesAsStringsForFeatureID(featureID);
}

int64 UCesiumMetadataUtilityBlueprintLibrary::GetFeatureIDForFace(
    UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
    int64 faceID) {
  return FeatureTable.GetFeatureIDForVertex(
      Primitive.GetFirstVertexIDFromFaceID(faceID));
}
