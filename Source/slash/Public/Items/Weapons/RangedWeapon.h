// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Weapon.h"
#include "TimerManager.h"
#include "RangedWeapon.generated.h"

class USceneComponent;
class USoundBase;
class UParticleSystem;
class UAnimMontage;
class UAnimSequence;

/** 弹匣 / 备弹变化，HUD 订阅这个委托刷新文本 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32 /*InMagazine*/, int32 /*Reserve*/);

/**
 * 枪械基类：Hitscan 射线射击、弹匣与备弹、换弹。
 * 挂点由 AWeapon::Equip 负责；Muzzle 是 ItemMesh 的子组件，可在 BP 里微调相对位置。
 */
UCLASS()
class SLASH_API ARangedWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	ARangedWeapon();

	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;

	/** 不管蓝图里 WeaponType 是否被父类默认值覆盖，枪械永远走远程分支 */
	virtual bool IsRanged() const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void AddReserveAmmo(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Weapon|Ranged")
	bool CanFire() const;

	void SetAiming(bool bNewAiming) { bIsAiming = bNewAiming; }

	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	FORCEINLINE int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	FORCEINLINE int32 GetReserveAmmo() const { return ReserveAmmo; }
	FORCEINLINE int32 GetMagazineSize() const { return MagazineSize; }
	FORCEINLINE float GetReloadTime() const { return ReloadTime; }
	FORCEINLINE bool IsReloading() const { return bIsReloading; }

	/** AI 开火入口：从枪口朝 TargetPoint 射击，带 AIAimSpreadDegrees 散布；不用准星、不做后坐力 */
	void FireAt(const FVector& TargetPoint);

	/** 枪管指向的世界旋转（步枪模型长轴是 +Y） */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ranged")
	FRotator GetBarrelRotation() const;

	/** HUD 订阅：弹匣 / 备弹变化 */
	FOnAmmoChanged OnAmmoChanged;

protected:
	void FireOnce();
	void FinishReload();

	/** 按当前移动状态挑一个换弹动画：跑动/下落用移动版，否则用站立版 */
	UAnimSequence* SelectReloadAnimation() const;

	/** 玩家：两段式射线（相机 -> 准星点，枪口 -> 准星点），输出射击方向，避免隔墙命中 */
	void ComputePlayerShotDirection(FVector& OutShotDirection) const;

	/** 从枪口沿 ShotDirection 做一次射线，并结算伤害与特效（玩家和 AI 共用） */
	void ResolveShot(const FVector& MuzzleLocation, const FVector& ShotDirection);

	void ApplyShotDamage(const FHitResult& Hit, const FVector& ShotDirection);
	void PlayFireFX(const FVector& MuzzleLocation, const FHitResult& Hit);

	/** 可在 BP 里补枪口火光/弹着特效；C++ 已经处理了配置好的音效和粒子 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|FX")
	void OnFireFX(const FVector& MuzzleLocation, const FVector& ImpactPoint, bool bHit);

	UPROPERTY(VisibleAnywhere, Category = "Weapon|Components")
	USceneComponent* Muzzle;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float DamagePerShot = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float FireInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float Range = 10000.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float HipSpreadDegrees = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float AimSpreadDegrees = 0.3f;

	/** AI 开火时的散布（度），比玩家的开镜散布略大 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float AIAimSpreadDegrees = 4.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float HeadshotMultiplier = 2.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	int32 AmmoInMagazine = 30;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	int32 ReserveAmmo = 120;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float ReloadTime = 2.2f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float RecoilPitch = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float RecoilYaw = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	USoundBase* EmptySound;

	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	UAnimMontage* ReloadMontage;

	/** 站立换弹动画（AnimSequence）。优先于 ReloadMontage，按插槽动画播放一次 */
	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	UAnimSequence* ReloadStandingAnimation;

	/** 移动/跑动中换弹动画；没配置时退回站立换弹动画 */
	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	UAnimSequence* ReloadMovingAnimation;

	/** 二维速度超过这个值就认为在移动，用移动换弹动画 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	float ReloadMovingSpeedThreshold = 100.f;

	/** 换弹动画使用的 AnimBP 插槽 */
	UPROPERTY(EditAnywhere, Category = "Weapon|FX")
	FName ReloadSlotName = FName("DefaultSlot");

	/** 弹匣打空后自动换弹 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Ranged")
	bool bAutoReload = true;

	/** 调试用：在 PIE 里画出射线 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Debug")
	bool bDrawDebugShot = false;

private:
	bool bIsFiring = false;
	bool bIsAiming = false;
	bool bIsReloading = false;

	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
};