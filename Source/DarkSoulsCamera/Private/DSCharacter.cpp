// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DSCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DSTargetComponent.h"
#include "DSLockArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ADSCharacter

ADSCharacter::ADSCharacter()
{
	LockonControlRotationRate = 10.f;
	TargetSwitchMouseDelta = 3.f;
	TargetSwitchAnalogValue = .8f;
	TargetSwitchMinDelaySeconds = .5f;
	BreakLockMouseDelta = 10.f;
	BrokeLockAimingCooldown = .5f;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraLockArm = CreateDefaultSubobject<UDSLockArmComponent>(TEXT("CameraLockArm"));
	CameraLockArm->SetupAttachment(RootComponent);
	CameraLockArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraLockArm, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create target component
	TargetComponent = CreateDefaultSubobject<UDSTargetComponent>(TEXT("TargetComponent"));
	TargetComponent->SetupAttachment(GetRootComponent());
}

//////////////////////////////////////////////////////////////////////////
// Input
void ADSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings

	/* Assert PlayerInputComponent exists */
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADSCharacter::MoveRight);

	// Handle mouse aiming input
	PlayerInputComponent->BindAxis("Turn", this, &ADSCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ADSCharacter::LookUp);

	// Handle analog stick aiming input
	PlayerInputComponent->BindAxis("TurnRate", this, &ADSCharacter::TurnAtRate);	
	PlayerInputComponent->BindAxis("LookUpRate", this, &ADSCharacter::LookUpAtRate);

	// Action inputs
	PlayerInputComponent->BindAction("ToggleCameraLock", IE_Pressed, CameraLockArm, &UDSLockArmComponent::ToggleCameraLock);
	PlayerInputComponent->BindAction("ToggleSoftLock", IE_Pressed, CameraLockArm, &UDSLockArmComponent::ToggleSoftLock);
}

void ADSCharacter::MoveForward(float Val)
{
	if ((Controller != NULL) && (Val != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = CameraLockArm->CameraTarget == nullptr ? Controller->GetControlRotation() : (CameraLockArm->CameraTarget->GetOwner()->GetActorLocation() - GetActorLocation()).GetSafeNormal().Rotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Val);
	}
}

void ADSCharacter::MoveRight(float Val)
{
	if ((Controller != NULL) && (Val != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = CameraLockArm->CameraTarget == nullptr ? Controller->GetControlRotation() : (CameraLockArm->CameraTarget->GetOwner()->GetActorLocation() - GetActorLocation()).GetSafeNormal().Rotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}

void ADSCharacter::Turn(float Val)
{
	float TimeSinceLastTargetSwitch = GetWorld()->GetRealTimeSeconds() - LastTargetSwitchTime;

	if (CameraLockArm->IsCameraLockedToTarget())
	{
		// Should break soft-lock?
		if (CameraLockArm->bUseSoftLock && FMath::Abs(Val) > BreakLockMouseDelta)
		{
			CameraLockArm->BreakTargetLock();
			BrokeLockTime = GetWorld()->GetRealTimeSeconds();
			CameraLockArm->bSoftlockRequiresReset = true;
		}
		// Should try switch target?
		else if(FMath::Abs(Val) > TargetSwitchMouseDelta 
			&& TimeSinceLastTargetSwitch > TargetSwitchMinDelaySeconds)	// Prevent switching multiple times using a single movement
		{
			if (Val < 0)
				CameraLockArm->SwitchTarget(EDirection::Left);
			else
				CameraLockArm->SwitchTarget(EDirection::Right);

			LastTargetSwitchTime = GetWorld()->GetRealTimeSeconds();
		}
	}
	else
	{
		// If camera lock was recently broken by a large mouse delta, allow a cooldown time to prevent erratic camera movement
		bool bRecentlyBrokeLock = (GetWorld()->GetRealTimeSeconds() - BrokeLockTime) < BrokeLockAimingCooldown;	
		if(!bRecentlyBrokeLock)
			AddControllerYawInput(Val);	
	}	
}

void ADSCharacter::LookUp(float Val)
{
	if (!CameraLockArm->IsCameraLockedToTarget())
		AddControllerPitchInput(Val);
}

void ADSCharacter::TurnAtRate(float Val)
{	
	// Ensure the analog stick returned to neutral since last target switch attempt
	if (FMath::Abs(Val) < .1f)
		bAnalogSettledSinceLastTargetSwitch = true;

	if (CameraLockArm->IsCameraLockedToTarget() && (FMath::Abs(Val) > TargetSwitchAnalogValue) && bAnalogSettledSinceLastTargetSwitch)
	{
		if (Val < 0)
			CameraLockArm->SwitchTarget(EDirection::Left);
		else
			CameraLockArm->SwitchTarget(EDirection::Right);

		bAnalogSettledSinceLastTargetSwitch = false;
	}
	else
	{
		// calculate delta for this frame from the rate information
		AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void ADSCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	if (!CameraLockArm->IsCameraLockedToTarget())
		AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


void ADSCharacter::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (CameraLockArm->IsCameraLockedToTarget())
	{
		// Vector from player to target
		FVector TargetVect = CameraLockArm->CameraTarget->GetComponentLocation() - CameraLockArm->GetComponentLocation();
		FRotator TargetRot = TargetVect.GetSafeNormal().Rotation();
		FRotator CurrentRot = GetControlRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, LockonControlRotationRate);

		// Update control rotation to face target
		GetController()->SetControlRotation(NewRot);
	}
}
