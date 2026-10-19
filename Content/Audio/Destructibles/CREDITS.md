# 木板 / 纸板 破裂音效 — 来源与授权

本目录共 **88** 个 wav，全部为 `44100 Hz / 16-bit / 立体声`，可直接拖入 UE 导入为 SoundWave。

## 目录

```
Destructibles/
├── Wood/        32 个 —— 木板断裂、劈裂、木箱碎裂、板材垮塌、树枝折断
└── Cardboard/   56 个 —— 纸箱撕裂、拆箱、压溃，纸张撕裂、裁切
```

命名规则：`SFX_<类型>_<序号>.wav`。同前缀多序号即同一类音效的多个变体，适合做随机选择（Random → Play Sound）。

## 来源与授权

| 前缀 | 来源 | 授权 | 是否需署名 |
|---|---|---|---|
| `SFX_WoodBreak*` `SFX_WoodSplinter*` `SFX_WoodBoxShatter*` `SFX_TableBreak*` `SFX_Cardboard*` `SFX_PaperTear*` | [freesoundsite.com](https://freesoundsite.com/) | 站点声明「无版权限制、可商用」 | 站点称不需要 |
| `SFX_Mixkit_*` | [mixkit.co](https://mixkit.co/free-sound-effects/) | Mixkit Free License，可商用 | 不需要 |
| `SFX_SB_*` | [soundbible.com](https://soundbible.com/) | **CC BY 3.0** | **需要** |

### ⚠️ 两点需要你确认

1. **`SFX_SB_*` 四个文件必须署名**（CC BY 3.0）。若要发布游戏，请在制作人员名单里加上：

   > "Branch Break", "Branch Wood Stress Cracking", "Wooden Thump" — SoundBible.com (CC BY 3.0)
   > "Smashing" — Yuri Santana, via SoundBible.com (CC BY 3.0)

   不想署名就直接删掉这 4 个文件，其余 84 个不受影响。

2. **freesoundsite.com 是聚合站，不是原始录音方。** 它的文件名为哈希串，其中部分内容与 Pixabay 的音效（文件名形如 `075584_cardboard-rip-1`）对得上，推测是转载 Pixabay 的内容。Pixabay 本身授权宽松（可商用、免署名），但我没有逐条核对每个文件的原始出处。**如果这个项目要商业化发行，建议只用 `SFX_Mixkit_*`（授权明确），或把 freesoundsite 那批换成 Pixabay / Freesound 上的 CC0 原档。**

## 音频处理

本目录的 wav 由原始下载文件转换而来，处理如下：

- 裁掉首尾静音（阈值 -60 dBFS），保留 5 ms 淡入淡出避免爆音
- 峰值归一到 -1 dBFS，提升量上限 +12 dB
- 统一转成 44100 Hz / 16-bit / 立体声

因此**各文件的响度已大致拉平**，但削峰类的短促爆裂音（crack）感知响度仍会低于长尾的撕裂音——进引擎后建议按类别过一遍音量，或放进 Sound Class / MetaSound 里统一控制。

## 原始文件与试听

- 原始下载文件（mp3/wav + 处理清单）：项目根目录 `SFX_Source/`，已加入 `.gitignore`
- 试听页：用浏览器打开 `SFX_Source/audition.html`，按类别分组、可逐条播放

## 已知排除的文件

- `thin-wooden-board-sharp-crack` 合集的 4 个文件实测接近静音（峰值 -33 ~ -52 dBFS），已丢弃。
