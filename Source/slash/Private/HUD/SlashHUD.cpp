// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SlashHUD.h"
#include "Blueprint/UserWidget.h"
#include "HUD/SlashOverlay.h"
#include "Characters/SlashCharacter.h"
#include "Items/Weapons/RangedWeapon.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ASlashHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!SlashOverlayClass) return;

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController) return;

	SlashOverlay = CreateWidget<USlashOverlay>(PlayerController, SlashOverlayClass);
	if (SlashOverlay)
	{
		SlashOverlay->AddToViewport();
	}
}

ASlashCharacter* ASlashHUD::GetSlashCharacter() const
{
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		return Cast<ASlashCharacter>(PlayerController->GetPawn());
	}
	return nullptr;
}

void ASlashHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawCrosshair();
	DrawAmmo();
}

void ASlashHUD::DrawCrosshair()
{
	if (!bDrawCrosshair)
	{
		return;
	}

	ASlashCharacter* Character = GetSlashCharacter();

	// 只有瞄准时才显示，切换时用帧时间做淡入淡出
	const float TargetAlpha = (Character && Character->IsAiming()) ? 1.f : 0.f;
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	CrosshairAlpha = FMath::FInterpTo(CrosshairAlpha, TargetAlpha, DeltaTime, CrosshairFadeSpeed);
	if (CrosshairAlpha <= 0.01f)
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	FLinearColor Color = CrosshairColor;
	Color.A *= CrosshairAlpha;

	const float Half = CrosshairThickness * 0.5f;

	// 以屏幕中心为原点画四条短线：右、左、下、上
	DrawRect(Color, CenterX + CrosshairGap, CenterY - Half, CrosshairLength, CrosshairThickness);
	DrawRect(Color, CenterX - CrosshairGap - CrosshairLength, CenterY - Half, CrosshairLength, CrosshairThickness);
	DrawRect(Color, CenterX - Half, CenterY + CrosshairGap, CrosshairThickness, CrosshairLength);
	DrawRect(Color, CenterX - Half, CenterY - CrosshairGap - CrosshairLength, CrosshairThickness, CrosshairLength);

	if (bDrawCrosshairCenterDot)
	{
		constexpr float DotSize = 2.f;
		DrawRect(Color, CenterX - DotSize * 0.5f, CenterY - DotSize * 0.5f, DotSize, DotSize);
	}
}

void ASlashHUD::DrawAmmo()
{
	if (!bDrawAmmo || !Canvas || !GEngine)
	{
		return;
	}

	ASlashCharacter* Character = GetSlashCharacter();
	if (!Character)
	{
		return;
	}

	ARangedWeapon* Rifle = Character->GetEquippedRifle();
	if (!Rifle)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	if (!Font)
	{
		return;
	}

	const bool bReloading = Rifle->IsReloading();
	const FString MagazineText = bReloading
		? ReloadingText
		: FString::FromInt(Rifle->GetAmmoInMagazine());
	const FString ReserveText = FString::Printf(TEXT("/ %d"), Rifle->GetReserveAmmo());

	const float ReserveScale = AmmoTextScale * 0.55f;

	float MagazineWidth = 0.f, MagazineHeight = 0.f;
	float ReserveWidth = 0.f, ReserveHeight = 0.f;
	GetTextSize(MagazineText, MagazineWidth, MagazineHeight, Font, AmmoTextScale);
	GetTextSize(ReserveText, ReserveWidth, ReserveHeight, Font, ReserveScale);

	constexpr float Gap = 10.f;
	const float TotalWidth = MagazineWidth + Gap + ReserveWidth;
	const float StartX = Canvas->ClipX - AmmoMargin - TotalWidth;
	const float MagazineY = Canvas->ClipY - AmmoMargin - MagazineHeight;
	// 备弹靠底对齐，看起来像 "/ 90" 跟在主数字后面
	const float ReserveY = MagazineY + MagazineHeight - ReserveHeight;

	const FLinearColor MagazineColor =
		(Rifle->GetAmmoInMagazine() > 0 || bReloading) ? AmmoTextColor : AmmoEmptyColor;

	DrawText(MagazineText, MagazineColor, StartX, MagazineY, Font, AmmoTextScale);
	DrawText(ReserveText, AmmoReserveColor, StartX + MagazineWidth + Gap, ReserveY, Font, ReserveScale);
}
