import unreal


def log(msg):
    unreal.log(f"[RifleIdleCheck] {msg}")


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
log(f"game world = {world.get_name() if world else 'None'}")
if not world:
    raise SystemExit

pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
log(f"pawn = {pawn.get_name() if pawn else 'None'}")
if not pawn:
    raise SystemExit

asset = unreal.load_asset(
    "/Game/Bludeprint/Characters/Animations/Rifle/A_IdleAiming_XBot_Retargeted"
)
log(f"asset = {asset.get_name() if asset else 'None'}")

meshes = pawn.get_components_by_class(unreal.SkeletalMeshComponent)
log(f"skeletal mesh components = {len(meshes)}")
for mesh in meshes:
    anim = mesh.get_anim_instance()
    if not anim:
        log(f"mesh {mesh.get_name()} has no anim instance")
        continue

    try:
        playing = anim.is_playing_slot_animation(asset, "DefaultSlot")
    except Exception as exc:
        playing = f"error: {exc}"
    log(f"mesh={mesh.get_name()} anim={anim.get_name()} playing={playing}")

    try:
        montage = anim.get_current_active_montage()
        log(f"active montage = {montage.get_name() if montage else 'None'}")
    except Exception as exc:
        log(f"active montage read failed: {exc}")