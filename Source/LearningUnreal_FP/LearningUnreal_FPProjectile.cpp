// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LearningUnreal_FPProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SphereComponent.h"
#include "Engine/EngineTypes.h"
#include "Components/MeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Math/TransformNonVectorized.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "TargetSignal.h"

class IProjectileState {
public:

protected:
	IProjectileState*	m_NextState;

public:
	IProjectileState(IProjectileState* nextState) {
		m_NextState = nextState;
	}

	virtual IProjectileState*		ShouldTransitionTo(ALearningUnreal_FPProjectile* self) = 0;
	virtual void		BeginState(ALearningUnreal_FPProjectile* self) = 0;
	virtual void		EndState(ALearningUnreal_FPProjectile* self) = 0;
	virtual void		Tick(ALearningUnreal_FPProjectile* self, float deltaTime) = 0;
};

class ProjectileState_Launch : public IProjectileState {
protected:
	float		m_LaunchTime = 0.0f;

public:
	ProjectileState_Launch(IProjectileState* nextState, float launchDelay) : IProjectileState(nextState) {
		m_LaunchTime = launchDelay;
	}

	virtual IProjectileState*		ShouldTransitionTo(ALearningUnreal_FPProjectile* self) override {
		return m_LaunchTime <= 0.0f ? m_NextState : NULL;
	}

	virtual void		BeginState(ALearningUnreal_FPProjectile* self) override {
		APlayerCameraManager *camManager = self->GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
		self->TargetDirection = camManager->GetCameraRotation().Vector();

		FRotator offsetRotation(FMath::RandRange(self->InitialPitchRange.X, self->InitialPitchRange.Y), FMath::RandRange(self->InitialYawRange.X, self->InitialYawRange.Y), 0.0f);
		FRotator newRotation = self->TargetDirection.ToOrientationRotator() + offsetRotation;
		self->SetActorRotation(newRotation);

		self->ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * self->ProjectileMovement->InitialSpeed);

		m_LaunchTime = self->GetLaunchDelay();
		self->ProjectileMovement->ProjectileGravityScale = 0.0f;
	}

	virtual void		EndState(ALearningUnreal_FPProjectile* self) override {
	}

	virtual void		Tick(ALearningUnreal_FPProjectile* self, float deltaTime) override {
		m_LaunchTime -= deltaTime;
	}
};

class ProjectileState_Fly : public IProjectileState {
protected:
	FVector		m_StaticTargetLocation;
	float		m_NoiseInput;
	FVector		m_NoiseOffsets_WorldSpace;

public:	
	ProjectileState_Fly(IProjectileState* nextState) : IProjectileState(nextState) {
	}

	virtual IProjectileState*		ShouldTransitionTo(ALearningUnreal_FPProjectile* self) override {
		return NULL;
	}

	FVector	GetTargetLocation(ALearningUnreal_FPProjectile* self) {
		return self->ProjectileMovement->HomingTargetComponent != NULL ? self->ProjectileMovement->HomingTargetComponent->GetComponentLocation() : m_StaticTargetLocation;
	}

	virtual void		BeginState(ALearningUnreal_FPProjectile* self) {
		//self->ProjectileMovement->ProjectileGravityScale = 0.0f;

		/*FRotator offsetRotation(FMath::RandRange(self->InitialPitchRange.X, self->InitialPitchRange.Y), FMath::RandRange(self->InitialPitchRange.X, self->InitialYawRange.Y), 0.0f);
		FRotator newRotation = self->TargetDirection.ToOrientationRotator() + offsetRotation;
		self->SetActorRotation(newRotation);*/

		self->ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * self->ProjectileMovement->InitialSpeed);

		FVector traceTarget = self->StartPosition + self->TargetDirection * 20000.0f;

		FHitResult OutHit;
		FCollisionQueryParams CollisionParams;
		FCollisionObjectQueryParams EverythingButGizmos(FCollisionObjectQueryParams::AllObjects);
		EverythingButGizmos.RemoveObjectTypesToQuery(COLLISION_GIZMO);
		EverythingButGizmos.RemoveObjectTypesToQuery(ECC_WorldStatic);

		CollisionParams.AddIgnoredActor(self);

		if (self->GetWorld()->LineTraceSingleByObjectType(OutHit, self->GetActorLocation(), traceTarget, EverythingButGizmos, CollisionParams))
		{
			if (OutHit.bBlockingHit)
			{ 
				self->ProjectileMovement->HomingTargetComponent = OutHit.Component;
				auto owner = self->ProjectileMovement->HomingTargetComponent->GetOwner();
				auto signal = (UTargetSignal*)owner->GetComponentByClass(UTargetSignal::StaticClass());
				if (signal != NULL) {
					signal->Signal();
				}
			}
			else {
				m_StaticTargetLocation = OutHit.ImpactPoint;
			}
		}
		self->ProjectileMovement->bIsHomingProjectile = self->ProjectileMovement->HomingTargetComponent.IsValid();

		m_NoiseInput = self->GetGameTimeSinceCreation();

		UGameplayStatics::SpawnEmitterAttached(self->GetTrailFx(), Cast<USceneComponent>(self->GetComponentByClass(UMeshComponent::StaticClass())));

		m_NoiseOffsets_WorldSpace = FVector::ZeroVector;
	}

	virtual void		EndState(ALearningUnreal_FPProjectile* self) override {
	}

	FVector		ProjectPointOnLine(const FVector& pt, const FVector& dir, const FVector& lineStart) {
		float distAlongLine = FVector::DotProduct(pt - lineStart, dir);
		
		FVector ptOnLine = lineStart + dir * distAlongLine;

		return ptOnLine;
	}

	float DetermineNoiseScale(ALearningUnreal_FPProjectile* self) {
		if (self->ProjectileMovement->HomingTargetComponent != NULL) {
			float minDist = self->DistRangeToReduceNoise.X;
			float maxDist = self->DistRangeToReduceNoise.Y;

			FVector vecToTarget = GetTargetLocation(self) - self->GetActorLocation();
			float distToTarget = FMath::Clamp( vecToTarget.Size(), minDist, maxDist );
			return (distToTarget - minDist) / (maxDist - minDist);
		}
		return 1.0f;
	}

	virtual void		Tick(ALearningUnreal_FPProjectile* self, float deltaTime) override {
		m_NoiseInput += deltaTime * self->NoiseFrequency;
		float noiseInput = m_NoiseInput;
		float yFrac = FMath::PerlinNoise1D(noiseInput);
		float zFrac = FMath::PerlinNoise1D(noiseInput + 349.0f);
		
		float scale = DetermineNoiseScale(self);

		if (scale > 0.0f && self->NoiseGain.Size() > 0.0f) {
			FVector	offsets;
			offsets.X = 0.0f;
			offsets.Y = self->NoiseGain.X * scale * yFrac;
			offsets.Z = self->NoiseGain.Y * scale * zFrac;

			FVector actorLoc = self->GetActorLocation();

			FTransform	transform(self->GetActorRotation());
			FVector denoisedLocation = actorLoc - m_NoiseOffsets_WorldSpace;

			if (self->ProjectileMovement->HomingTargetComponent == NULL) {
				FRotator deltaRot = (self->TargetDirection.ToOrientationRotator() - self->GetActorRotation());
				self->SetActorRotation(self->GetActorRotation() + deltaRot * deltaTime * 15.0f);
				self->ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * self->ProjectileMovement->Velocity.Size());

				FVector ptOnLine = ProjectPointOnLine(denoisedLocation, self->TargetDirection, self->StartPosition);
				denoisedLocation = ptOnLine + (denoisedLocation - ptOnLine) * scale;
			}

			m_NoiseOffsets_WorldSpace = transform.TransformVector(offsets);
			self->SetActorLocation(denoisedLocation + m_NoiseOffsets_WorldSpace);
		}
		else {
			int dsf = 454;
		}
	}
};

void ALearningUnreal_FPProjectile::DestroyStates() {
	if (m_FlyState) {
		delete m_FlyState;
		m_FlyState = NULL;
	}

	if (m_LaunchState) {
		delete m_LaunchState;
		m_LaunchState = NULL;
	}
}

ALearningUnreal_FPProjectile::ALearningUnreal_FPProjectile(): InitialPitchRange(-45.0f, 45.0f), InitialYawRange(-45.0f, 45.0f)
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");

	CollisionComp->OnComponentHit.AddDynamic(this, &ALearningUnreal_FPProjectile::OnHit);

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;
	ProjectileMovement->bIsHomingProjectile = true;
	//ProjectileMovement->HomingAccelerationMagnitude = 200.f;

	PrimaryActorTick.bCanEverTick = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

ALearningUnreal_FPProjectile::~ALearningUnreal_FPProjectile() {
	DestroyStates();
}

void ALearningUnreal_FPProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && OtherActor != this && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
	}

	if (!OtherComp->IsSimulatingPhysics()) {
		FVector actorDir = ProjectileMovement->Velocity.GetSafeNormal();
		float dot = FVector::DotProduct(Hit.Normal, -actorDir);
		if (dot < 0.3f) {
			return;
		}
	}

	UGameplayStatics::SpawnEmitterAtLocation(this->GetWorld(), m_ImpactFx, Hit.Location, Hit.Normal.ToOrientationRotator());

	Destroy();
}

void ALearningUnreal_FPProjectile::AttmeptToChangeState(ALearningUnreal_FPProjectile* self) {
	if (!m_CurrentState) {
		return;
	}

	IProjectileState* nextState = m_CurrentState->ShouldTransitionTo(self);
	if (!nextState || nextState == m_CurrentState) {
		return;
	}

	ChangeState(self, nextState);
}

void ALearningUnreal_FPProjectile::ChangeState(ALearningUnreal_FPProjectile* self, IProjectileState* nextState) {
	if (m_CurrentState) {
		m_CurrentState->EndState(self);
	}

	m_CurrentState = nextState;

	if (m_CurrentState) {
		m_CurrentState->BeginState(self);
	}
}

void ALearningUnreal_FPProjectile::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* pc = GetWorld()->GetFirstPlayerController();
	if (!pc) {
		//Error but happens when accidently hitting play in blueprints editor
		return;
	}

	APlayerCameraManager* camManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	TargetDirection = camManager->GetCameraRotation().Vector();
	StartPosition = camManager->GetCameraLocation();//This should be barrel of gun but for now

	DestroyStates();
	m_FlyState = new ProjectileState_Fly(NULL);
	m_LaunchState = new ProjectileState_Launch(m_FlyState, m_LaunchDelay);
	ChangeState(this, m_LaunchState);
}

void ALearningUnreal_FPProjectile::Tick(float DeltaTime)
{
	lifeSpan += DeltaTime;
	if (lifeSpan >= InitialLifeSpan) {
		UGameplayStatics::SpawnEmitterAtLocation(this->GetWorld(), m_ImpactFx, GetActorLocation(), FVector::UpVector.ToOrientationRotator());
		return;
	}

	Super::Tick(DeltaTime);

	m_CurrentState->Tick(this, DeltaTime);
	AttmeptToChangeState(this);
}