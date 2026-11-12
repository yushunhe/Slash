import unreal

FBX_PATH = r"C:\Users\27933\Documents\Unreal Projects\slash\Assets\Animations\Firing Rifle.fbx"
DEST_PATH = "/Game/Bludeprint/Characters/Animations/Rifle"
IMPORTED_NAME = "A_FiringRifle_XBot"
IMPORTED_PATH = f"{DEST_PATH}/{IMPORTED_NAME}"

# XBot 骨架没有 root 骨骼，Mixamo 动画可以直接导入；
# 然后用项目已有的 RTG_XBot 重定向到玩家骨架。
SOURCE_SKELETON = unreal.load_asset("/Game/Mixamo/XBot/SK_X_Bot_Skeleton")
SOURCE_MESH = unreal.load_asset("/Game/Mixamo/XBot/SKM_X_Bot")
TARGET_MESH = unreal.load_asset("/Game/QuantumCharacter/Mesh/SKM_QuantumCharacter")
RETARGETER = unreal.load_asset("/Game/Mixamo/XBot/RIgs/RTG_XBot")


def log(msg):
    unreal.log(f"[FiringRifle] {msg}")


def import_fbx():
    # Interchange 会把纯动画 FBX 判成空内容，切回 legacy FBX 导入器
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")

    task = unreal.AssetImportTask()
    task.filename = FBX_PATH
    task.destination_path = DEST_PATH
    task.destination_name = IMPORTED_NAME
    task.automated = True
    task.save = True
    task.replace_existing = True

    options = unreal.FbxImportUI()
    options.import_mesh = False
    options.import_animations = True
    options.import_materials = False
    options.import_textures = False
    options.import_as_skeletal = True
    options.skeleton = SOURCE_SKELETON

    options.anim_sequence_import_data = unreal.FbxAnimSequenceImportData()

    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def inspect_imported():
    asset = unreal.EditorAssetLibrary.load_asset(IMPORTED_PATH)
    if not asset:
        log(f"ERROR: import produced no asset at {IMPORTED_PATH}")
        return None

    log(f"Imported class = {asset.get_class().get_name()}")
    if isinstance(asset, unreal.AnimSequence):
        try:
            skel = asset.get_editor_property("skeleton")
            log(f"Skeleton = {skel.get_path_name() if skel else 'None'}")
        except Exception as exc:
            log(f"Could not read skeleton: {exc}")
        log(f"Length = {asset.get_play_length()}")
    return asset


def retarget(asset):
    if not asset:
        return

    asset_data = unreal.EditorAssetLibrary.find_asset_data(IMPORTED_PATH)
    if not asset_data.is_valid():
        log("ERROR: AssetData invalid")
        return

    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.set_editor_property("assets_to_retarget", [asset_data])
    inputs.set_editor_property("source_mesh", SOURCE_MESH)
    inputs.set_editor_property("target_mesh", TARGET_MESH)
    inputs.set_editor_property("ik_retarget_asset", RETARGETER)
    inputs.set_editor_property("target_path", DEST_PATH)
    inputs.set_editor_property("prefix", "")
    inputs.set_editor_property("suffix", "_Retargeted")
    inputs.set_editor_property("use_source_path", False)
    inputs.set_editor_property("include_referenced_assets", False)
    inputs.set_editor_property("overwrite_existing_files", True)

    results = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    log(f"Retarget produced {len(results)} asset(s)")
    for result in results:
        log(f"  -> {result.package_name}")

    unreal.EditorAssetLibrary.save_directory(DEST_PATH, only_if_is_dirty=False, recursive=True)
    log("Saved destination directory")


if __name__ == "__main__":
    log("Starting Firing Rifle import/retarget")
    import_fbx()
    imported = inspect_imported()
    if imported:
        retarget(imported)
    else:
        log("ERROR: import failed; no retarget attempted")
    unreal.EditorAssetLibrary.save_directory(DEST_PATH, only_if_is_dirty=False, recursive=True)