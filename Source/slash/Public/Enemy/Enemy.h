// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"

class UAnimMontage;
class USoundBase;
class UParticleSystem;
class UAttributeComponent;
class UWidgetComponent;
class UHealthBarComponent;
class UPawnSensingComponent;

UCLASS()
class SLASH_API AEnemy : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	AEnemy();
	virtual void Tick(float DeltaTime) override;
	void CheckPatrolTarget();
	void CheckCombatTarget();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	void DirectionalHitReact(const FVector& ImpactPoint);
	void DirectionalDie(const FVector& ImpactPoint);
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
private:

	UPROPERTY(VisibleAnywhere)
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere)
	UHealthBarComponent* HealthBarWidget;

	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	/**
* Animation Montages
*/
	UPROPERTY(EditDefaultsOnly, Category = Montages)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = Montages)
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, Category = Sounds)
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = VisualEffects)
	UParticleSystem* HitParticles;

	UPROPERTY()
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, Category = Combat)
	double CombatRadius = 1500.f;

	/**
	* Navigation
	*/

	UPROPERTY()
	class AAIController* EnemyController;

	//Currennt patrol target
	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	AActor* PatrolTarget;

	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	TArray<AActor*> PatrolTargets;

	//到达判定的容差，要明显大于 MoveTo 的接受半径
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	double PatrolRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	double AttackRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	double MoveAcceptanceRadius = 15.f;

	FTimerHandle PatrolTimer;
	void PatrolTimerFinished();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Die(const FName& SectionName);
	bool InTargetRange(AActor* Target, double Radius);
	/**
* Play montage function
*/
	void PlayHitReactMontage(const FName& SectionName);

	UPROPERTY(BlueprintReadOnly)
	EDeathPose DeathPose = EDeathPose::EDP_Alive;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMin = 4.f;
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMax = 6.f;

	EEnemyState EnemyState = EEnemyState::EES_Patrolling;
protected:
	void MoveToTarget(AActor* Target);
	AActor* ChoosePatrolTarget();

	UFUNCTION()
	void PawnSeen(APawn* SeenPawn);
};
