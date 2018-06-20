// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DSCharacter.generated.h"

UCLASS(config=Game)
class ADSCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UDSLockArmComponent* CameraLockArm;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/* Target component for camera lock on system */
	class UDSTargetComponent* TargetComponent;

public:

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock On Camera")
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock On Camera")
	float BaseLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock On Camera")
	float LockonControlRotationRate;;

	/* Tolerance for a mouse movement to be considered an input to switch targets */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
	float TargetSwitchMouseDelta;

	/* Tolerance for a mouse movement to be considered an input to switch targets */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
	float TargetSwitchAnalogValue;

	/* True if player returned analog-stick to center after last target switch */
	bool bAnalogSettledSinceLastTargetSwitch;

	/* Cooldown time before another target switch can occur, used to make target switching more controllable */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
	float TargetSwitchMinDelaySeconds;

	/* Time that last target switch occurred */
	UPROPERTY(BlueprintReadOnly, Category = "Lock On Camera")
	float LastTargetSwitchTime;

	/* Tolerance for a mouse movement to be considered an input to break target lock */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
	float BreakLockMouseDelta;

	/* Time to wait after breaking lock with mouse movement before player gets control of the camera back.  Prevents over scrolling camera after breaking lock. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
	float BrokeLockAimingCooldown;

	/* Time that player broke camera lock at */
	float BrokeLockTime;

public:
	/* Constructor */
	ADSCharacter();

protected:	
	/* Handle player input events */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for forwards/backward input */
	void MoveForward(float Value);
	/** Called for side to side input */
	void MoveRight(float Value);

	/* Handle horizontal mouse input */
	void Turn(float Val);
	/* Handle vertical mouse input */
	void LookUp(float Val);

	/* Handle horizontal analog stick input */
	void TurnAtRate(float Rate);

	/* Handle vertical analog stick input */
	void LookUpAtRate(float Rate);


	/* Tick every frame */
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	
public:
	/** Returns CameraBoom subobject **/
	UFUNCTION(BlueprintCallable, Category = "Lock On Camera")
	FORCEINLINE class UDSLockArmComponent* GetCameraBoom() const { return CameraLockArm; }
	/** Returns FollowCamera subobject **/
	UFUNCTION(BlueprintCallable, Category = "Lock On Camera")
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	

};
