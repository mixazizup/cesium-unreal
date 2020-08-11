// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumGeoreferenceable.h"
#include "UObject/WeakInterfacePtr.h"
#include "CesiumGeoreference.generated.h"

UENUM(BlueprintType)
enum class EOriginPlacement : uint8 {
	/**
	 * Use the tileset's true origin as the Actor's origin. For georeferenced
	 * tilesets, this usually means the Actor's origin will be at the center
	 * of the Earth.
	 */
	TrueOrigin UMETA(DisplayName = "True origin"),

	/*
	 * Use the center of the tileset's bounding volume as the Actor's origin. This option
	 * preserves precision by keeping all tileset vertices as close to the Actor's origin
	 * as possible.
	 */
	BoundingVolumeOrigin UMETA(DisplayName = "Bounding volume center"),

	/**
	 * Use a custom position within the tileset as the Actor's origin. The position is
	 * expressed as a longitude, latitude, and height, and that position within the tileset
	 * will be at coordinate (0,0,0) in the Actor's coordinate system.
	 */
	CartographicOrigin UMETA(DisplayName = "Longitude / latitude / height")
};

/**
 * Controls how global geospatial coordinates are mapped to coordinates in the Unreal Engine level.
 */
UCLASS()
class CESIUM_API ACesiumGeoreference : public AActor
{
	GENERATED_BODY()

public:
	static ACesiumGeoreference* GetDefaultForActor(AActor* Actor);

	ACesiumGeoreference();

	/**
	 * The placement of this Actor's origin (coordinate 0,0,0) within the tileset. 3D Tiles tilesets often
	 * use Earth-centered, Earth-fixed coordinates, such that the tileset content is in a small bounding
	 * volume 6-7 million meters (the radius of the Earth) away from the coordinate system origin.
	 * This property allows an alternative position, other then the tileset's true origin, to be treated
	 * as the origin for the purpose of this Actor. Using this property will preserve vertex precision
	 * (and thus avoid jittering) much better precision than setting the Actor's Transform property.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	EOriginPlacement OriginPlacement = EOriginPlacement::BoundingVolumeOrigin;

	/**
	 * The longitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLongitude = 0.0;

	/**
	 * The latitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLatitude = 0.0;

	/**
	 * The height of the custom origin placement in meters above the WGS84 ellipsoid.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginHeight = 0.0;

	/**
	 * If true, the tileset is rotated so that the local up at the center of the tileset's bounding
	 * volume is aligned with the usual Unreal Engine up direction, +Z. This is useful because
	 * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates in which the local
	 * up direction depends on where you are on the Earth. If false, the tileset's true rotation
	 * is used.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin || OriginPlacement==EOriginPlacement::BoundingVolumeOrigin"))
	bool AlignTilesetUpWithZ = true;

	void AddGeoreferencedObject(ICesiumGeoreferenceable* Object);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	TArray<TWeakInterfacePtr<ICesiumGeoreferenceable>> _georeferencedObjects;
};
