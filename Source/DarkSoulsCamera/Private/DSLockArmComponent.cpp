// Fill out your copyright notice in the Description page of Project Settings.

#include "DSLockArmComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DSTargetComponent.h"
#include "GameFramework/Pawn.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, text)

UDSLockArmComponent::UDSLockArmComponent()
{
	MaxTargetLockDistance = 750.f;
	bDrawDebug = true;

	TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	bUsePawnControlRotation = true; // Rotate the arm based on the controller
	bEnableCameraLag = true;
	bEnableCameraRotationLag = false;
	CameraLagSpeed = 3.f;
	CameraRotationLagSpeed = 2.f;
	CameraLagMaxDistance = 100.f;
}

void UDSLockArmComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsCameraLockedToTarget())
	{		
		DrawDebugSphere(GetWorld(), CameraTarget->GetComponentLocation(), 20.f, 16, FColor::Red); //Draw target point

		// Break lock if player is too far from target
		if ((CameraTarget->GetComponentLocation() - GetComponentLocation()).Size() > MaxTargetLockDistance + CameraTarget->GetScaledSphereRadius())
		{
			if (bUseSoftLock)
			{
				// Try switch to a new target in range
				if (UDSTargetComponent* NewCameraTarget = GetLockTarget())
					LockToTarget(NewCameraTarget);
				else
					BreakTargetLock();
			}
			else 
			{ 
				BreakTargetLock(); 
			}
		}
	}
	else
	{		
		if (bUseSoftLock) // Attempt to auto target nearby enemy
		{
			if (UDSTargetComponent* NewCameraTarget = GetLockTarget())
			{
				if (!bSoftlockRequiresReset) // Soft-lock is reset?
					LockToTarget(NewCameraTarget);
			}
			else // If player forcibly broke soft-lock, reset it when no target is within range
			{
				bSoftlockRequiresReset = false;
			}
		}
	}

	// Draw debug
	if (bDrawDebug)
	{
		for (UDSTargetComponent* Target : GetTargetComponents())
		{
			DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), Target->GetComponentLocation(), FColor::Green);
		}

		// Draw target inclusion sphere
		DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(), MaxTargetLockDistance, 32, FColor(0.f, 0.f, 0.f, 0.0f));

		UKismetSystemLibrary::DrawDebugString(this, FVector::ZeroVector, bUseSoftLock ? "Soft-lock Enabled" : "Soft-lock Disabled", GetOwner(), FLinearColor::Green);
		
		if (bSoftlockRequiresReset)
			UKismetSystemLibrary::DrawDebugString(this, FVector(0.f, 0.f, -10.f), "Soft-lock Requires Reset", GetOwner(), FLinearColor::Green);
	}
}

void UDSLockArmComponent::ToggleCameraLock()
{
	if (bUseSoftLock)   // Soft-lock supersedes player input
	{
		bSoftlockRequiresReset = false;
		return;
	}

	// If CameraTarget is set, unset it
	if (IsCameraLockedToTarget())
	{
		BreakTargetLock();
		return;
	}

	UDSTargetComponent* NewCameraTarget = GetLockTarget();

	if(NewCameraTarget != nullptr)
	{
		print(TEXT("Testing"));
		LockToTarget(NewCameraTarget);
	}
}

void UDSLockArmComponent::ToggleSoftLock()
{
	bUseSoftLock = !bUseSoftLock;

	if (bUseSoftLock)
	{
		print(TEXT("Soft-lock enabled"));
		bSoftlockRequiresReset = false;
	}
	else
	{
		BreakTargetLock();
		print(TEXT("Soft-lock disabled"));
	}
}

void UDSLockArmComponent::LockToTarget(UDSTargetComponent* NewTargetComponent)
{
	CameraTarget = NewTargetComponent;
	bEnableCameraRotationLag = true;
	//GetCharacterMovement()->bOrientRotationToMovement = false;
}

void UDSLockArmComponent::BreakTargetLock()
{
	if (IsCameraLockedToTarget())
	{
		CameraTarget = nullptr;
		//GetController()->SetControlRotation(FollowCamera->GetForwardVector().Rotation());
		bEnableCameraRotationLag = false;
		//GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

UDSTargetComponent* UDSLockArmComponent::GetLockTarget()
{
	TArray<UDSTargetComponent*> AvailableTargets = GetTargetComponents();
	if (AvailableTargets.Num() == 0)
		return nullptr;

	// Get the target with the smallest angle difference from the camera forward vector
	float ClosestDotToCenter = 0.f;
	UDSTargetComponent* TargetComponent = nullptr;

	for (int32 i = 0; i < AvailableTargets.Num(); i++)
	{
		float Dot = FVector::DotProduct(GetForwardVector(), (AvailableTargets[i]->GetComponentLocation() - GetComponentLocation()).GetSafeNormal());
		if (Dot > ClosestDotToCenter)
		{
			ClosestDotToCenter = Dot;
			TargetComponent = AvailableTargets[i];
		}
	}
	return TargetComponent;
}

void UDSLockArmComponent::SwitchTarget(EDirection SwitchDirection)
{
	if (!IsCameraLockedToTarget()) return;

	TArray<UDSTargetComponent*> AvailableTargets = GetTargetComponents();	// Get targets within lock-on range	
	if (AvailableTargets.Num() < 2) return;	// Must have an existing camera target and 1 additional target

	FVector CurrentTargetDir = (CameraTarget->GetComponentLocation() - GetComponentLocation()).GetSafeNormal();

	TArray<UDSTargetComponent*> ViableTargets;

	for (UDSTargetComponent* Target : AvailableTargets)
	{
		//  Don't consider current target as a switch target
		if (Target == CameraTarget) continue;

		FVector TargetDir = (Target->GetComponentLocation() - GetComponentLocation()).GetSafeNormal();
		FVector Cross = FVector::CrossProduct(CurrentTargetDir, TargetDir);

		if ((SwitchDirection == EDirection::Left && Cross.Z < 0.f)	// Negative Z indicates left
			|| (SwitchDirection == EDirection::Right && Cross.Z > 0.f))	// Positive Z indicates right
		{
			ViableTargets.AddUnique(Target);
		}
	}

	if (ViableTargets.Num() == 0) return;

	/*
	Select the target with the smallest angle difference to the current target
	*/
	int32 BestDotIdx = 0;
	for (int32 i = 1; i < ViableTargets.Num(); i++)
	{
		float BestDot = FVector::DotProduct(CurrentTargetDir, (ViableTargets[BestDotIdx]->GetComponentLocation() - GetComponentLocation()).GetSafeNormal());
		float TestDot = FVector::DotProduct(CurrentTargetDir, (ViableTargets[i]->GetComponentLocation() - GetComponentLocation()).GetSafeNormal());

		// Higher dot product indicates this target vector has a smaller angle than the previous best
		if (TestDot > BestDot)
			BestDotIdx = i;
	}

	LockToTarget(ViableTargets[BestDotIdx]);
}

TArray<UDSTargetComponent*> UDSLockArmComponent::GetTargetComponents()
{
	TArray<UPrimitiveComponent*> TargetPrims;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { EObjectTypeQuery::ObjectTypeQuery2 }; // World dynamic object type

	// Overlap check for targetable component
	UKismetSystemLibrary::SphereOverlapComponents(GetOwner(),GetComponentLocation(),MaxTargetLockDistance, ObjectTypes,UDSTargetComponent::StaticClass(),TArray<AActor*>{GetOwner()},TargetPrims);

	TArray<UDSTargetComponent*> TargetComps;
	for (UPrimitiveComponent* Comp : TargetPrims)
	{
		TargetComps.Add(Cast<UDSTargetComponent>(Comp));
	}

	return TargetComps;
}

bool UDSLockArmComponent::IsCameraLockedToTarget()
{
	return CameraTarget != nullptr;
}
