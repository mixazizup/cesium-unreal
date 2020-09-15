// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlay.h"
#include "CesiumBingMapsOverlay.generated.h"

UENUM(BlueprintType)
enum class EBingMapsStyle : uint8 {
	Aerial UMETA(DisplayName = "Aerial"),
    AerialWithLabelsOnDemand UMETA(DisplayName = "Aerial with Labels"),
	RoadOnDemand UMETA(DisplayName = "Road"),
	CanvasDark UMETA(DisplayName = "Canvas Dark"),
	CanvasLight UMETA(DisplayName = "Canvas Light"),
	CanvasGray UMETA(DisplayName = "Canvas Gray"),
	OrdnanceSurvey UMETA(DisplayName = "Ordnance Survey"),
	CollinsBart UMETA(DisplayName = "Collins Bart")
};

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CESIUM_API UCesiumBingMapsOverlay : public UCesiumRasterOverlay
{
	GENERATED_BODY()

	/**
	 * The ID of the Cesium ion asset to use. If this property is non-zero, the Bing Maps Key and Map Style properties are ignored
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString IonAccessToken;

	/**
	 * The Bing Maps API key to use. This property is ignored if the Ion Asset ID is non-zero.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString BingMapsKey;
	
	/**
	 * The map style to use. This property is ignored if the Ion Asset ID is non-zero.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	EBingMapsStyle MapStyle = EBingMapsStyle::Aerial;

	virtual void AddToTileset(Cesium3DTiles::Tileset& tileset) override;
};
