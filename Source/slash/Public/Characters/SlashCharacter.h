
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
	FORCEINLINE ARangedWeapon* GetEquippedRifle() const { return EquippedRifle; }
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

	/** 记录蓝图中配置的髋射相机参数，瞄准结束后回到这里 */
	void CaptureHipCameraValues();

	/** 每帧在髋射视角和过肩瞄准视角之间平滑过渡 */
	void UpdateAimCamera(float DeltaTime);

	/** 瞄准时让角色朝向 = 相机朝向 + AimYawOffset，使持枪姿势对准准星 */
	void UpdateAimFacing();

	/** 正在换弹：换弹动画期间锁住移动，等动画播完 */
	bool IsReloadingRifle() const;

	/** 髋射开火时把视角平滑转向枪口方向 */
	void UpdateFireCameraAlignment(float DeltaTime);

	/** 是否在髋射开火时把视角对齐到枪口 */
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	bool bAlignCameraToMuzzleOnFire = true;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float FireCameraAlignSpeed = 10.f;

	/** 移动速度超过这个值就不做对齐，避免视角跟着枪转、输入方向又跟着视角转 */
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float FireCameraAlignMaxSpeed = 150.f;

	/** 瞄准时角色相对相机偏转的角度（度，正值向右）；用来补偿动画里持枪的朝向偏差 */
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimYawOffset = 10.f;

	void ReloadWeapon();
	void SwitchWeapon();

	/** 持枪站立/瞄准时播放的待机动画 */
	void PlayRifleIdle();
	void StopRifleIdle();

	/** 按住左键射击时循环播放的开火动画，优先于待机动画 */
	void PlayRifleFire();
	void StopRifleFire();

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
	float AimArmLength = 175.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimFOV = 65.f;

	/** 瞄准时相机相对角色的偏移（Y 向右、Z 向上），把相机让到右肩外，角色落在画面左下 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector AimSocketOffset = FVector(0.f, 52.f, 45.f);

	/** 瞄准时抬高的注视点，让相机略微俯视角色 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector AimTargetOffset = FVector(0.f, 0.f, 40.f);

	/** 进入/退出瞄准时相机过渡的快慢 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimCameraInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimWalkSpeed = 300.f;

	/** 步枪待机动画（重定向后的 Idle Aiming），在 BP_SlashCharacter 里指定 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* RifleIdleAnimation;

	UPROPERTY()
	UAnimMontage* RifleIdleMontageInstance;

	bool bRifleIdlePlaying = false;

	/** 步枪开火动画（重定向后的 Firing Rifle），在 BP_SlashCharacter 里指定 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* RifleFireAnimation;

	/** 开火时步枪挂到的 socket；平时仍挂在 "Weapon" 上，方便单独调开火姿势的持枪位置 */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName RifleFireSocketName = FName("WeaponFire");

	UPROPERTY()
	UAnimMontage* RifleFireMontageInstance;

	bool bRifleFirePlaying = false;

	/** 进入瞄准前的移速，退出时恢复 */
	float PreAimWalkSpeed = 600.f;

	/** 0 = 髋射视角，1 = 完整过肩瞄准视角 */
	float AimBlend = 0.f;

	/** BeginPlay 时记录的默认相机参数（蓝图中配置的值） */
	float HipArmLength = 300.f;
	float HipFOV = 90.f;
	FVector HipSocketOffset = FVector::ZeroVector;
	FVector HipTargetOffset = FVector::ZeroVector;

public:
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
	FORCEINLINE EActionState GetActionState() const { return ActionState; }
};
