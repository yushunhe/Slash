// Fill out your copyright notice in the Description page of Project Settings.


#include "Breaakable/BreakableActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Items/Ammo.h"
#include "Components/BoxComponent.h"

ABreakableActor::ABreakableActor()
{
	PrimaryActorTick.bCanEverTick = true;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollectionComponent"));
	SetRootComponent(GeometryCollectionComponent);
	GeometryCollectionComponent->SetGenerateOverlapEvents(true);
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(GetRootComponent());
	Box->SetBoxExtent(FVector(40.f, 40.f, 96.f));
	Box->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
}

void ABreakableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABreakableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABreakableActor::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (bIsBroken) return;
	bIsBroken = true;
	UWorld* World = GetWorld();
	
	if (World && AmmoClasses.Num() > 0)
	{
		
		World->SpawnActor<AAmmo>(AmmoClasses[0], GetActorLocation(), GetActorRotation());
	
	}
}

