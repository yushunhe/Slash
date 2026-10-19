// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/HitInterface.h"
#include "BreakableActor.generated.h"

class UGeometryCollectionComponent;
class UBoxComponent;
UCLASS()
class SLASH_API ABreakableActor : public AActor, public IHitInterface
{
	GENERATED_BODY()
	
public:	
	ABreakableActor();
	virtual void Tick(float DeltaTime) override;
	
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UBoxComponent* Box;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UGeometryCollectionComponent* GeometryCollectionComponent;
private:

	UPROPERTY(EditAnywhere, Category = "Breakable Properties")
	TArray<TSubclassOf<class AAmmo>> AmmoClasses;

	bool bIsBroken = false;

};
