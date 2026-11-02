// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SlashHUD.generated.h"

class USlashOverlay;

/**
 * 玩家 HUD：负责创建并持有主界面 Overlay。
 * 在 GameMode 的 HUD Class 里指定 BP_SlashHUD，并在 BP 里把 SlashOverlayClass 设为 WBP_SlashOverlay。
 */
UCLASS()
class SLASH_API ASlashHUD : public AHUD
{
	GENERATED_BODY()

public:
	FORCEINLINE USlashOverlay* GetSlashOverlay() const { return SlashOverlay; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = HUD)
	TSubclassOf<USlashOverlay> SlashOverlayClass;

	UPROPERTY()
	USlashOverlay* SlashOverlay;
};
