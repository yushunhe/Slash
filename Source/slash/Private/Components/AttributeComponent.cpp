// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 默认血量：否则没在蓝图里填值时 Health/MaxHealth 都是 0，
	// IsAlive() 永远为 false，受击反应、血条都不会工作（还会除零）
	Health = 100.f;
	MaxHealth = 100.f;
}


void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UAttributeComponent::ReceiveDamage(float Damage)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
}

float UAttributeComponent::GetHealthPercent()
{
	if (MaxHealth <= 0.f) return 0.f;
	return Health / MaxHealth;
}

bool UAttributeComponent::IsAlive()
{
	return Health > 0.0f;
}


void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

