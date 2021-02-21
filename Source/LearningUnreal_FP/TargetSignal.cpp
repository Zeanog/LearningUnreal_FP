// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetSignal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
UTargetSignal::UTargetSignal()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTargetSignal::BeginPlay()
{
	Super::BeginPlay();

	auto actorOwner = GetOwner();
	auto meshComp = (UStaticMeshComponent*)actorOwner->GetComponentByClass(UStaticMeshComponent::StaticClass());
	auto material = meshComp->GetMaterial(0);
	m_Material = UMaterialInstanceDynamic::Create(material, NULL);
	meshComp->SetMaterial(0, m_Material);

	m_SignalTime = 0.0f;
	m_ScaledSignalTime = 0.0f;
}

void UTargetSignal::Signal() {
	StartFlash();
}

void UTargetSignal::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	m_SignalTime = FMath::Max( m_SignalTime - DeltaTime, 0.0f );
	m_ScaledSignalTime = FMath::Max(m_SignalTime - DeltaTime * m_Frequency, 0.0f);
	if (m_SignalTime > 0.0f) {
		m_Material->SetScalarParameterValue(TEXT("Blend"), FMath::Sin(m_ScaledSignalTime));
	}
}

