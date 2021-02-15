#include "CesiumGeoreferenceComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "CesiumTransforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Cartographic.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_transform.hpp>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() :
	_worldOriginLocation(0.0),
	_absoluteLocation(0.0),
	_relativeLocation(0.0),
	_actorToECEF(),
	_actorToUnrealRelativeWorld(),
	_ownerRoot(nullptr),
	_ignoreOnUpdateTransform(false)
{
	this->bAutoActivate = true;
	this->bWantsOnUpdateTransform = true;
	PrimaryComponentTick.bCanEverTick = false;
	// TODO: double check if we can attach to ownerRoot here instead of OnRegister
}

void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {
	// TODO: do we need the double precision calculations here when talking about only rotation?

	// local up in ECEF (the +Z axis)
	glm::dvec3 actorUpECEF = glm::normalize(this->_actorToECEF[2]);

	// the surface normal of the ellipsoid model of the globe at the ECEF location of the actor
	glm::dvec3 ellipsoidNormal = CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(this->_actorToECEF[3]);
	
	// cosine of the angle between the actor's up direction and the ellipsoid normal
	double cos = glm::dot(actorUpECEF, ellipsoidNormal);

	// TODO: use an appropriate Cesium epsilon
	if (cos < -0.999) {
		// The actor's current up direction is completely upside down with respect to the ellipsoid normal.
		
		// We want to do a 180 degree rotation around X. We can do this by flipping the Y and Z axes 
		this->_actorToECEF[1] *= -1.0;
		this->_actorToECEF[2] *= -1.0;

	} else {
		// the axis of the shortest available rotation with a magnitude that is sine of the angle
		glm::dvec3 sin_axis = glm::cross(ellipsoidNormal, actorUpECEF);
		
		// We construct a rotation matrix using Rodrigues' rotation formula for rotating by theta around an axis. Note that we can skip out on all of the trigonometric
		// calculations due to the fact we had vectors representing the before/after of the rotation; we were able to compute cos and sin * axis directly from those.		
		
		// K is the cross product matrix of the axis, i.e. K v = axis x v, where v is any vector.
		// Here we have a factor of sine theta that we let through as well since it will simplify the calcuations in Rodrigues` formula.
		glm::dmat3 sin_K(
			0.0, -sin_axis.z, sin_axis.y,
			sin_axis.z, 0.0, -sin_axis.x,
			-sin_axis.y, sin_axis.x, 0.0 
		);
		// Rodrigues' rotation formula
		glm::dmat4 R = glm::dmat3(1.0) + sin_K + sin_K * sin_K / (1.0 + cos);

		// we only want to apply the rotation to the local axes, not the translation
		this->_actorToECEF[0] = R * this->_actorToECEF[0];
		this->_actorToECEF[1] = R * this->_actorToECEF[1];
		this->_actorToECEF[2] = R * this->_actorToECEF[2];
	}

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

// TODO: are single-precision long/lat/height as accurate as double-precision ECEF?
void UCesiumGeoreferenceComponent::MoveToLongLatHeight() {
	glm::dvec3 ecef = CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
		CesiumGeospatial::Cartographic::fromDegrees(this->Longitude, this->Latitude, this->Height)
	);
	this->SetAccurateECEF(ecef.x, ecef.y, ecef.z);
}

void UCesiumGeoreferenceComponent::MoveToECEF() {
	this->SetAccurateECEF(this->ECEF_X, this->ECEF_Y, this->ECEF_Z);
}

void UCesiumGeoreferenceComponent::SetAccurateECEF(double ecef_x, double ecef_y, double ecef_z) {
	this->_actorToECEF[3] = glm::vec4(ecef_x, ecef_y, ecef_z, 1.0);

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);

	this->_relativeLocation = this->_actorToUnrealRelativeWorld[3];
	this->_absoluteLocation = this->_relativeLocation + this->_worldOriginLocation;
}

// TODO: is this really the best place to do this? Maybe do this in the constructor?
// TODO: should we account for the case where this component gets moved to another actor?
void UCesiumGeoreferenceComponent::OnRegister() {
	Super::OnRegister();

	AActor* owner = this->GetOwner();
	this->_ownerRoot = owner->GetRootComponent();
	this->AttachToComponent(this->_ownerRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);

	this->_updateAbsoluteLocation();
	this->_updateRelativeLocation();
	this->_initGeoreference();
}

// TODO: this problem probably also exists in TilesetRoot, but if an origin rebase has happened before placing the actor (or tileset) the internal 
// world origin might be incorrect?

// TODO: maybe this calculation can happen solely on the Georeference actor who can maintain a double precision consensus of the worldOrigin
void UCesiumGeoreferenceComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) {
	USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

	const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
	this->_worldOriginLocation = glm::dvec3(
		static_cast<double>(oldOrigin.X) - static_cast<double>(InOffset.X),
		static_cast<double>(oldOrigin.Y) - static_cast<double>(InOffset.Y),
		static_cast<double>(oldOrigin.Z) - static_cast<double>(InOffset.Z)
	);

	// Do _not_ call _updateAbsoluteLocation. The absolute position doesn't change with
	// an origin rebase, and we'll lose precision if we update the absolute location here.

	this->_updateRelativeLocation();
	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

// TODO: maybe implement moveComponentImpl to tell transform to ignore, or to pass on to rootcomponent

void UCesiumGeoreferenceComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) {
	USceneComponent::OnUpdateTransform(UpdateTransformFlags, Teleport);
	
	// TODO: is this safe / reliable? Will it skip the wrong transform callback?
	// Should check if this is being called in the same thread sequentially after SetRelativeTransformation()
	// if we generated this transform call internally, we should ignore it
	if (_ignoreOnUpdateTransform) {
		_ignoreOnUpdateTransform = false;
		return;
	}
	
	this->_updateAbsoluteLocation();
	this->_updateRelativeLocation();
	this->_updateActorToECEF();
	this->_updateActorToUnrealRelativeWorldTransform();
}

void UCesiumGeoreferenceComponent::BeginPlay() {
	Super::BeginPlay();
}

void UCesiumGeoreferenceComponent::Activate(bool bReset) {

}

void UCesiumGeoreferenceComponent::Deactivate() {

}

void UCesiumGeoreferenceComponent::OnComponentDestroyed(bool bDestroyingHierarchy) {
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	// TODO: remove from georeferenced list ?
}

bool UCesiumGeoreferenceComponent::IsBoundingVolumeReady() const {
	return false;
}

std::optional<Cesium3DTiles::BoundingVolume> UCesiumGeoreferenceComponent::GetBoundingVolume() const {
	return std::nullopt;
}

void UCesiumGeoreferenceComponent::UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::_updateAbsoluteLocation() {
	const FVector& relativeLocation = this->_ownerRoot->GetComponentLocation();
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	this->_absoluteLocation = glm::dvec3(
		static_cast<double>(originLocation.X) + static_cast<double>(relativeLocation.X),
		static_cast<double>(originLocation.Y) + static_cast<double>(relativeLocation.Y),
		static_cast<double>(originLocation.Z) + static_cast<double>(relativeLocation.Z)
	);
}

void UCesiumGeoreferenceComponent::_updateRelativeLocation() {
	// Note: We are tracking this instead of using the floating-point UE relative world location, since this will be more accurate.
	// This means that while the rendering, physics, and anything else on the UE side might be lossy, our internal representation of the location will remain accurate.
	this->_relativeLocation = this->_absoluteLocation - this->_worldOriginLocation;
}

void UCesiumGeoreferenceComponent::_initGeoreference() {
	this->Georeference = ACesiumGeoreference::GetDefaultForActor(this->GetOwner());
	if (this->Georeference) {
		this->_updateActorToECEF(); 
		this->Georeference->AddGeoreferencedObject(this);
	}
	// Note: when a georeferenced object is added, UpdateGeoreferenceTransform will automatically be called
}

void UCesiumGeoreferenceComponent::_updateActorToECEF() {
	if (!this->Georeference) {
		return;
	}
	// TODO: we should avoid duplicate computation from Georeferenced -> ECEF vs ECEF -> Georeferenced, they are computed identically plus inverting in Georeference
	glm::dmat4 georeferencedToEllipsoidCenteredTransform = this->Georeference->GetGeoreferencedToEllipsoidCenteredTransform();

	FMatrix actorToRelativeWorld = this->_ownerRoot->GetComponentToWorld().ToMatrixWithScale();

	glm::dmat4 actorToAbsoluteWorld(
		glm::dvec4(actorToRelativeWorld.M[0][0], actorToRelativeWorld.M[0][1], actorToRelativeWorld.M[0][2], actorToRelativeWorld.M[0][3]),
		glm::dvec4(actorToRelativeWorld.M[1][0], actorToRelativeWorld.M[1][1], actorToRelativeWorld.M[1][2], actorToRelativeWorld.M[1][3]),
		glm::dvec4(actorToRelativeWorld.M[2][0], actorToRelativeWorld.M[2][1], actorToRelativeWorld.M[2][2], actorToRelativeWorld.M[2][3]),
		glm::dvec4(this->_absoluteLocation, 1.0)
	);

	this->_actorToECEF = georeferencedToEllipsoidCenteredTransform * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium * actorToAbsoluteWorld;
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform() {
	if (!this->Georeference) {
		return;
	}
	// TODO: check if this can be precomputed on the Georeference side
	glm::dmat4 ellipsoidCenteredToGeoreferencedTransform = this->Georeference->GetEllipsoidCenteredToGeoreferencedTransform();
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
	glm::dmat4 absoluteToRelativeWorld(
		glm::dvec4(1.0, 0.0, 0.0, 0.0),
		glm::dvec4(0.0, 1.0, 0.0, 0.0),
		glm::dvec4(0.0, 0.0, 1.0, 0.0),
		glm::dvec4(-this->_worldOriginLocation, 1.0)
	);

	this->_actorToUnrealRelativeWorld = absoluteToRelativeWorld * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferencedTransform * this->_actorToECEF;
}

void UCesiumGeoreferenceComponent::_setTransform(const glm::dmat4& transform) {
	// we are about to get an OnUpdateTransform callback for this, so we preemptively mark down to ignore it
	_ignoreOnUpdateTransform = true;

	this->_ownerRoot->SetRelativeTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));
}
