# VisCheck 地图碰撞数据更新指南

## 概述

VisCheck 使用从 CS2 地图提取的碰撞网格做射线检测，判断敌人是否可见。
碰撞网格文件（`.opt`）位于 `data/` 目录，exe 启动时自动加载。

## 前置工具

1. **Source 2 Viewer CLI** — `C:\Users\00011152\Desktop\csqiang\Source2Viewer-CLI.exe`
   - 来源: https://github.com/ValveResourceFormat/ValveResourceFormat/releases
   - 版本: v19.2 (2026-07-02 下载)
   - 最新版直接下载: `https://github.com/ValveResourceFormat/ValveResourceFormat/releases/latest/download/cli-windows-x64.zip`

2. **Python 3** — `C:\Users\00011152\AppData\Local\Python\bin\python.exe`
   - 工具脚本: `tools/glb2opt.py`

## 更新流程（地图更新后重做全部）

### Step 1: 提取碰撞网格

```bash
# 对每张地图，用 Source2Viewer-CLI 导出 world_physics.vmdl_c 为 glb
"Source2Viewer-CLI.exe" -i "游戏目录\game\csgo\maps\de_mirage.vpk" ^
    -d --gltf_export_format "glb" -o "D:\cs2_opt\data" ^
    -f "maps/de_mirage/world_physics.vmdl_c"
```

输出: `D:\cs2_opt\data\maps\de_mirage\world_physics_physics.glb`

### Step 2: glb → opt 转换

```bash
python tools/glb2opt.py ^
    "D:\cs2_opt\data\maps\de_mirage\world_physics_physics.glb" ^
    "data/de_mirage.opt"
```

### Step 3: 复制到项目

```bash
cp "D:\cs2_opt\data\de_mirage.opt" "data/"
cp "D:\cs2_opt\data\de_mirage.opt" "cs2_hvh/bin/data/"
```

## 现有地图列表

| 地图名 | 文件名 | 三角形数 | 文件大小 |
|--------|--------|---------|---------|
| de_mirage | de_mirage.opt | ~133K | 4.6MB |
| de_dust2 | de_dust2.opt | ~400K | 15MB |
| de_nuke | de_nuke.opt | ~180K | 6.6MB |
| de_vertigo | de_vertigo.opt | ~200K | 7.2MB |
| de_anubis | de_anubis.opt | ~720K | 26MB |
| de_ancient | de_ancient.opt | ~900K | 33MB |
| de_cache | de_cache.opt | ~1.5M | 56MB |
| de_overpass | de_overpass.opt | ~1.3M | 48MB |
| de_train | de_train.opt | ~1.5M | 54MB |
| de_inferno | de_inferno.opt | ~2.5M | 94MB |
| aim_map2_go_pwa | aim_map2_go_pwa.opt | ~13K | 0.5MB |

## 批量转换脚本

```bash
for %%m in (de_mirage de_inferno de_anubis de_dust2 de_nuke de_overpass de_vertigo de_ancient de_cache de_train) do (
  "Source2Viewer-CLI.exe" -i "游戏目录\game\csgo\maps\%%m.vpk" -d --gltf_export_format "glb" -o "D:\cs2_opt\data" -f "maps/%%m/world_physics.vmdl_c"
  python tools/glb2opt.py "D:\cs2_opt\data\maps\%%m\world_physics_physics.glb" "data/%%m.opt"
)
```

## 注意

- `.opt` 文件仅包含三角形顶点数据，不含材质/纹理/伤害信息
- 地图更新后 `world_physics.vmdl_c` 文件路径不变，但碰撞网格可能变化
- `tools/glb2opt.py` 是独立的 Python 脚本，可脱离项目单独使用
- `.opt` 文件被 `.gitignore` 排除，不提交到 git
