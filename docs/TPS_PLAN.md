# Slash 项目：第三人称射击（TPS）改造计划

- 状态：Phase 0–2 + 步枪 Idle Aiming 动画已实现（2026-11-09）；HUD、移动/瞄准动画层、敌人远程 AI 待做
- 日期：2026-11-09
- 适用范围：在现有 Slash 近战框架上增加「近战 + 步枪」的混合 TPS 玩法
- 相关文档：`CLAUDE.md`（项目架构与开发约定）

## 实现进度

- [x] Phase 0：崩溃修复、输入映射、步枪挂点
- [x] Phase 1：`ARangedWeapon`、`BP_Rifle`、拾取/切枪
- [x] Phase 2：Hitscan、伤害、`Execute_GetHit`、后坐力、调试射线
- [ ] Phase 3：弹匣/备弹/换弹逻辑已完成；弹药 HUD/准星未做
- [x] Phase 4（动画基础）：ADS 相机/移速/散布逻辑；Idle Aiming 已导入、重定向到玩家骨架，持枪站立时播放
- [ ] Phase 4（完整）：移动/跳跃时保持持枪姿势、上身动画层、Aim Offset
- [ ] Phase 5：敌人远程 AI
- [ ] Phase 6：打磨

## 0. 目标与假设

**目标**：保留现有剑/近战玩法，新增一把可拾取、可切换的步枪，支持腰射/瞄准射击、弹匣与备弹、换弹、命中反馈和准星 HUD。

**假设**：

- 先做玩家射击闭环，敌人远程 AI 放到后期。
- 命中判定先用 Hitscan（射线），榴弹/火箭弹等抛射物以后再做。
- 输入继续使用项目现有的 legacy 输入，不强制迁移 Enhanced Input。
- 如果改成纯 TPS（不保留近战），可跳过 Phase 1 的剑/枪切换，其余阶段不变。

## 1. 现状盘点

### 代码侧

- `ASlashCharacter`：第三人称弹簧臂相机、移动/跳跃/拾取/近战/布娃娃死亡。
- `AWeapon`：近战 `WeaponBox` 重叠 + BoxTrace 结算伤害（`Source/slash/Private/Items/Weapons/Weapon.cpp:85`）。
- `AEnemy`：巡逻/追击/近战攻击，`PawnSensing` 只认 `SlashCharacter` 标签。
- `UAttributeComponent`：Health/Stamina/Gold/Souls，**没有弹药**。
- `USlashOverlay`：只有血条/体力条/金币文本。
- 输入：legacy 输入，`ASlashCharacter::SetupPlayerInputComponent` 在 `Source/slash/Private/Characters/SlashCharacter.cpp:99`；`Config/DefaultInput.ini` 已有 Jump/Equip/Attack，**Attack 绑在鼠标左键**。
- 弹药拾取：`AAmmo::OnSphereOverlap` 目前只播音效然后 `Destroy()`（`Source/slash/Private/Items/Ammo.cpp:6-15`），没有加弹。

### 资产侧（可直接复用）

- 步枪模型：`Content/QuantumCharacter/Mesh/Rifle/SM_Rifle_Olive` + `M_Rifle_Olive`。
- 步枪/手枪动画：`Content/Characters/Mannequins/Anims/Rifle`、`.../Pistol`，含 `MM_Rifle_Fire`、`MM_Rifle_Reload`、`MM_Rifle_Equip`、`AO_Rifle`，以及 `MM_Pistol_Fire_Montage`。
- 重定向：`RTG_Military` 已能从 Quinn/Mannequin 骨架重定向到玩家骨架。
- 玩家骨架 socket：`Weapon`、`RightHandSocket`（都在 `hand_r`）、`SpineSocket`、`thighsocket`。
- 玩家动画蓝图：`ABP_Military`（含 `DefaultSlot`）+ `ABP_Military_Mainstates`（Idle/Run）。
- 爆头骨骼：玩家 `head`，敌人 `Head`（FName 比较不区分大小写）。

### 缺口

枪械类、射击判定、弹匣/备弹/换弹、瞄准相机、准星/弹药 HUD、步枪动画层、敌人远程 AI。

## 2. 关键设计决策

| 事项 | 建议方案 | 理由 |
|---|---|---|
| 玩法范围 | 保留近战，步枪作为第二武器 | 现有近战/敌人/关卡不用推翻 |
| 命中判定 | Hitscan（射线） | 最容易做准、好调、性能好；抛射物以后再做 |
| 武器类 | 新建 `ARangedWeapon : AWeapon` | 复用拾取/装备/挂点，不污染近战逻辑 |
| 角色状态 | 保留 `ECharacterState`，新增 `ECombatMode { Melee, Ranged }` + `bIsAiming` | 避免改现有 ABP 的枚举分支，降低回归风险 |
| 输入 | 继续 legacy，新增 `Aim/Reload/SwitchWeapon`，左键按当前武器分发 | 现有输入能用，迁移 Enhanced Input 放到后期 |
| 动画 | 上身 Layered Blend + 步枪 AimOffset，不做全量状态机 | MVP 先让枪能打，动画后补 |
| 敌人 | Phase 5 可选，先做玩家射击闭环 | 先把核心手感做出来 |

## 3. 分阶段实施

### Phase 0：准备（0.5–1 天）

1. **修必现崩溃**：`Source/slash/Private/Enemy/Enemy.cpp:467` 的 `CombatTarget = EventInstigator->GetPawn();` 没有判空，枪械伤害一旦没有 Instigator 就会崩。改成：

   ```cpp
   CombatTarget = EventInstigator ? EventInstigator->GetPawn() : nullptr;
   if (CombatTarget) { ChaseTarget(); }
   ```

2. `Config/DefaultInput.ini` 加映射：

   ```ini
   +ActionMappings=(ActionName="Aim",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=RightMouseButton)
   +ActionMappings=(ActionName="Reload",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=R)
   +ActionMappings=(ActionName="SwitchWeapon",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=Q)
   ```

   **左键继续叫 Attack**，不要新增 Fire，否则近战和射击会抢同一个键。

3. 确认 `SM_Rifle_Olive` 的朝向：模型长轴是 **+Y**（包围盒 Y 约 -27 到 +57），枪口在 +Y 方向；玩家 `Weapon` socket 在 `hand_r`，相对变换已经比较接近握枪点。先在 BP 里试挂，必要时加一个 `RifleSocket` 微调。

4. 在 `SM_Rifle_Olive` 上或 BP_Rifle 里加 `MuzzleSocket`，本地位置大约 `(0, 57, 20)`，按实际模型微调。

**验收**：工程能编译，输入映射在 Project Settings 里能看到，步枪能挂到手上升降。

### Phase 1：枪械类与装备/切换（1–2 天）

**新增 `ARangedWeapon`**

- 继承 `AWeapon`，复用 `Equip` / `AttachMeshToSocket`。
- 新增：
  - `USceneComponent* Muzzle`
  - `Damage / FireInterval / Range / HipSpreadDeg / AimSpreadDeg / HeadshotMultiplier`
  - `MagazineSize / AmmoInMagazine / ReserveAmmo`
  - `StartFire() / StopFire() / Reload() / AddReserveAmmo() / CanFire()`
  - `FOnAmmoChanged` 委托
- 把 `AWeapon::Equip` 改成 `virtual`，在 `ARangedWeapon` 里补上弹药 HUD 绑定。
- `AWeapon::Damage` 目前是 private，改成 `protected` 或加 `GetDamage()`。

**角色侧**

- `ASlashCharacter` 增加：
  - `ARangedWeapon* EquippedRifle`
  - `AWeapon* EquippedMeleeWeapon`
  - `ECombatMode CombatMode`
  - `bool bHasRifle / bIsAiming`
- `EquipWeapon` 按武器类型分支：
  - 近战：维持现有 `CharacterState = ECS_EquippedOneHandedWeapon`。
  - 步枪：`bHasRifle = true`，`CombatMode = Ranged`，`CharacterState = ECS_Unequipped`（剑收回背上）。
- **不要 Destroy 旧武器**：`EKeyPressed` 现在会 `EquippedWeapon->Destroy()`（`SlashCharacter.cpp:252` 附近），改成挂到 `SpineSocket` / `thighsocket`。
- `SwitchWeapon()`：Q 键在剑和步枪之间切换，切换时把非当前武器挂到背上/腰上。

**验收**：E 拾取步枪 → 自动拿在手上；Q 切换剑/枪；近战攻击、装备动画不回归。

### Phase 2：射击核心（2–3 天）

**输入分发**

- `Attack` 的 `IE_Pressed` 改成 `PrimaryActionPressed()`：
  - 近战模式 → 调原 `Attack()`。
  - 步枪模式 → `EquippedRifle->StartFire()`。
- 新增 `IE_Released` → `PrimaryActionReleased()` → `StopFire()`，支持全自动。

**两段式射线（关键）**

1. 从相机中心向前打一条 `ECC_Visibility` 射线，得到准星指向的 `AimPoint`。
2. 从枪口向 `AimPoint` 再打一条射线，避免「隔着墙打中敌人」。
3. 对第二条射线的命中 Actor：
   - `UGameplayStatics::ApplyPointDamage(...)`
   - **再手动调 `IHitInterface::Execute_GetHit()`**

这一步不能省：现有敌人的死亡是由 `GetHit_Implementation` 驱动的，只调 `ApplyDamage` 只会掉血/追击，不会播死亡。

核心骨架：

```cpp
void ARangedWeapon::FireOnce()
{
    if (!CanFire()) return;

    ASlashCharacter* Character = Cast<ASlashCharacter>(GetOwner());
    if (!Character) return;

    const UCameraComponent* Camera = Character->GetFollowCamera();
    const FVector CamStart = Camera->GetComponentLocation();
    const FVector CamEnd = CamStart + Camera->GetForwardVector() * Range;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(RifleShot), false, this);
    Params.AddIgnoredActor(Character);

    FHitResult CamHit;
    GetWorld()->LineTraceSingleByChannel(CamHit, CamStart, CamEnd, ECC_Visibility, Params);
    const FVector AimPoint = CamHit.bBlockingHit ? CamHit.ImpactPoint : CamEnd;

    const float Spread = Character->IsAiming() ? AimSpreadDeg : HipSpreadDeg;
    const FVector MuzzleLoc = Muzzle->GetComponentLocation();
    const FVector ShotDir = FMath::VRandCone(
        (AimPoint - MuzzleLoc).GetSafeNormal(),
        FMath::DegreesToRadians(Spread));

    FHitResult ShotHit;
    GetWorld()->LineTraceSingleByChannel(
        ShotHit, MuzzleLoc, MuzzleLoc + ShotDir * Range, ECC_Visibility, Params);

    --AmmoInMagazine;
    OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);

    if (ShotHit.GetActor())
    {
        const float FinalDamage = ShotHit.BoneName.ToString().Equals(TEXT("head"), ESearchCase::IgnoreCase)
            ? Damage * HeadshotMultiplier : Damage;

        UGameplayStatics::ApplyPointDamage(
            ShotHit.GetActor(), FinalDamage, ShotDir, ShotHit,
            GetInstigatorController(), this, UDamageType::StaticClass());

        if (IHitInterface* HitInterface = Cast<IHitInterface>(ShotHit.GetActor()))
        {
            HitInterface->Execute_GetHit(ShotHit.GetActor(), ShotHit.ImpactPoint);
        }
    }

    // 枪口火光/声音/弹着点 FX
}
```

**同队/自伤过滤**

- 忽略 `GetOwner()` 和 `this`。
- 若 `GetOwner()->ActorHasTag("Enemy")` 且命中 Actor 也有 `Enemy`，直接 return（和近战 `ActorIsSameType` 一致）。

**后坐力**

- 每发 `AddControllerPitchInput(-RecoilPitch)` + 少量随机 Yaw；或用 CameraShake。

**验收**：射击敌人能掉血、受击、死亡；隔墙打不中；不会打自己；全自动按住能连发。

### Phase 3：弹药/换弹/HUD（1–2 天）

- `ARangedWeapon` 内维护 `AmmoInMagazine / ReserveAmmo`。
- `Reload()`：弹匣不满且备弹 > 0 → 播 `MM_Rifle_Reload` 蒙太奇（需自己从 AnimSequence 建 Montage），计时结束后补满。
- 空仓开火 → 播 `MM_Rifle_DryFire`，可选自动换弹。
- **`AAmmo` 改造**：
  - 加 `int32 AmmoAmount`（`AmmoCount` 目前没被使用）。
  - `OnSphereOverlap` 不再直接 `Destroy()`，改为调用 `SlashCharacter->AddAmmo(AmmoAmount)`，成功加弹后再销毁。
  - `ASlashCharacter::AddAmmo` 转发给 `EquippedRifle->AddReserveAmmo()`。
- **HUD** 在 `USlashOverlay` 加：
  - `UTextBlock* AmmoText`
  - `UTextBlock* ReserveAmmoText`
  - `UImage* Crosshair`
  - `UImage* HitMarker`（可选）
  - `SetAmmo(int32 Mag, int32 Reserve)`
  - 注意 WBP 里的控件命名要和 `BindWidget` 完全一致。

**验收**：显示 `30 / 120`；R 换弹；打空后不能开火；拾取弹药备弹增加；爆箱子的弹药有去处。

### Phase 4：瞄准与动画（2–3 天）

**ADS 相机**

- 右键 `Aim`：FOV 90 → 65；`CameraBoom->TargetArmLength` 300 → 150；加一点 `SocketOffset` 做越肩视角。
- `bUseControllerRotationYaw = true`、`bOrientRotationToMovement = false`，让角色朝准星方向；松右键恢复。
- `MaxWalkSpeed` 600 → 300 左右；瞄准时散布收紧。
- 死亡/切武器时记得恢复这些值。

**动画**

- 用 `RTG_Military` 把 `AO_Rifle`、`MF_Rifle_Idle_ADS`、Rifle Jog/Walk、`MM_Rifle_Fire`、`MM_Rifle_Reload` 从 Mannequin 骨架重定向到玩家骨架。
- 在 `ABP_Military` 里加 `bHasRifle` / `bIsAiming`，在现有 locomotion 之后用 **Layered Blend Per Bone**（建议 `spine_03`，深度 2–3）叠一个步枪上身层。
- 蒙太奇必须经过 `DefaultSlot`，否则打不出来。
- 全自动不要每发播全身蒙太奇，否则会锁移动；用上身 Slot 或只做枪口特效 + 后坐力。

**如果没有时间做完整重定向**：先用现有近战 locomotion，步枪只挂手 + 播 Fire 蒙太奇，也能玩，只是跑动时姿势会有点怪。

**验收**：右键越肩瞄准；角色朝准星；移动速度/散布变化；射击时上身有动作；跑动不锁死。

### Phase 5：敌人远程 AI（可选，3–4 天）

- 给 `AEnemy` 加 `bool bIsRanged`、`RangedRange`、`FireInterval`、`BurstCount`、`Accuracy`；或新建 `ARangedEnemy : AEnemy`。
- 给 `BP_EnemySwat` 配一把低伤害 BP_Rifle，挂到 `RightHandSocket`。
- `Attack()` 分支：远程时面向玩家、做视线检测（`LineTraceSingleByChannel` 从敌人眼睛到玩家），有视线才开火。
- **注意现有 AI 参数**：
  - `CombatRadius = 1500`，步枪射程 10000；玩家在 3000 外开枪，敌人受击后 `CheckCombatTarget` 会立刻 `LoseInterest`。远程玩法要把 `CombatRadius` 提到 3000–5000。
  - `PawnSensing->SightRadius = 1000`；远程敌人建议提到 2000–3000。
- 动画可先复用近战姿势 + 枪口 FX，追求效果再给 Mixamo 骨架做重定向。

**验收**：敌人在中距离能射击玩家；玩家可利用掩体；敌人不会隔墙开枪。

### Phase 6：打磨（可选，3–5 天）

武器数据表/DataAsset、后坐力曲线、伤害衰减、命中音效、弹壳抛出、枪口灯光、镜头震动、准星扩散、切枪动画、弹药类型、命中反馈、平衡数值。

## 4. 文件改动清单

| 类型 | 文件 | 改动 |
|---|---|---|
| 新增 | `Source/slash/Public/Items/Weapons/RangedWeapon.h` | 枪械类声明 |
| 新增 | `Source/slash/Private/Items/Weapons/RangedWeapon.cpp` | 射击/换弹/弹药 |
| 新增 | `Content/Bludeprint/Items/BP_Rifle.uasset` | 步枪蓝图，挂 `SM_Rifle_Olive` + MuzzleSocket |
| 新增 | `AM_Rifle_Fire` / `AM_Rifle_Reload` | 从 AnimSequence 建 Montage |
| 修改 | `Source/slash/Public/Characters/CharacterTypes.h` | 加 `ECombatMode`（或 `ECS_EquippedRifle`） |
| 修改 | `Source/slash/Public/Characters/SlashCharacter.h` / `.cpp` | 输入分发、瞄准、切枪、弹药接口 |
| 修改 | `Source/slash/Public/Characters/SlashAnimInstance.h` / `.cpp` | 暴露 `bIsAiming` / `bHasRifle` / AimYaw/Pitch |
| 修改 | `Source/slash/Public/Items/Weapons/Weapon.h` / `.cpp` | `Equip` 改 virtual，Damage 改 protected |
| 修改 | `Source/slash/Public/Items/Ammo.h` / `.cpp` | 加弹量、调用角色 `AddAmmo` |
| 修改 | `Source/slash/Public/HUD/SlashOverlay.h` / `.cpp` | 弹药/准星/命中标记 |
| 修改 | `Source/slash/Public/Enemy/Enemy.h` / `.cpp` | 修 `EventInstigator` 判空；可选远程 AI |
| 修改 | `Config/DefaultInput.ini` | Aim/Reload/SwitchWeapon |
| 修改 | `BP_SlashCharacter` | 蒙太奇、步枪默认值 |
| 修改 | `WBP_SlashOverlay` | 新控件，名字对齐 `BindWidget` |
| 修改 | `ABP_Military` | 步枪上身层/AimOffset |
| 可选 | `BP_EnemySwat` | 敌人持枪 |

## 5. 推荐初始数值

| 参数 | 建议值 |
|---|---|
| 单发伤害 | 20 |
| 射速 | 600 RPM（`FireInterval = 0.1s`） |
| 弹匣/备弹 | 30 / 120 |
| 换弹时间 | 2.2s |
| 射程 | 10000（约 100m） |
| 腰射散布 | 1.5° |
| 瞄准散布 | 0.3° |
| 爆头倍率 | 2.0x |
| 后坐力 | Pitch 0.4–0.6°，Yaw ±0.2–0.4° |
| ADS FOV | 65（默认 90） |
| ADS 臂长 | 150（默认 300） |
| 瞄准移速 | 300（默认 600） |
| 敌人远程伤害 | 8–12，三连发，间隔 0.15s，散布 3° |

## 6. 容易踩的坑

1. **敌人死亡不是 `TakeDamage` 驱动的**：`ApplyPointDamage` 后必须调 `Execute_GetHit`，否则敌人只掉血不死。
2. **`Enemy.cpp:467` 空指针**：远程伤害前先修，否则某些伤害来源直接崩。
3. **`AAmmo` 现在是空实现**：只销毁不给弹，必须先改，否则弹药系统没有来源。
4. **左键冲突**：`Attack` 已经绑在左键，不要再加 `Fire`；在 `PrimaryActionPressed` 里按武器分发。
5. **`AWeapon::Equip` 不是 virtual**：要重写就必须先加 `virtual`。
6. **切枪不要 Destroy 旧武器**：`EKeyPressed` 现在会销毁，改成挂背/挂腰。
7. **全自动别播全身蒙太奇**：会锁住移动；用上身 Slot 或纯特效 + 后坐力。
8. **步枪模型长轴是 +Y**：挂到 `Weapon` socket 后要调相对旋转，枪口放在本地 `(0, ~57, ~20)` 附近。
9. **动画需要重定向**：步枪动画在 Mannequin 骨架上，玩家是 Military 骨架；用 `RTG_Military` 重定向，否则播不出来。
10. **敌人交战半径**：`CombatRadius` 1500 对步枪太短，远程玩法要调大，否则打一枪敌人就「脱战」。
11. **相机射线忽略自己**：`Params.AddIgnoredActor(Character)`，玩家网格是 Block Visibility 的，不忽略会打到自己。
12. **HUD 控件名**：`BindWidget` 必须和 WBP 里的名字完全一致，改完要编译检查。

## 7. 排期与验收

| 阶段 | 内容 | 预估 | 验收标准 |
|---|---|---|---|
| 0 | 修复 + 输入 + 步枪挂点 | 0.5–1 天 | 编译通过，输入映射生效 |
| 1 | 枪械类 + 装备/切换 | 1–2 天 | E 拾取、Q 切换、近战不回归 |
| 2 | 射击判定 + 伤害 + FX | 2–3 天 | 打敌人掉血/受击/死亡，不穿墙 |
| 3 | 弹药 + 换弹 + HUD | 1–2 天 | `30/120`、R 换弹、拾取加弹 |
| 4 | ADS + 动画 | 2–3 天 | 越肩瞄准、上身动作、移速变化 |
| 5 | 敌人远程 AI | 3–4 天 | 敌人中距离射击、掩体有效 |
| 6 | 打磨 | 3–5 天 | 手感/音效/平衡 |

**MVP（Phase 0–3）约 4.5–8 人日**，就能得到「能跑、能切枪、能打、有弹药和 HUD」的闭环。完整玩法（含动画和敌人远程 AI）约 3–4 周；按兼职/学习节奏可能更长。

## 8. 建议的第一步

不要一上来做动画和敌人 AI。先做 **Phase 0 + Phase 1 + Phase 2 的最小闭环**：

1. 新建 `ARangedWeapon`，用 `SM_Rifle_Olive` 做 `BP_Rifle`。
2. 实现两段式射线 + `ApplyPointDamage` + `Execute_GetHit`。
3. 在 PIE 里用 `DrawDebugLine` 验证命中，确认能打死 `BP_EnemySwat`。
4. 再加弹药/HUD，最后做 ADS 和动画重定向。
5. 敌人远程 AI 放到最后，避免同时调玩家手感和 AI。