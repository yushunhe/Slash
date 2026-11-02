// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SlashHUD.h"
#include "Blueprint/UserWidget.h"
#include "HUD/SlashOverlay.h"

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
