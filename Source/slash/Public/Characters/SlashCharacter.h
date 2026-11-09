
#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "SlashCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AItem;
class AWeapon;
class ARangedWeapon;
class UAnimMontage;
class UAnimSequence;
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

	UFUNCTION(BlueprintCallable)
	void AddAmmo(int32 Amount);

	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	FORCEINLINE bool HasRifle() const { return bHasRifle; }
	FORCEINLINE ECombatMode GetCombatMode() const { return CombatMode; }

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

	/** 左键：按当前武器分发近战/射击 */
	void PrimaryActionPressed();
	void PrimaryActionReleased();

	/** 右键瞄准 */
	void StartAim();
	void StopAim();

	void ReloadWeapon();
	void SwitchWeapon();

	/** 持枪站立/瞄准时播放的待机动画 */
	void PlayRifleIdle();
	void StopRifleIdle();

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

	/** 当前枪械；剑/近战仍由 EquippedWeapon 管理 */
	UPROPERTY()
	ARangedWeapon* EquippedRifle;

	UPROPERTY()
	AWeapon* EquippedMeleeWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	ECombatMode CombatMode = ECombatMode::ECM_Melee;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bHasRifle = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimArmLength = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimFOV = 65.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimWalkSpeed = 300.f;

	/** 步枪待机动画（重定向后的 Idle Aiming），在 BP_SlashCharacter 里指定 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* RifleIdleAnimation;

	UPROPERTY()
	UAnimMontage* RifleIdleMontageInstance;

	bool bRifleIdlePlaying = false;

	/** 进入瞄准前的相机/移速，退出时恢复 */
	float PreAimWalkSpeed = 600.f;
	float PreAimArmLength = 300.f;
	float PreAimFOV = 90.f;

public:
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
	FORCEINLINE EActionState GetActionState() const { return ActionState; }
};
