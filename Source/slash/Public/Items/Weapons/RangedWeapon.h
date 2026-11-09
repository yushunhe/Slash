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

	/** HUD 订阅：弹匣 / 备弹变化 */
	FOnAmmoChanged OnAmmoChanged;

protected:
	void FireOnce();
	void FinishReload();

	/** 两段式射线：相机 -> 准星点，枪口 -> 准星点，避免隔墙命中 */
	void PerformTrace(FHitResult& OutHit, FVector& OutShotDirection) const;

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