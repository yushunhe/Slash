#include "Characters/SlashCharacter.h"
#include  "GameFramework/SpringArmComponent.h"
#include  "Camera/CameraComponent.h"
#include  "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Items/Item.h"
#include "Items/Weapons/Weapon.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom);
}

void ASlashCharacter::BeginPlay()
{
	Super::BeginPlay();

	Tags.Add(FName("SlashCharacter"));

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
	PlayerInputComponent->BindAction(FName("Attack"), IE_Pressed, this, &ASlashCharacter::Attack);
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
	if (!IsUnoccupied()) return;
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
	if (!IsUnoccupied()) return;
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
		if (EquippedWeapon)
		{
			EquippedWeapon->Destroy();
		}
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

void ASlashCharacter::EquipWeapon(AWeapon* Weapon)
{
	Weapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	OverlappingItem = nullptr;
	EquippedWeapon = Weapon;
}

void ASlashCharacter::AttackEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::CanAttack()
{
	return IsUnoccupied() && CharacterState != ECharacterState::ECS_Unequipped;
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
