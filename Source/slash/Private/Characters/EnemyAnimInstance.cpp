// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyAnimInstance.h"
#include "Characters/BaseCharacter.h"

void UEnemyAnimInstance::AnimNotify_EnableBoxCollision()
{
	SetWeaponCollision(ECollisionEnabled::QueryOnly);
}

void UEnemyAnimInstance::AnimNotify_DisableBoxColliision()
{
	SetWeaponCollision(ECollisionEnabled::NoCollision);
}

void UEnemyAnimInstance::SetWeaponCollision(ECollisionEnabled::Type CollisionEnabled)
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(TryGetPawnOwner()))
	{
		Character->SetWeaponCollisionEnabled(CollisionEnabled);
	}
}
