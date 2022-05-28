// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbSystem1Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AClimbSystem1Character

AClimbSystem1Character::AClimbSystem1Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AClimbSystem1Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AClimbSystem1Character::CustomJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AClimbSystem1Character::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AClimbSystem1Character::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AClimbSystem1Character::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AClimbSystem1Character::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AClimbSystem1Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AClimbSystem1Character::TouchStopped);
}

void AClimbSystem1Character::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	//Jump();
	CustomJump ();
}

void AClimbSystem1Character::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AClimbSystem1Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AClimbSystem1Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AClimbSystem1Character::CustomJump()
{
	if (bIsClimbing)
	{
		bIsClimbing = bIsHeadHit = bIsPelvisHit = false;
		StopJumping();
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		GetCharacterMovement()->bOrientRotationToMovement = true;
		BackToGroundFromClimb ();
		return;
	}
	Jump();
	TraceFromHeadAndPelvis(FVector(0,0,0));
	if (bIsHeadHit)
	{
		FTimerHandle TempTimerHanlde;
		GetWorldTimerManager().SetTimer(TempTimerHanlde, this, &AClimbSystem1Character::InitClimb, 0.1f);
	}
}

void AClimbSystem1Character::TraceFromHeadAndPelvis(FVector Offset)
{
	FVector Start;
	FVector End;
	FHitResult Hit;
	FCollisionQueryParams CollisionQueryParam;
	// Commented the pelvisSocket based location. Due to up wall movement. As the difference between pelvis and actor location differs
	Start = GetActorLocation()/*GetMesh()->GetSocketLocation(TEXT("pelvisSocket"))*/ + Offset;	
	End = Start + (GetActorForwardVector() * 200);
	bIsPelvisHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionQueryParam);
	if (bIsPelvisHit)
	{
		PelvisHit = Hit;
		//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);
		Start = GetMesh()->GetSocketLocation(TEXT("headSocket")) + Offset;
		End = Start + (GetActorForwardVector() * 200);
		bIsHeadHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionQueryParam);
		HeadHitLocation = Hit.Location;
		HeadHitNormal = Hit.Normal;
		if (bIsHeadHit)
		{
			HeadHit = Hit;
			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);			
		}
		else
		{
			FVector Location = Start + Offset;
			Location.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			MoveToTopEdge(Location);
		}
	}
}

void AClimbSystem1Character::InitClimb()
{
	bIsClimbing = true;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	UE_LOG(LogTemp, Warning, TEXT("HeadHitLocation : %s , Normal : %s"), *HeadHitLocation.ToString(), *HeadHitNormal.ToString());
	FVector TargetRelativeLoc = HeadHitLocation + (HeadHitNormal * WallDistance);
	FRotator TargetRelativeRot = UKismetMathLibrary::MakeRotFromX(HeadHitNormal * -1.0f);
	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), TargetRelativeLoc, TargetRelativeRot, false, false, 0.4f, false, EMoveComponentAction::Type::Move, LatentInfo);
}

void AClimbSystem1Character::MoveForward(float Value)
{
	if(bIsClimbing)
	{
		if (Value != 0)
			ClimbMovement(Value, GetActorUpVector());
		return;
	}
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AClimbSystem1Character::MoveRight(float Value)
{
	if (bIsClimbing)
	{
		if(Value != 0)
			ClimbMovement(Value, GetActorRightVector());
		return;
	}
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
