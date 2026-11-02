// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSoulsChanged, int32);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SLASH_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ReceiveDamage(float Damage);
	void UseStamina(float StaminaCost);

	float GetHealthPercent();
	float GetStaminaPercent();
	bool IsAlive();

	void AddGold(int32 AmountOfGold);
	void AddSouls(int32 NumberOfSouls);

	FORCEINLINE int32 GetGold() const { return Gold; }
	FORCEINLINE int32 GetSouls() const { return Souls; }
	FORCEINLINE float GetStamina() const { return Stamina; }
	FORCEINLINE float GetDodgeCost() const { return DodgeCost; }
	FORCEINLINE float GetStaminaRegenRate() const { return StaminaRegenRate; }

	/** HUD 订阅这几个委托来刷新界面 */
	FOnHealthChanged OnHealthChanged;
	FOnStaminaChanged OnStaminaChanged;
	FOnGoldChanged OnGoldChanged;
	FOnSoulsChanged OnSoulsChanged;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float Health;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float MaxHealth;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float Stamina;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float MaxStamina;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int32 Gold;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int32 Souls;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float DodgeCost;

	/** 每秒体力回复速度 */
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float StaminaRegenRate;
};
