// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

/**
 * 敌人专用的动画实例。
 * 蒙太奇里的命名 notify（EnableBoxCollision / DisableBoxColliision）会自动调用
 * 这里的 AnimNotify_<名字> 函数，不需要在动画蓝图里连节点。
 */
UCLASS()
class SLASH_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** 对应 AM_Attack 里名为 EnableBoxCollision 的 notify */
	UFUNCTION()
	void AnimNotify_EnableBoxCollision();

	/** 对应 AM_Attack 里名为 DisableBoxColliision 的 notify（注意原拼写是三个 i） */
	UFUNCTION()
	void AnimNotify_DisableBoxColliision();

private:
	void SetWeaponCollision(ECollisionEnabled::Type CollisionEnabled);
};
