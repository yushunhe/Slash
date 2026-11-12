// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapons/RangedWeapon.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ARangedWeapon::ARangedWeapon()
{
	WeaponType = EWeaponType::EWT_Ranged;

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(ItemMesh);
	// 步枪模型长轴是 +Y，枪口在 +Y 方向；BP 里可继续微调
	Muzzle->SetRelativeLocation(FVector(0.f, 57.f, 15.f));
}

void ARangedWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	Super::Equip(InParent, InSocketName, NewOwner, NewInstigator);

	// 装备时同步一次初始弹药，避免 HUD 显示为空
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);
}

void ARangedWeapon::StartFire()
{
	if (!CanFire())
	{
		if (AmmoInMagazine <= 0 && EmptySound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, EmptySound, Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation());
		}
		return;
	}

	bIsFiring = true;
	FireOnce();

	// 全自动：按射速循环；单发武器把 FireInterval 设为 0
	if (bIsFiring && FireInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(FireTimer, this, &ARangedWeapon::FireOnce, FireInterval, true);
	}
	else
	{
		bIsFiring = false;
	}
}

void ARangedWeapon::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimer);
}

bool ARangedWeapon::CanFire() const
{
	return !bIsReloading && AmmoInMagazine > 0;
}

void ARangedWeapon::FireOnce()
{
	if (!CanFire())
	{
		StopFire();
		return;
	}

	--AmmoInMagazine;
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);

	FVector ShotDirection = FVector::ForwardVector;
	ComputePlayerShotDirection(ShotDirection);
	const FVector MuzzleLocation = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
	ResolveShot(MuzzleLocation, ShotDirection);

	// 后坐力：相机上抬 + 少量随机水平偏移
	if (APlayerController* PlayerController = Cast<APlayerController>(GetInstigatorController()))
	{
		PlayerController->AddPitchInput(-RecoilPitch);
		PlayerController->AddYawInput(FMath::FRandRange(-RecoilYaw, RecoilYaw));
	}

	if (AmmoInMagazine <= 0)
	{
		StopFire();

		// 打空后自动换弹；没有备弹时 Reload 会自己返回
		if (bAutoReload)
		{
			Reload();
		}
	}
}

FRotator ARangedWeapon::GetBarrelRotation() const
{
	if (ItemMesh)
	{
		// 网格长轴为 +Y，也就是枪管朝向
		return ItemMesh->GetRightVector().Rotation();
	}
	return GetActorRotation();
}

void ARangedWeapon::FireAt(const FVector& TargetPoint)
{
	if (!CanFire())
	{
		return;
	}

	const FVector MuzzleLocation = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
	const FVector BaseDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
	if (BaseDirection.IsNearlyZero())
	{
		return;
	}

	--AmmoInMagazine;
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);

	const FVector ShotDirection = FMath::VRandCone(BaseDirection, FMath::DegreesToRadians(AIAimSpreadDegrees));
	ResolveShot(MuzzleLocation, ShotDirection);
}

void ARangedWeapon::ComputePlayerShotDirection(FVector& OutShotDirection) const
{
	OutShotDirection = FVector::ForwardVector;

	AController* OwnerController = GetInstigatorController();
	UWorld* World = GetWorld();
	if (!OwnerController || !World)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(RangedWeaponAim), false, this);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	// 1) 相机中心 -> 准星指向点
	const FVector AimEnd = ViewLocation + ViewRotation.Vector() * Range;
	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, ViewLocation, AimEnd, ECC_Visibility, Params);
	const FVector AimPoint = AimHit.bBlockingHit ? AimHit.ImpactPoint : AimEnd;

	// 2) 枪口指向准星点，加上散布
	const FVector MuzzleLocation = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
	const float SpreadRadians = FMath::DegreesToRadians(bIsAiming ? AimSpreadDegrees : HipSpreadDegrees);
	const FVector BaseDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	OutShotDirection = FMath::VRandCone(BaseDirection, SpreadRadians);
}

void ARangedWeapon::ResolveShot(const FVector& MuzzleLocation, const FVector& ShotDirection)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(RangedWeaponShot), false, this);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const FVector ShotEnd = MuzzleLocation + ShotDirection * Range;
	World->LineTraceSingleByChannel(Hit, MuzzleLocation, ShotEnd, ECC_Visibility, Params);

	if (bDrawDebugShot)
	{
		const FVector DebugEnd = Hit.bBlockingHit ? Hit.ImpactPoint : ShotEnd;
		DrawDebugLine(World, MuzzleLocation, DebugEnd, FColor::Red, false, 1.5f, 0, 1.5f);
		DrawDebugPoint(World, DebugEnd, 8.f, FColor::Yellow, false, 1.5f);
	}

	ApplyShotDamage(Hit, ShotDirection);
	PlayFireFX(MuzzleLocation, Hit);
}

void ARangedWeapon::ApplyShotDamage(const FHitResult& Hit, const FVector& ShotDirection)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	// 同队免伤：敌人打敌人不结算
	if (GetOwner() && GetOwner()->ActorHasTag(TEXT("Enemy")) && HitActor->ActorHasTag(TEXT("Enemy")))
	{
		return;
	}

	const bool bHeadshot = Hit.BoneName.ToString().Equals(TEXT("head"), ESearchCase::IgnoreCase);
	const float AppliedDamage = DamagePerShot * (bHeadshot ? HeadshotMultiplier : 1.f);

	UGameplayStatics::ApplyPointDamage(
		HitActor,
		AppliedDamage,
		ShotDirection,
		Hit,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass());

	// 敌人的死亡/受击反应由 GetHit 驱动，必须显式调用
	if (IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
	{
		HitInterface->Execute_GetHit(HitActor, Hit.ImpactPoint);
	}
}

void ARangedWeapon::PlayFireFX(const FVector& MuzzleLocation, const FHitResult& Hit)
{
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLocation);
	}

	if (MuzzleFlash && Muzzle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), MuzzleFlash, MuzzleLocation, Muzzle->GetComponentRotation());
	}

	if (Hit.bBlockingHit && ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), ImpactParticles, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	OnFireFX(MuzzleLocation, Hit.bBlockingHit ? Hit.ImpactPoint : FVector::ZeroVector, Hit.bBlockingHit);
}

void ARangedWeapon::Reload()
{
	if (bIsReloading || AmmoInMagazine >= MagazineSize || ReserveAmmo <= 0)
	{
		return;
	}

	bIsReloading = true;
	StopFire();

	UAnimSequence* ReloadSequence = SelectReloadAnimation();

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance())
			{
				if (ReloadSequence)
				{
					// 和待机/开火一样走插槽，避免全身姿态被状态机顶掉
					AnimInstance->PlaySlotAnimationAsDynamicMontage(
						ReloadSequence,
						ReloadSlotName,
						0.1f,
						0.1f,
						1.f,
						1);
				}
				else if (ReloadMontage)
				{
					AnimInstance->Montage_Play(ReloadMontage);
				}
			}
		}
	}

	// 时长跟动画对齐，动画缺失时退回配置的 ReloadTime
	const float ReloadDuration = ReloadSequence ? ReloadSequence->GetPlayLength() : ReloadTime;
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &ARangedWeapon::FinishReload, ReloadDuration, false);
}

UAnimSequence* ARangedWeapon::SelectReloadAnimation() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	const bool bMoving = OwnerCharacter &&
		(OwnerCharacter->GetVelocity().SizeSquared2D() > FMath::Square(ReloadMovingSpeedThreshold) ||
			OwnerCharacter->GetCharacterMovement()->IsFalling());

	return (bMoving && ReloadMovingAnimation) ? ReloadMovingAnimation : ReloadStandingAnimation;
}

void ARangedWeapon::FinishReload()
{
	const int32 MissingAmmo = MagazineSize - AmmoInMagazine;
	const int32 AmmoToLoad = FMath::Min(MissingAmmo, ReserveAmmo);

	AmmoInMagazine += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;
	bIsReloading = false;

	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);
}

void ARangedWeapon::AddReserveAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	ReserveAmmo += Amount;
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);
}
