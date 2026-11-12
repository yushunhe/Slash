// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SlashHUD.generated.h"

class USlashOverlay;
class ASlashCharacter;

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
	virtual void DrawHUD() override;

	/** 准星：只在瞄准时出现，淡入淡出 */
	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	bool bDrawCrosshair = true;

	/** 四条线离中心的距离 */
	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	float CrosshairGap = 8.f;

	/** 每条线的长度 */
	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	float CrosshairLength = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	float CrosshairThickness = 2.f;

	/** 淡入淡出速度，越大越干脆 */
	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	float CrosshairFadeSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	bool bDrawCrosshairCenterDot = true;

	UPROPERTY(EditDefaultsOnly, Category = Crosshair)
	FLinearColor CrosshairColor = FLinearColor(1.f, 1.f, 1.f, 0.85f);

	/** 右下角弹药显示：弹匣 / 备弹 */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bDrawAmmo = true;

	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	FLinearColor AmmoTextColor = FLinearColor::White;

	/** 弹匣为空时主数字的颜色 */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	FLinearColor AmmoEmptyColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	FLinearColor AmmoReserveColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	float AmmoTextScale = 1.8f;

	/** 离屏幕右下角的边距 */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	float AmmoMargin = 48.f;

	/** 换弹时替代弹匣数字显示的文本 */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	FString ReloadingText = TEXT("RELOADING");

private:
	UPROPERTY(EditDefaultsOnly, Category = HUD)
	TSubclassOf<USlashOverlay> SlashOverlayClass;

	UPROPERTY()
	USlashOverlay* SlashOverlay;

	/** 当前准星透明度，用于淡入淡出 */
	float CrosshairAlpha = 0.f;

	void DrawCrosshair();
	void DrawAmmo();
	ASlashCharacter* GetSlashCharacter() const;
};
