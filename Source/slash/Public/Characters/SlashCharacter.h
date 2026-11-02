
#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "SlashCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AItem;
class AWeapon;
class UAnimMontage;
class USlashOverlay;

UCLASS()
class SLASH_API ASlashCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:

	ASlashCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

protected:

	virtual void BeginPlay() override;

	/**
	* Input functions
	*/
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EKeyPressed();
	virtual void Attack() override;

	/**
	* Death
	*/
	virtual void Die(const FName& SectionName) override;
	void DirectionalDie(const FVector& ImpactPoint);

	/** 死亡时直接切换成布娃娃物理 */
	void StartRagdoll();

	FVector DeathImpactPoint = FVector::ZeroVector;
	FVector DeathImpactDirection = FVector::ZeroVector;

	/**
	* HUD
	*/
	void InitializeOverlay();
	void InitializeAttributeDelegates();

	/**
	* Combat
	*/
	void EquipWeapon(AWeapon* Weapon);
	virtual void AttackEnd() override;
	virtual bool CanAttack() override;
	bool CanDisarm();
	bool CanArm();
	void Disarm();
	void Arm();
	void PlayEquipMontage(const FName& SectionName);
	bool IsUnoccupied();
	bool IsOccupied();

	UFUNCTION(BlueprintCallable)
	void AttachWeaponToBack();

	UFUNCTION(BlueprintCallable)
	void AttachWeaponToHand();

	UFUNCTION(BlueprintCallable)
	void FinishEquipping();

	UFUNCTION(BlueprintCallable)
	void HitReactEnd();

private:
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* ViewCamera;

	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;

	UPROPERTY(EditDefaultsOnly, Category = Montages)
	UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<USlashOverlay> SlashOverlayClass;

	UPROPERTY()
	USlashOverlay* SlashOverlay;

	ECharacterState CharacterState = ECharacterState::ECS_Unequipped;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;

public:
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
	FORCEINLINE EActionState GetActionState() const { return ActionState; }
};
