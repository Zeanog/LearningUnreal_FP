// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LearningUnreal_FPGameMode.h"
#include "LearningUnreal_FPHUD.h"
#include "LearningUnreal_FPCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALearningUnreal_FPGameMode::ALearningUnreal_FPGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ALearningUnreal_FPHUD::StaticClass();
}
