// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "DSLockArmComponent.generated.h"

UENUM(BlueprintType)
enum class EDirection : uint8
{
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right"),
};

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class DARKSOULSCAMERA_API UDSLockArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:	
	/* Max Distance from the character for an actor to be targetable */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
		float MaxTargetLockDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
		bool bUseSoftLock;

	/* Turn debug visuals on/off */
	UPROPERTY(EditDefaultsOnly, Category = "Lock On Camera")
		bool bDrawDebug;

	/* True if lock was recently broken and mouse delta is still high */
	bool bSoftlockRequiresReset;

	/* The component the camera is currently locked on to */
	UPROPERTY(BlueprintReadOnly)
		class UDSTargetComponent* CameraTarget;

	UDSLockArmComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ToggleCameraLock();
	void ToggleSoftLock();
	void LockToTarget(UDSTargetComponent* NewTargetComponent);
	void BreakTargetLock();
	class UDSTargetComponent* GetLockTarget();
	void SwitchTarget(EDirection SwitchDirection);
	TArray<class UDSTargetComponent*> GetTargetComponents();

	/* True if the camera is currently locked to a target */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock On Camera")
		bool IsCameraLockedToTarget();
};
