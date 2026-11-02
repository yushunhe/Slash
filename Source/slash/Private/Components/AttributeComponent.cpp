// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 默认数值：否则没在蓝图里填值时 Health/MaxHealth 都是 0，
	// IsAlive() 永远为 false，受击反应、血条都不会工作（还会除零）
	Health = 100.f;
	MaxHealth = 100.f;

	Stamina = 100.f;
	MaxStamina = 100.f;
	StaminaRegenRate = 10.f;
	DodgeCost = 14.f;

	Gold = 0;
	Souls = 0;
}


void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();


}

void UAttributeComponent::ReceiveDamage(float Damage)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(GetHealthPercent());
}

void UAttributeComponent::UseStamina(float StaminaCost)
{
	Stamina = FMath::Clamp(Stamina - StaminaCost, 0.0f, MaxStamina);
	OnStaminaChanged.Broadcast(GetStaminaPercent());
}

float UAttributeComponent::GetHealthPercent()
{
	if (MaxHealth <= 0.f) return 0.f;
	return Health / MaxHealth;
}

float UAttributeComponent::GetStaminaPercent()
{
	if (MaxStamina <= 0.f) return 0.f;
	return Stamina / MaxStamina;
}

bool UAttributeComponent::IsAlive()
{
	return Health > 0.0f;
}

void UAttributeComponent::AddGold(int32 AmountOfGold)
{
	Gold += AmountOfGold;
	OnGoldChanged.Broadcast(Gold);
}

void UAttributeComponent::AddSouls(int32 NumberOfSouls)
{
	Souls += NumberOfSouls;
	OnSoulsChanged.Broadcast(Souls);
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 体力自动回复，回复过程中持续通知 HUD
	const float PreviousStamina = Stamina;
	Stamina = FMath::FInterpTo(Stamina, MaxStamina, DeltaTime, StaminaRegenRate);
	if (!FMath::IsNearlyEqual(PreviousStamina, Stamina))
	{
		OnStaminaChanged.Broadcast(GetStaminaPercent());
	}
}
