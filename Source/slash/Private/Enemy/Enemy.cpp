// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/WidgetComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "HUD/HealthBarComponent.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "Slash/Debug_macros.h"
// Sets default values
AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Block);
	GetMesh()->SetGenerateOverlapEvents(true);

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 1000.f;
	PawnSensing->SetPeripheralVisionAngle(45.f);
}

void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}

	EnemyController = Cast<AAIController>(GetController());
	MoveToTarget(PatrolTarget);

	if (PawnSensing)
	{
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	}
}

void AEnemy::Die(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);

		AnimInstance->Montage_JumpToSection(SectionName, DeathMontage);
	}
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(8.0f);
}

bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr)
		return false;

	const double DistanceToTarget = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
		DRAW_SPHERE_SingleFrame(GetActorLocation());
		DRAW_SPHERE_SingleFrame(Target->GetActorLocation());
		return DistanceToTarget <= Radius;

}

void AEnemy::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactMontage);
	}
}

void AEnemy::MoveToTarget(AActor* Target)
{
	if (EnemyController == nullptr || Target == nullptr) return;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(MoveAcceptanceRadius);
	EnemyController->MoveTo(MoveRequest);
}

AActor* AEnemy::ChoosePatrolTarget()
{
	TArray<AActor*> ValidTargets;
	for (AActor* Target : PatrolTargets)
	{
		if (Target && Target != PatrolTarget)
		{
			ValidTargets.AddUnique(Target);
		}
	}

	const int32 NumValidTargets = ValidTargets.Num();
	if (NumValidTargets > 0)
	{
		const int32 TargetSelection = FMath::RandRange(0, NumValidTargets - 1);
		return ValidTargets[TargetSelection];
	}
	return nullptr;
}

void AEnemy::PawnSeen(APawn* SeenPawn)
{
	if (EnemyState == EEnemyState::EES_Chasing) return;
	if (SeenPawn->ActorHasTag(FName("SlashCharacter")))
	{
        GetWorldTimerManager().ClearTimer(PatrolTimer);
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		CombatTarget = SeenPawn;

		if (EnemyState != EEnemyState::EES_Attacking)
		{
		EnemyState = EEnemyState::EES_Chasing;
		MoveToTarget(CombatTarget);
		UE_LOG(LogTemp, Warning, TEXT("Pawn Seen,Chasing Player"));
		}
	
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (EnemyState > EEnemyState::EES_Patrolling)
	{
		CheckCombatTarget();
	}
	else 
	{
        CheckPatrolTarget();
	}
	
}

void AEnemy::CheckPatrolTarget()
{
	if (PatrolTarget && EnemyController)
	{
		if (InTargetRange(PatrolTarget, PatrolRadius))
		{
			PatrolTarget = ChoosePatrolTarget();

			if (!GetWorldTimerManager().IsTimerActive(PatrolTimer))
			{
				const float WaitTime = FMath::RandRange(WaitMin, WaitMax);
				GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, WaitTime);
			}
		}
	}
}

void AEnemy::CheckCombatTarget()
{

		if (!InTargetRange(CombatTarget, CombatRadius))
		{
			CombatTarget = nullptr;
			if (HealthBarWidget)
			{
				HealthBarWidget->SetVisibility(false);
			}
			EnemyState = EEnemyState::EES_Patrolling;
			GetCharacterMovement()->MaxWalkSpeed = 125.f;
			MoveToTarget(PatrolTarget);
			UE_LOG(LogTemp, Warning, TEXT("Lose Interest in Player"));
		}
	
	else if (!InTargetRange(CombatTarget, AttackRadius) && EnemyState != EEnemyState::EES_Chasing)
	{
			EnemyState = EEnemyState::EES_Chasing;
			GetCharacterMovement()->MaxWalkSpeed = 300.f;
			MoveToTarget(CombatTarget);
			UE_LOG(LogTemp, Warning, TEXT("Chasing Player"));
	}
    else if(InTargetRange(CombatTarget, AttackRadius) && EnemyState != EEnemyState::EES_Attacking)
    {
			EnemyState = EEnemyState::EES_Attacking;
			UE_LOG(LogTemp, Warning, TEXT("Attacking Player"));
    }
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	//DRAW_SPHERE_COLOR(ImpactPoint, FColor::Cyan);
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(true);
	}
	if (Attributes && Attributes->IsAlive())
	{
			DirectionalHitReact(ImpactPoint);
	}
	else
	{
		DirectionalDie(ImpactPoint);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}
	
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint);
	}
}

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint)
{

	const FVector Forward = GetActorForwardVector();
	const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
	const FVector ToHit = (ImpactLowered - GetActorLocation()).GetSafeNormal();

	const double CosTheta = FVector::DotProduct(Forward, ToHit);
	double Theta = FMath::Acos(CosTheta);
	//convert to degrees
	Theta = FMath::RadiansToDegrees(Theta);

	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0.f)
	{
		Theta *= -1.f;
	}

	FName Section("FromBack");

	if (Theta >= -45.f && Theta < 45.f)
	{
		Section = FName("FromFront");
	}
	else if (Theta >= -135.f && Theta < -45.f)
	{
		Section = FName("FromLeft");
	}
	else if (Theta >= 45.f && Theta < 135.f)
	{
		Section = FName("FromRight");
	}

	
	PlayHitReactMontage(Section);
/*
	UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + CrossProduct * 100.f, 5.f, FColor::Red, 5.f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Theta: %f"), Theta));
	}
	UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + Forward * 60.f, 5.f, FColor::Red, 5.f);
	UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + ToHit * 60.f, 5.f, FColor::Green, 5.f);
*/
}

void AEnemy::DirectionalDie(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();
	const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
	const FVector ToHit = (ImpactLowered - GetActorLocation()).GetSafeNormal();

	const double CosTheta = FVector::DotProduct(Forward, ToHit);
	double Theta = FMath::Acos(CosTheta);
	//convert to degrees
	Theta = FMath::RadiansToDegrees(Theta);

	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0.f)
	{
		Theta *= -1.f;
	}

	FName Section("DeathBack");
	DeathPose = EDeathPose::EDP_DeadBack;
	if (Theta >= -45.f && Theta < 45.f)
	{
		Section = FName("DeathFront");
		DeathPose = EDeathPose::EDP_DeadFront;
	}
	else if (Theta >= -135.f && Theta < -45.f)
	{
		Section = FName("DeathLeft");
		DeathPose = EDeathPose::EDP_DeadLeft;
	}
	else if (Theta >= 45.f && Theta < 135.f)
	{
		Section = FName("DeathBack");
		DeathPose = EDeathPose::EDP_DeadBack;
	}
	Die(Section);
}



float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes && HealthBarWidget)
	{
		Attributes->ReceiveDamage(DamageAmount);
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());

	}
	CombatTarget = EventInstigator->GetPawn();
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	MoveToTarget(CombatTarget);
	return DamageAmount;
}

