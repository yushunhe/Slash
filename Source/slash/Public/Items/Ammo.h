// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Ammo.generated.h"

/**
 * 
 */
UCLASS()
class SLASH_API AAmmo : public AItem
{
	GENERATED_BODY()
protected:

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
private:
	UPROPERTY(EditAnywhere, Category = Sounds)
	USoundBase* PickUpSound;

	UPROPERTY(EditAnywhere, Category = "Ammo Properties")
	int32 AmmoCount;
};
