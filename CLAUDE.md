# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Unreal Engine 5.8 C++ third-person action game (`slash`), extended from DruidMech's UE5 Ultimate Developer Course "Slash" project. The runtime module is `slash`; targets are `slash` (Game) and `slashEditor` (Editor). There is no automated test suite.

The project currently has melee combat, enemy AI, attributes/HUD, breakables, ragdoll death, and a third-person rifle prototype (`ARangedWeapon`: pickup/switch, hitscan fire, ammo, reload, ADS state). Ammo HUD and rifle animations are not implemented yet.

Feature roadmap: the third-person shooting work is tracked in `docs/TPS_PLAN.md`.

## Build & run

Engine installed at `C:\Program Files\Epic Games\UE_5.8`.

```powershell
# Build the editor target (run from the repo root)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" slashEditor Win64 Development -Project="C:\Users\27933\Documents\Unreal Projects\slash\slash.uproject" -WaitMutex

# Build the game target
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" slash Win64 Development -Project="C:\Users\27933\Documents\Unreal Projects\slash\slash.uproject" -WaitMutex

# Launch the editor
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\27933\Documents\Unreal Projects\slash\slash.uproject"
```

- Quick C++ iteration uses Live Coding in the editor (`Ctrl+Alt+F11`). Header/`UCLASS`/reflection changes generally need a full rebuild and editor restart.
- `.sln`/`.slnx` files are gitignored/generated. Generate/open project files from the `.uproject` as usual.
- No lint or test command exists. Verify in PIE; the main map is `Content/Maps/Slashmap.umap`. Its GameMode is `BP_BirdGameMode` (set in the map's World Settings, not in `DefaultEngine.ini`).
- `.mcp.json` exposes Unreal's ModelContextProtocol endpoint at `http://127.0.0.1:8000/mcp`. It only works while the editor is running. Read-only inspection of Blueprints, assets, and the editor is safe; prefer inspection over edits unless the task calls for changes.

## Architecture

### Character hierarchy

- `ABaseCharacter : ACharacter, IHitInterface` (`Public/Characters/BaseCharacter.h`) owns the shared pieces: `UAttributeComponent`, `EquippedWeapon`, attack/hit-react/death montages, `GetHit`, and `SetWeaponCollisionEnabled`.
- `ASlashCharacter : ABaseCharacter` adds the spring-arm camera, player input, weapon pickup/equip/disarm, HUD creation/binding, and ragdoll death.
- `AEnemy : ABaseCharacter` adds `UPawnSensingComponent`, patrol/chase/attack state machine (`EEnemyState`), an overhead `UHealthBarComponent`, default weapon spawning, and death montage/lifespan.
- `AItem` is the pickup base (root is a `UStaticMeshComponent` named `ItemMesh`; has an overlap `Sphere` and a Niagara ember effect).
  - `AWeapon : AItem` is the melee weapon: a `WeaponBox` overlap triggers `BoxTrace`, damage, and `GetHit`. `AWeapon` has a `WeaponType` and a virtual `IsRanged()`.
  - `ARangedWeapon : AWeapon` is the hitscan rifle: camera-to-aim-point and muzzle-to-aim-point traces, `ApplyPointDamage` plus `Execute_GetHit`, magazine/reserve ammo, reload, recoil, and optional debug shot lines.
  - `AAmmo : AItem` is an ammo pickup; it adds reserve ammo when the player has a rifle.
- `ABreakableActor` implements `IHitInterface` and spawns an `AAmmo` actor from `AmmoClasses[0]` when hit.
- `ABird` is a standalone Pawn from the course, unrelated to the player.

### Damage pipeline (important)

1. Melee: `AWeapon::OnBoxOverlap` (`Private/Items/Weapons/Weapon.cpp:85`) runs a box trace, then calls `UGameplayStatics::ApplyDamage` **and** `IHitInterface::Execute_GetHit` on the hit actor.
2. `TakeDamage` updates `UAttributeComponent` health and, on the player, has a fallback death path.
3. Enemy **death and hit reactions are driven by `GetHit`**, not by `TakeDamage`. Enemy `TakeDamage` only reduces health, updates the health bar, and makes the enemy chase the instigator.
4. Therefore any new damage source (hitscan, projectile, explosion) must call `Execute_GetHit` after applying damage, or enemies will lose all health but never die. Alternatively, refactor death into the `TakeDamage` path first.
5. Player death (`ASlashCharacter::Die`) disables movement/input/collision and calls `StartRagdoll`; it deliberately skips the death montage.

### Input

- The character uses **legacy input** (`BindAxis`/`BindAction`) in `ASlashCharacter::SetupPlayerInputComponent` (`Private/Characters/SlashCharacter.cpp:99`).
- Despite `Config/DefaultInput.ini` setting Enhanced Input's `DefaultPlayerInputClass`/`DefaultInputComponentClass`, the C++ character does not use Enhanced Input mapping contexts. The `Content/Input/IMC_*` and `IA_*` assets are from the template and are not wired to this character.
- Action mappings live in `Config/DefaultInput.ini`: `Jump`, `Equip`, `Attack` (left mouse button), `Aim` (right mouse button), `Reload` (R), `SwitchWeapon` (Q). `SwitchWeapon` also picks up a nearby rifle when none is equipped. Add new mappings there, not in the BP.
- Names are `FName`-based and case-insensitive (`Moveforward` matches `MoveForward`; `thighsocket` matches `thighSocket`).
- Left mouse button is `Attack` and now dispatches by weapon: melee plays the attack montage, rifle calls `StartFire()`/`StopFire()`.

### HUD

- `ASlashHUD` creates `USlashOverlay` from `SlashOverlayClass` (BP_SlashHUD points it at `WBP_SlashOverlay`).
- `ASlashCharacter::BeginPlay` binds `UAttributeComponent` delegates to overlay setters, with a fallback that creates the overlay if the GameMode's HUD class is not configured.
- `USlashOverlay` uses `UPROPERTY(meta=(BindWidget))`. The widgets in `WBP_SlashOverlay` must use exactly these names: `HealthProgressBar`, `StaminaProgressBar`, `Count`.
- Enemy health bars use `UHealthBarComponent` (a `UWidgetComponent`) with `UHealthBar`; the widget is cached on first use.

### Animation

- `USlashAnimInstance` exposes `GroundSpeed`, `IsFalling`, and `CharacterState` to the player Animation Blueprint. BP_SlashCharacter uses `ABP_Military`, which has a `DefaultSlot` and linked `ABP_Military_Mainstates` (Idle/Run).
- Montages are played from C++ (e.g. `PlayMontageSection`, `PlayAttackMontage`). A montage only plays if the ABP has a matching Slot.
- Rifle idle: `ASlashCharacter::PlayRifleIdle` plays `RifleIdleAnimation` on the ABP's `DefaultSlot` as a looping dynamic montage; `Tick` stops it when moving, falling, or dead. There is no upper-body slot yet, so the pose is full-body while stationary.
- `UEnemyAnimInstance` implements named anim notifies `AnimNotify_EnableBoxCollision` and `AnimNotify_DisableBoxColliision` (note the three `i`s) to toggle the equipped weapon's box collision. Montage notify names must match exactly.

### Assets and Blueprints

- Player: `Content/Bludeprint/Characters/BP_SlashCharacter` uses mesh `SKM_QuantumCharacter` and anim BP `ABP_Military`. Player mesh sockets: `Weapon` and `RightHandSocket` (both on `hand_r`), `SpineSocket`, `thighsocket`.
- Enemies: `BP_BaseEnemy`, `BP_Enemy`, `BP_EnemySwat`. The Swat uses a Mixamo skeleton and has `RightHandSocket`; its head bone is `Head` (player head bone is `head`).
- Melee weapons: `BP_Weapon`, `BP_EnemyWeapon`. Rifle: `BP_Rifle` (child of `ARangedWeapon`, `SM_Rifle_Olive` mesh, `Muzzle` at local `(0,57,15)`). Ammo: `BP_Base_Ammo`. HUD: `WBP_SlashOverlay`, `WBP_HealthBar`.
- Rifle-related assets already exist: `Content/QuantumCharacter/Mesh/Rifle/SM_Rifle_Olive` (static mesh, long axis +Y, ~84 units), `Content/QuantumCharacter/Materials/M_Rifle_Olive`, and Mannequin rifle/pistol anims under `Content/Characters/Mannequins/Anims/{Rifle,Pistol}` (`MM_Rifle_Fire`, `MM_Rifle_Reload`, `MM_Rifle_Equip`, `AO_Rifle`, etc.). The retargeter `Content/Bludeprint/Characters/Rigs/RTG_Military` targets the player skeleton.
- Imported rifle idle: `Content/Bludeprint/Characters/Animations/Rifle/A_IdleAiming_XBot` (XBot skeleton) and `A_IdleAiming_XBot_Retargeted` (player skeleton, 2.1s). `BP_SlashCharacter.RifleIdleAnimation` points at the retargeted sequence.
- Headless animation helpers: `Scripts/import_idle_aiming.py` imports the Mixamo FBX and retargets it with `RTG_XBot`; `Scripts/check_rifle_idle.py` queries the PIE AnimInstance to confirm the slot animation is playing.
- Most `Content/Fab`, `Content/Downloads`, `Content/Megaplant_Library`, `Content/Dungeon`, and `Content/QuantumCharacter` assets are third-party packs. Do not reorganize them.

### Collision conventions

- Player mesh: object type `WorldDynamic`; ignores all channels; blocks `Visibility`; overlaps `WorldDynamic` (so enemy weapon boxes can hit the player).
- Enemy mesh: `WorldDynamic`; blocks `Visibility`; ignores `Camera`.
- `AWeapon::WeaponBox`: `WorldDynamic`; overlaps everything except `Pawn` and `Camera`; collision is enabled/disabled by anim notifies.
- Player capsule ignores `Camera` so the third-person spring arm does not get pushed in.
- For hitscan traces, use `ECC_Visibility` and ignore the shooter.

## Known quirks and pitfalls

- The folder `Source/slash/Public/Breaakable/` (and `Private/Breaakable/`) is intentionally misspelled. Follow the existing spelling when adding breakable code.
- `AWeapon::Equip` is virtual, but `AWeapon::Damage` is still private with a `GetDamage()` getter.
- `AItem::ItemMesh` is the root StaticMeshComponent. Set a weapon's mesh on that component in the Blueprint.
- `AAmmo::OnSphereOverlap` now grants reserve ammo, but only when `ASlashCharacter::HasRifle()` is true; otherwise the pickup stays in the world.
- `AEnemy::TakeDamage` now null-checks `EventInstigator` before assigning `CombatTarget`.
- Enemy AI tuning: `CombatRadius = 1500` and `PawnSensing` sight radius `1000`. Ranged enemy combat needs larger values, otherwise an enemy hit from farther away immediately loses interest.
- `ASlashCharacter::EquipWeapon` holsters the non-active weapon (`SpineSocket` for the rifle, `thighsocket` for melee) and `SwitchWeapon` toggles between them; the rifle sets `CharacterState = ECS_Unequipped` so the existing melee ABP keeps working until a rifle animation layer exists.
- `ARangedWeapon::bDrawDebugShot` draws the shot trace in PIE. It is enabled on `BP_Rifle` for debugging; disable it before shipping.
- Importing an animation-only Mixamo FBX: Interchange rejects it as empty, so run `Interchange.FeatureFlags.Import.FBX 0` first (the helper script does this). The legacy importer also rejects a target skeleton that has a `root` bone if the FBX has no root track; the XBot skeleton has no `root` bone, so it works there and can then be retargeted with `RTG_XBot`.
- `UAttributeComponent` regenerates stamina in `TickComponent` and broadcasts health/stamina/gold/souls changes through multicast delegates. Health does not regenerate.
- `Config/DefaultEngine.ini` sets `bIsWorldPartitioned=False` for navigation and uses a dynamic Recast navmesh; enemy pathing depends on that.

## Repository constraints

- Git tracks `.uasset`/`.umap` as binary and does **not** use Git LFS. GitHub's 100 MiB hard file limit is nearly reached: the largest tracked file is an external actor under `Content/__ExternalActors__/Maps/Slashmap/...` at about 99 MB. Do not add or modify large binary assets casually. Check before committing:
  ```bash
  git ls-tree -r -l HEAD | sort -k4 -n | tail
  ```
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, and generated `.sln`/`.slnx` files are gitignored. Do not commit them.

## Workflow checklist

- New C++ class: header in `Source/slash/Public/...`, implementation in `Source/slash/Private/...`, use the `SLASH_API` macro, include `...generated.h` last, then build.
- New input: add the mapping to `Config/DefaultInput.ini`, bind it in `ASlashCharacter::SetupPlayerInputComponent`.
- New HUD element: add a `BindWidget` property and setter to `USlashOverlay`, then create a widget with the identical name in `WBP_SlashOverlay`.
- New gameplay behavior: verify in PIE on `Slashmap`; there are no automated tests to run.