// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbSystem1GameMode.h"
#include "ClimbSystem1Character.h"
#include "UObject/ConstructorHelpers.h"

AClimbSystem1GameMode::AClimbSystem1GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
