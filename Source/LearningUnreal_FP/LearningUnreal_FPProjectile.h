// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningUnreal_FPProjectile.generated.h"

UCLASS(config=Game)
class ALearningUnreal_FPProjectile : public AActor
{
	GENERATED_BODY()

protected:
	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	class USphereComponent* CollisionComp;

	UPROPERTY(EditAnywhere, DisplayName="Launch Delay", BlueprintReadWrite)
	float		m_LaunchDelay = 0.5f;

	UPROPERTY(EditAnywhere, DisplayName = "Impact Fx", BlueprintReadWrite)
	class UParticleSystem*			m_ImpactFx;

	UPROPERTY(EditAnywhere, DisplayName = "Trail Fx", BlueprintReadWrite)
	class UParticleSystem* m_TrailFx;

	class ProjectileState_Fly*		m_FlyState;
	class ProjectileState_Launch*	m_LaunchState;
	class IProjectileState*			m_CurrentState;

	float lifeSpan = 0.0f;

public:
	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LinearAccelDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D	InitialPitchRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D	InitialYawRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Gain", Category = Noise)
	FVector2D	NoiseGain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Frequency", Category = Noise)
	float		NoiseFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise)
	FVector2D	DistRangeToReduceNoise;

	FVector		TargetDirection;
	FVector		StartPosition;

protected:
	void	DestroyStates();

	virtual void BeginPlay() override;

	void	AttmeptToChangeState(ALearningUnreal_FPProjectile* self);

	void	ChangeState(ALearningUnreal_FPProjectile* self, class IProjectileState* nextState);

public:
	ALearningUnreal_FPProjectile();
	virtual ~ALearningUnreal_FPProjectile();

	FORCEINLINE float		GetLaunchDelay() const {
		return m_LaunchDelay;
	}

	FORCEINLINE UParticleSystem* GetTrailFx() const {
		return m_TrailFx;
	}

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	FORCEINLINE class USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	virtual void Tick(float DeltaTime) override;
};

