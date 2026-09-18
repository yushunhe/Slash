// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SlashAnimInstance.h"
#include "Characters/SlashCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USlashAnimInstance::NativeInitializeAnimation()
{
	// Call the base class implementation
	Super::NativeInitializeAnimation();
	// Add your custom animation initialization logic here
	// For example, you can initialize animation variables or set up references to the owning character
	SlashCharacter = Cast<ASlashCharacter>(TryGetPawnOwner());
	if (SlashCharacter)
	{
		SlashCharacterMovement = SlashCharacter->GetCharacterMovement();
	}
	
}

void USlashAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (SlashCharacterMovement)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(SlashCharacterMovement->Velocity);
		IsFalling = SlashCharacterMovement->IsFalling();
	}

}
