// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetSignal.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEARNINGUNREAL_FP_API UTargetSignal : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, DisplayName = "Frequency", BlueprintReadWrite)
	float							m_Frequency = 1.0f;

	class UMaterialInstanceDynamic* m_Material;

	UPROPERTY(EditAnywhere, DisplayName = "Duration", BlueprintReadWrite)
	float							m_SignalDuration = 1.0f;
	float							m_SignalTime;

	float							m_ScaledSignalTime;

public:	
	// Sets default values for this component's properties
	UTargetSignal();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void		StartFlash() {
		m_SignalTime = m_SignalDuration;
	}

	void		StopFlash() {
		m_SignalTime = 0.0f;
	}

public:	
	void		Signal();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
