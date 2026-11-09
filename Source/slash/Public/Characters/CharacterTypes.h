#pragma once


UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	ECS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECS_EquippedOneHandedWeapon UMETA(DisplayName = "Equipped One-Handed Weapon"),
	ECS_EquippedTwoHandedWeapon UMETA(DisplayName = "Equipped Two-Handed Weapon")
};

UENUM(BlueprintType)
enum class EActionState : uint8
{
	EAS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	EAS_Attacking UMETA(DisplayName = "Attacking"),
	EAS_EquippingWeapon UMETA(DisplayName = "Equipping Weapon"),
	EAS_HitReaction UMETA(DisplayName = "Hit Reaction"),
	EAS_Dead UMETA(DisplayName = "Dead"),
};

UENUM(BlueprintType)
enum class EDeathPose : uint8
{
	EDP_Alive UMETA(DisplayName = "Alive"),
	EDP_DeadFront UMETA(DisplayName = "Dead Front"),
	EDP_DeadBack UMETA(DisplayName = "Dead Back"),
	EDP_DeadLeft UMETA(DisplayName = "Dead Left"),
	EDP_DeadRight UMETA(DisplayName = "Dead Right"),

	EDP_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	EES_NoState UMETA(DisplayName = "NoState"),

	EES_Dead UMETA(DisplayName = "Dead"),
	EES_Patrolling UMETA(DisplayName = "Patrolling"),
	EES_Chasing UMETA(DisplayName = "Chasing"),
	EES_Attacking UMETA(DisplayName = "Attacking"),
	EES_Engaged UMETA(DisplayName = "Engaged"),

};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Melee UMETA(DisplayName = "Melee"),
	EWT_Ranged UMETA(DisplayName = "Ranged")
};

UENUM(BlueprintType)
enum class ECombatMode : uint8
{
	ECM_Melee UMETA(DisplayName = "Melee"),
	ECM_Ranged UMETA(DisplayName = "Ranged")
};