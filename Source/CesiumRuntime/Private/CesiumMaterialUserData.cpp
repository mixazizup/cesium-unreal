#include "CesiumMaterialUserData.h"
#include "Materials/MaterialInstance.h"

void UCesiumMaterialUserData::PostEditChangeOwner() {
  Super::PostEditChangeOwner();

  this->LayerNames.Empty();

  UMaterialInstance* pMaterial = Cast<UMaterialInstance>(this->GetOuter());
  if (pMaterial) {
    const FStaticParameterSet& parameters = pMaterial->GetStaticParameters();
    const TArray<FStaticMaterialLayersParameter>& layerParameters =
        parameters.MaterialLayersParameters;

    for (const FStaticMaterialLayersParameter& layerParameter :
         layerParameters) {
      if (layerParameter.ParameterInfo.Name != "Cesium")
        continue;

      this->LayerNames.Reserve(layerParameter.Value.LayerNames.Num());
      for (const FText& text : layerParameter.Value.LayerNames) {
        this->LayerNames.Add(text.ToString());
      }
    }
  }
}
