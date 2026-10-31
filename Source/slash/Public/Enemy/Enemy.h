// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"


class USoundBase;
class UParticleSystem;
class UWidgetComponent;
class UHealthBarComponent;
class UPawnSensingComponent;

UCLASS()
class SLASH_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemy();
	virtual void Tick(float DeltaTime) override;
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;

	void CheckPatrolTarget();
	void CheckCombatTarget();
	void DirectionalDie(const FVector& ImpactPoint);

protected:
	virtual void BeginPlay() override;
	virtual void Die(const FName& SectionName) override;
	virtual void Attack() override;
	virtual void PlayAttackMontage() override;
	virtual bool CanAttack() override;
	virtual void AttackEnd() override;
	virtual void HandleDamage(float DamageAmount) override;
	bool InTargetRange(AActor* Target, double Radius);

	UPROPERTY(BlueprintReadOnly)
	EDeathPose DeathPose = EDeathPose::EDP_Alive;

	UPROPERTY(BlueprintReadOnly)
	EEnemyState EnemyState = EEnemyState::EES_Patrolling;

private:
	/** AI Behavior */
	void InitializeEnemy();
	void HideHealthBar();
	void ShowHealthBar();
	void LoseInterest();
	void StartPatrolling();
	void ChaseTarget();
	bool IsOutsideCombatRadius();
	bool IsOutsideAttackRadius();
	bool IsInsideAttackRadius();
	bool IsChasing();
	bool IsAttacking();
	bool IsDead();
	bool IsEngaged();
	void ClearPatrolTimer();
	void StartAttackTimer();
	void ClearAttackTimer();
	void PatrolTimerFinished();
	void StopAttackMontage();
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void MoveToTarget(AActor* Target);
	AActor* ChoosePatrolTarget();
	void SpawnDefaultWeapon();

	UFUNCTION()
	void PawnSeen(APawn* SeenPawn);

	UPROPERTY(VisibleAnywhere)
	UHealthBarComponent* HealthBarWidget;

	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<class AWeapon> WeaponClass;

	// 武器要挂到的骨骼 socket 名字，和你的僵尸/敌人骨架保持一致
	UPROPERTY(EditAnywhere, Category = Combat)
	FName WeaponSocketName = FName("RightHandSocket");

	UPROPERTY()
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, Category = Combat)
	double CombatRadius = 1500.f;

	// 出手前的最小/最大等待：0 = 一进攻击距离就立刻出手
	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackMin = 0.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackMax = 0.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float PatrollingSpeed = 125.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ChasingSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float DeathLifeSpan = 8.f;

	FTimerHandle AttackTimer;

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
	double AttackRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	double MoveAcceptanceRadius = 15.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMin = 4.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMax = 6.f;

	FTimerHandle PatrolTimer;
};
