#include "Characters/SlashCharacter.h"
#include  "GameFramework/SpringArmComponent.h"
#include  "Camera/CameraComponent.h"
#include  "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Items/Item.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/RangedWeapon.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/UserWidget.h"
#include "HUD/SlashHUD.h"
#include "HUD/SlashOverlay.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

ASlashCharacter::ASlashCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);

	// 教程设置：玩家的网格要能被武器的 WeaponBox 检测到（敌人才能砍中玩家）
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom);
}

void ASlashCharacter::BeginPlay()
{
	Super::BeginPlay();

	Tags.Add(FName("SlashCharacter"));

	CaptureHipCameraValues();
	InitializeOverlay();
	InitializeAttributeDelegates();
}

void ASlashCharacter::InitializeOverlay()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController) return;

	// 优先使用 ASlashHUD 创建的 Overlay（GameMode 的 HUD Class 设置为 BP_SlashHUD）
	if (ASlashHUD* SlashHUD = Cast<ASlashHUD>(PlayerController->GetHUD()))
	{
		SlashOverlay = SlashHUD->GetSlashOverlay();
	}

	// 兜底：没配置 SlashHUD 时自己创建一个，避免界面直接消失
	if (!SlashOverlay && SlashOverlayClass && IsLocallyControlled())
	{
		SlashOverlay = CreateWidget<USlashOverlay>(PlayerController, SlashOverlayClass);
		if (SlashOverlay)
		{
			SlashOverlay->AddToViewport();
		}
	}
}

void ASlashCharacter::InitializeAttributeDelegates()
{
	if (!Attributes) return;

	if (SlashOverlay)
	{
		// 数值变化时由 AttributeComponent 主动推给 HUD
		Attributes->OnHealthChanged.AddUObject(SlashOverlay, &USlashOverlay::SetHealthBarPercent);
		Attributes->OnStaminaChanged.AddUObject(SlashOverlay, &USlashOverlay::SetStaminaBarPercent);
		// Count 文本现在显示金币数；想显示 Souls 就换成 OnSoulsChanged 和 GetSouls()
		Attributes->OnGoldChanged.AddUObject(SlashOverlay, &USlashOverlay::SetCount);

		// 初始值：第一帧之前委托不会触发
		SlashOverlay->SetHealthBarPercent(Attributes->GetHealthPercent());
		SlashOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
		SlashOverlay->SetCount(Attributes->GetGold());
	}
}

void ASlashCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAimCamera(DeltaTime);
	UpdateAimFacing();
	UpdateFireCameraAlignment(DeltaTime);

	// 弹匣打空（或开始换弹）时即使还按着左键，也要退回普通瞄准动作
	if (bRifleFirePlaying && (!EquippedRifle || !EquippedRifle->CanFire()))
	{
		StopRifleFire();
	}

	const bool bReloading = EquippedRifle && EquippedRifle->IsReloading();

	// 持枪时：换弹 → 开火 → 待机，优先级依次降低
	if (CombatMode == ECombatMode::ECM_Ranged && EquippedRifle && IsAlive())
	{
		if (bReloading)
		{
			// 换弹动画占着插槽，别让待机把它顶掉
			StopRifleIdle();
		}
		else if (bRifleFirePlaying)
		{
			// 开火期间保持开火动作，不要被待机动画顶掉
			StopRifleIdle();
		}
		else if (GetVelocity().SizeSquared2D() > 100.f || GetCharacterMovement()->IsFalling())
		{
			StopRifleIdle();
		}
		else
		{
			PlayRifleIdle();
		}
	}
}

void ASlashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &ASlashCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("MoveRight"), this, &ASlashCharacter::MoveRight);
	PlayerInputComponent->BindAxis(FName("Turn"), this, &ASlashCharacter::Turn);
	PlayerInputComponent->BindAxis(FName("LookUp"), this, &ASlashCharacter::LookUp);

	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &ASlashCharacter::Jump);
	PlayerInputComponent->BindAction(FName("Equip"), IE_Pressed, this, &ASlashCharacter::EKeyPressed);
	PlayerInputComponent->BindAction(FName("Attack"), IE_Pressed, this, &ASlashCharacter::PrimaryActionPressed);
	PlayerInputComponent->BindAction(FName("Attack"), IE_Released, this, &ASlashCharacter::PrimaryActionReleased);
	PlayerInputComponent->BindAction(FName("Aim"), IE_Pressed, this, &ASlashCharacter::StartAim);
	PlayerInputComponent->BindAction(FName("Aim"), IE_Released, this, &ASlashCharacter::StopAim);
	PlayerInputComponent->BindAction(FName("Reload"), IE_Pressed, this, &ASlashCharacter::ReloadWeapon);
	PlayerInputComponent->BindAction(FName("SwitchWeapon"), IE_Pressed, this, &ASlashCharacter::SwitchWeapon);
}

void ASlashCharacter::Jump()
{
	if (IsUnoccupied())
	{
		Super::Jump();
	}
}

float ASlashCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);

	// 有些伤害来源只调 ApplyDamage 不调 GetHit，这里兜底触发死亡
	if (!IsAlive() && ActionState != EActionState::EAS_Dead)
	{
		const FVector ImpactPoint = DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation();
		DirectionalDie(ImpactPoint);
	}

	return DamageAmount;
}

void ASlashCharacter::GetHit_Implementation(const FVector& ImpactPoint)
{
	// 这一击把人打死了（伤害已在 TakeDamage 里结算）：只补音效/粒子，不重复播放倒地方向
	if (!IsAlive())
	{
		if (ActionState != EActionState::EAS_Dead)
		{
			DirectionalDie(ImpactPoint);
		}
		PlayHitSound(ImpactPoint);
		SpawnHitParticles(ImpactPoint);
		return;
	}

	Super::GetHit_Implementation(ImpactPoint);

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	// 没有配置受击蒙太奇时不进入受击状态，避免永远卡在 EAS_HitReaction
	if (IsAlive() && HitReactMontage)
	{
		ActionState = EActionState::EAS_HitReaction;
	}
}

void ASlashCharacter::Die(const FName& /*SectionName*/)
{
	StopAim();
	StopRifleFire();
	StopRifleIdle();

	if (EquippedRifle)
	{
		EquippedRifle->StopFire();
	}

	ActionState = EActionState::EAS_Dead;

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	if (APlayerController* PlayerController = GetController<APlayerController>())
	{
		DisableInput(PlayerController);
	}

	// 不播死亡蒙太奇，死亡瞬间直接瘫软
	StartRagdoll();
}

void ASlashCharacter::DirectionalDie(const FVector& ImpactPoint)
{
	const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
	DeathImpactPoint = ImpactPoint;
	DeathImpactDirection = (ImpactLowered - GetActorLocation()).GetSafeNormal();

	Die(NAME_None);
}

void ASlashCharacter::StartRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	// 没有 Physics Asset 就退化成停在死亡动画最后一帧，至少不会站起来
	if (!MeshComp->GetPhysicsAsset())
	{
		MeshComp->bPauseAnims = true;
		return;
	}

	// 先停动画，让物理从死亡动画最后一帧的姿势接管
	MeshComp->bPauseAnims = true;

	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	// 从当前动画姿势平滑混入物理，避免瞬间弹开
	MeshComp->bBlendPhysics = true;

	// 沿着受击方向补一点冲量，倒得更自然
	if (!DeathImpactDirection.IsNearlyZero())
	{
		const float ImpulseStrength = 12000.f;
		MeshComp->AddImpulseAtLocation(DeathImpactDirection * ImpulseStrength, DeathImpactPoint, FName("pelvis"));
	}
}

void ASlashCharacter::MoveForward(float Value)
{
	if (!IsUnoccupied() || IsReloadingRifle()) return;
	if (Controller && Value != 0.f)
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASlashCharacter::MoveRight(float Value)
{
	if (!IsUnoccupied() || IsReloadingRifle()) return;
	if (Controller && Value != 0.f)
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASlashCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASlashCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASlashCharacter::EKeyPressed()
{
	AWeapon* OverlappingWeapon = Cast<AWeapon>(OverlappingItem);
	if (OverlappingWeapon)
	{
		EquipWeapon(OverlappingWeapon);
	}
	else
	{
		if (CanDisarm())
		{
			Disarm();
		}
		else if (CanArm())
		{
			Arm();
		}
	}
}

void ASlashCharacter::Attack()
{
	Super::Attack();
	if (CanAttack())
	{
		PlayAttackMontage();
		ActionState = EActionState::EAS_Attacking;
	}
}

void ASlashCharacter::PrimaryActionPressed()
{
	if (CombatMode == ECombatMode::ECM_Ranged && EquippedRifle)
	{
		// 没子弹时只出空枪声，不要播开火动作
		const bool bCanFire = EquippedRifle->CanFire();
		EquippedRifle->StartFire();
		if (bCanFire)
		{
			PlayRifleFire();
		}
		return;
	}

	Attack();
}

void ASlashCharacter::PrimaryActionReleased()
{
	if (EquippedRifle)
	{
		EquippedRifle->StopFire();
		StopRifleFire();
	}
}

void ASlashCharacter::StartAim()
{
	if (bIsAiming || CombatMode != ECombatMode::ECM_Ranged || !EquippedRifle)
	{
		return;
	}

	bIsAiming = true;
	EquippedRifle->SetAiming(true);

	// 记录进入瞄准前的移速，退出时恢复（避免覆盖蓝图里调过的值）
	PreAimWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;

	// 朝向由 UpdateAimFacing 每帧手动设置（相机朝向 + AimYawOffset），
	// 所以这里不要再让控制器直接驱动 yaw，否则会覆盖偏移
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 相机由 UpdateAimCamera 在 Tick 里平滑过渡到过肩视角
}

void ASlashCharacter::StopAim()
{
	if (!bIsAiming)
	{
		return;
	}

	bIsAiming = false;
	if (EquippedRifle)
	{
		EquippedRifle->SetAiming(false);
	}

	GetCharacterMovement()->MaxWalkSpeed = PreAimWalkSpeed;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 相机由 UpdateAimCamera 在 Tick 里平滑回到髋射视角
}

bool ASlashCharacter::IsReloadingRifle() const
{
	return EquippedRifle && EquippedRifle->IsReloading();
}

void ASlashCharacter::UpdateFireCameraAlignment(float DeltaTime)
{
	if (!bAlignCameraToMuzzleOnFire || bIsAiming || !bRifleFirePlaying || !EquippedRifle)
	{
		return;
	}

	// 站着开火才对齐：一边移动一边转视角会让"输入方向（跟随相机）"和"相机（跟随枪口）"互相推动
	if (GetVelocity().SizeSquared2D() > FMath::Square(FireCameraAlignMaxSpeed))
	{
		return;
	}

	AController* OwnerController = GetController();
	if (!OwnerController)
	{
		return;
	}

	const FRotator Target = EquippedRifle->GetBarrelRotation();
	const FRotator Smoothed = FMath::RInterpTo(
		OwnerController->GetControlRotation(), Target, DeltaTime, FireCameraAlignSpeed);
	OwnerController->SetControlRotation(Smoothed);
}

void ASlashCharacter::UpdateAimFacing()
{
	if (!bIsAiming)
	{
		return;
	}

	AController* OwnerController = GetController();
	if (!OwnerController)
	{
		return;
	}

	// 角色始终面向"相机 yaw + 偏移"，这样动画里的持枪姿势能对准准星；
	// 射击方向仍由相机决定，不受这里影响
	const float DesiredYaw = OwnerController->GetControlRotation().Yaw + AimYawOffset;
	const FRotator Current = GetActorRotation();
	SetActorRotation(FRotator(Current.Pitch, DesiredYaw, Current.Roll));
}

void ASlashCharacter::CaptureHipCameraValues()
{
	if (CameraBoom)
	{
		HipArmLength = CameraBoom->TargetArmLength;
		HipSocketOffset = CameraBoom->SocketOffset;
		HipTargetOffset = CameraBoom->TargetOffset;
	}

	if (ViewCamera)
	{
		HipFOV = ViewCamera->FieldOfView;
	}
}

void ASlashCharacter::UpdateAimCamera(float DeltaTime)
{
	if (!CameraBoom) return;

	// 已经完全回到髋射视角：收尾一次后不再每帧写相机
	if (!bIsAiming && AimBlend <= KINDA_SMALL_NUMBER)
	{
		if (AimBlend > 0.f)
		{
			AimBlend = 0.f;
			CameraBoom->TargetArmLength = HipArmLength;
			CameraBoom->SocketOffset = HipSocketOffset;
			CameraBoom->TargetOffset = HipTargetOffset;
			if (ViewCamera) ViewCamera->SetFieldOfView(HipFOV);
		}
		return;
	}

	AimBlend = FMath::FInterpTo(AimBlend, bIsAiming ? 1.f : 0.f, DeltaTime, AimCameraInterpSpeed);

	CameraBoom->TargetArmLength = FMath::Lerp(HipArmLength, AimArmLength, AimBlend);
	CameraBoom->SocketOffset = FMath::Lerp(HipSocketOffset, AimSocketOffset, AimBlend);
	CameraBoom->TargetOffset = FMath::Lerp(HipTargetOffset, AimTargetOffset, AimBlend);

	if (ViewCamera)
	{
		ViewCamera->SetFieldOfView(FMath::Lerp(HipFOV, AimFOV, AimBlend));
	}
}

void ASlashCharacter::ReloadWeapon()
{
	if (EquippedRifle)
	{
		EquippedRifle->Reload();
		// 换弹会中断射击，开火动画也要跟着停
		StopRifleFire();
	}
}

void ASlashCharacter::SwitchWeapon()
{
	// 还没有步枪时，站在步枪旁边按 Q 也可以直接拾取（E 仍是通用拾取键）
	if (!EquippedRifle && OverlappingItem)
	{
		if (ARangedWeapon* Rifle = Cast<ARangedWeapon>(OverlappingItem))
		{
			EquipWeapon(Rifle);
			return;
		}
	}

	if (CombatMode == ECombatMode::ECM_Melee)
	{
		if (!EquippedRifle)
		{
			return;
		}

		// 收起近战武器，拿出步枪
		if (EquippedMeleeWeapon)
		{
			EquippedMeleeWeapon->AttachMeshToSocket(GetMesh(), FName("thighsocket"));
		}
		EquippedRifle->AttachMeshToSocket(GetMesh(), FName("Weapon"));
		EquippedWeapon = nullptr;
		CharacterState = ECharacterState::ECS_Unequipped;
		CombatMode = ECombatMode::ECM_Ranged;
		bHasRifle = true;
		PlayRifleIdle();
		return;
	}

	// 从步枪切回近战
	if (!EquippedRifle || !EquippedMeleeWeapon)
	{
		return;
	}

	EquippedRifle->StopFire();
	StopRifleFire();
	StopAim();
	StopRifleIdle();
	EquippedRifle->AttachMeshToSocket(GetMesh(), FName("SpineSocket"));
	EquippedMeleeWeapon->AttachMeshToSocket(GetMesh(), FName("RightHandSocket"));
	EquippedWeapon = EquippedMeleeWeapon;
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	CombatMode = ECombatMode::ECM_Melee;
}

void ASlashCharacter::PlayRifleIdle()
{
	if (!RifleIdleAnimation || bRifleIdlePlaying)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// 通过现有 DefaultSlot 播放动态蒙太奇；LoopCount 设很大，靠移动/切枪来停止
	constexpr int32 LoopCount = 100000;
	RifleIdleMontageInstance = AnimInstance->PlaySlotAnimationAsDynamicMontage(
		RifleIdleAnimation,
		FName("DefaultSlot"),
		0.2f,
		0.2f,
		1.f,
		LoopCount,
		0.f,
		0.f);

	bRifleIdlePlaying = RifleIdleMontageInstance != nullptr;
}

void ASlashCharacter::StopRifleIdle()
{
	if (!bRifleIdlePlaying)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.2f, RifleIdleMontageInstance);
		}
	}

	RifleIdleMontageInstance = nullptr;
	bRifleIdlePlaying = false;
}

void ASlashCharacter::PlayRifleFire()
{
	// 挂点和动画相互独立：即使没配开火动画，也要切到开火用的 socket
	if (EquippedRifle)
	{
		EquippedRifle->AttachMeshToSocket(GetMesh(), RifleFireSocketName);
	}

	if (bRifleFirePlaying)
	{
		return;
	}

	// 没有配置开火动画时保持原样，不影响射击本身
	if (!RifleFireAnimation)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	// 开火动作和待机动作抢同一个 DefaultSlot，先停待机
	StopRifleIdle();

	constexpr int32 LoopCount = 100000;
	RifleFireMontageInstance = AnimInstance->PlaySlotAnimationAsDynamicMontage(
		RifleFireAnimation,
		FName("DefaultSlot"),
		0.05f,
		0.1f,
		1.f,
		LoopCount,
		0.f,
		0.f);

	bRifleFirePlaying = RifleFireMontageInstance != nullptr;
}

void ASlashCharacter::StopRifleFire()
{
	// 回到平时的持枪 socket
	if (EquippedRifle)
	{
		EquippedRifle->AttachMeshToSocket(GetMesh(), FName("Weapon"));
	}

	if (!bRifleFirePlaying)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.15f, RifleFireMontageInstance);
		}
	}

	RifleFireMontageInstance = nullptr;
	bRifleFirePlaying = false;
}

void ASlashCharacter::AddAmmo(int32 Amount)
{
	if (EquippedRifle)
	{
		EquippedRifle->AddReserveAmmo(Amount);
	}
}

void ASlashCharacter::EquipWeapon(AWeapon* Weapon)
{
	if (!Weapon) return;

	if (bIsAiming)
	{
		StopAim();
	}

	if (Weapon->IsRanged())
	{
		ARangedWeapon* Rifle = Cast<ARangedWeapon>(Weapon);
		if (!Rifle) return;

		if (EquippedRifle && EquippedRifle != Rifle)
		{
			EquippedRifle->StopFire();
			StopRifleFire();
			EquippedRifle->Destroy();
		}

		// 收起近战武器，拿出步枪
		if (EquippedMeleeWeapon)
		{
			EquippedMeleeWeapon->AttachMeshToSocket(GetMesh(), FName("thighsocket"));
		}

		EquippedRifle = Rifle;
		EquippedRifle->Equip(GetMesh(), FName("Weapon"), this, this);
		EquippedWeapon = nullptr;
		OverlappingItem = nullptr;
		CharacterState = ECharacterState::ECS_Unequipped;
		CombatMode = ECombatMode::ECM_Ranged;
		bHasRifle = true;
		PlayRifleIdle();
		return;
	}

	// 近战武器
	StopRifleIdle();
	if (EquippedMeleeWeapon && EquippedMeleeWeapon != Weapon)
	{
		EquippedMeleeWeapon->Destroy();
	}

	// 有步枪时先收起来
	if (EquippedRifle)
	{
		EquippedRifle->StopFire();
		StopRifleFire();
		EquippedRifle->AttachMeshToSocket(GetMesh(), FName("SpineSocket"));
	}

	Weapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
	EquippedMeleeWeapon = Weapon;
	EquippedWeapon = Weapon;
	OverlappingItem = nullptr;
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	CombatMode = ECombatMode::ECM_Melee;
	bHasRifle = EquippedRifle != nullptr;
}

void ASlashCharacter::AttackEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::CanAttack()
{
	return CombatMode == ECombatMode::ECM_Melee
		&& IsUnoccupied()
		&& CharacterState != ECharacterState::ECS_Unequipped;
}

bool ASlashCharacter::CanDisarm()
{
	return IsUnoccupied() && CharacterState != ECharacterState::ECS_Unequipped;
}

bool ASlashCharacter::CanArm()
{
	return IsUnoccupied() && CharacterState == ECharacterState::ECS_Unequipped && EquippedWeapon;
}

void ASlashCharacter::Disarm()
{
	PlayEquipMontage(FName("Unequip"));
	CharacterState = ECharacterState::ECS_Unequipped;
	ActionState = EActionState::EAS_EquippingWeapon;
}

void ASlashCharacter::Arm()
{
	PlayEquipMontage(FName("Equip"));
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	ActionState = EActionState::EAS_EquippingWeapon;
}

void ASlashCharacter::AttachWeaponToBack()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("thighSocket"));
	}
}

void ASlashCharacter::AttachWeaponToHand()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("RightHandSocket"));
	}
}

void ASlashCharacter::PlayEquipMontage(const FName& SectionName)
{
	PlayMontageSection(EquipMontage, SectionName);
}

void ASlashCharacter::FinishEquipping()
{
	ActionState = EActionState::EAS_Unoccupied;
}

void ASlashCharacter::HitReactEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::IsUnoccupied()
{
	return ActionState == EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::IsOccupied()
{
	return ActionState != EActionState::EAS_Unoccupied;
}
