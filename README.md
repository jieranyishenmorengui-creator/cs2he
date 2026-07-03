# CS2 HvH

CS2 外部辅助 (External Cheat) — 自瞄 + ESP + 墙体检测

## 快速开始

1. 将 CS2 设置为 **无边框窗口模式**
2. 关闭游戏内 `设置 → 视频 → 增强型人物对比`（避免影响 ESP 显示）
3. 双击 `cs2_hvh.exe`（建议 **管理员权限** 运行）
4. 进入游戏主界面后，等待控制台显示 `[+] All offsets found`
5. 按 **INSERT** 键打开菜单，按 **END** 键紧急关闭

> ⚠️ 仅支持 **官匹**（国服/外服均可），仅保证 **ESP 功能**在休闲/竞技中可用。
> 其他模式或社区服务器可能不稳定。

## 功能

### 自瞄 (Aimbot)

| 功能 | 说明 |
|------|------|
| 自瞄开关 | 默认开启，鼠标侧键激活 |
| 目标部位 | 头/颈/脊椎/骨盆 |
| FOV 限制 | 像素级范围控制 |
| 平滑度 | 帧率无关的 EMA 平滑 |
| 提前量 | 速度外推 + 36单位硬上限防超 |
| RCS 压枪 | 可调比例的自动压枪 |
| 可见性检测 | **VisCheck 射线墙体检测**（见下文）|
| 目标锁定 | 锁定后不因新敌人出生切换 |
| 侧身头部补偿 | 敌人侧身时修正头骨位置 |

### 扳机 (Triggerbot)

| 功能 | 说明 |
|------|------|
| 模式 | m_iIDEntIndex / FOV角度检测 |
| 延迟 | 随机延迟区间模拟人手 |
| 速度限制 | 移动过快时不触发 |
| 团队检测 | 跳过队友 |

### ESP

| 功能 | 说明 |
|------|------|
| 方框 | 矩形 / 角框 / 3D |
| 骨骼 | 30 根骨骼绘制 |
| 血量条 | 当前血量 |
| 玩家名 | 支持 CJK 拼音首字母 |
| 武器名 | 武器图标名称 |
| 距离 | 米制距离显示 |
| **可见指示** | 敌人可见时框变绿色 **(aimbot 射线结果，零延迟)** |
| 头部圆圈 | 头部位置指示 |
| 平滑因子 | 屏幕位置插值防抖动 |
| 最大距离 | 200 单位以内显示 |

### 其他

| 功能 | 说明 |
|------|------|
| 观众列表 | 显示正在观看你的玩家名字 |
| 自动连跳 | 按住空格自动 Bunny Hop |
| FPS 限制 | 可调最大帧率 |

## VisCheck 墙体检测

### 工作原理

VisCheck 通过解析 CS2 地图的 `world_physics.vmdl_c` 碰撞网格，构建 BVH 加速结构，
在自瞄时做 **射线-三角形求交** 来判断敌人是否可见。

- 无 `.opt` 文件时：**自动回退到 `m_bSpotted` 机制**（游戏内置 spotted 标记，无延迟但不够精准）
- 有 `.opt` 文件时：**实时射线检测**，敌人缩回墙后瞬间不瞄
- 可见敌人 ESP 框为**绿色**（aimbot 射线结果，零延迟）

### 使用方法

1. 下载地图数据（见下方安装教程）
2. 放入 `cs2_hvh/bin/data/` 目录
3. 进游戏后按 **INSERT** → **Misc** 选项卡
4. 在 **VisCheck Map** 下拉框中选择当前地图
5. 左上角状态变为绿色 `VisCheck: de_mirage` 即加载成功

> ⚠️ 每次切换地图后需手动在菜单中重新选择地图。
> 选择后自动保存到 config.json。

### 地图数据安装

#### 方式一：下载预编译数据

从 [Releases](https://github.com/jieranyishenmorengui-creator/cs2he/releases) 下载 `vischeck_maps.zip`，解压后将 `data/` 文件夹放到 `cs2_hvh.exe` 同目录。

最终结构：
```
cs2_hvh.exe
└── data/
    ├── de_mirage.opt
    ├── de_inferno.opt
    ├── de_dust2.opt
    ├── de_nuke.opt
    ├── de_anubis.opt
    ├── de_ancient.opt
    ├── de_overpass.opt
    ├── de_cache.opt
    ├── de_train.opt
    ├── de_vertigo.opt
    ├── aim_map2_go_pwa.opt
    ├── aim_redline_fp.opt
    ├── cs_italy.opt
    ├── cs_office.opt
    └── (更多…)
```

#### 方式二：自行提取（地图更新后用）

详见 [VISCHEAT_MAP_DATA.md](VISCHEAT_MAP_DATA.md)

需要工具：
- [Source 2 Viewer CLI](https://github.com/ValveResourceFormat/ValveResourceFormat/releases)
- Python 3
- `tools/glb2opt.py`

### 可用地图

| 地图 | 对应 .opt | 三角形 |
|------|----------|--------|
| 荒漠迷城 | de_mirage.opt | 133K |
| 炼狱小镇 | de_inferno.opt | 2.5M |
| 阿努比斯 | de_anubis.opt | 720K |
| 炙热沙城Ⅱ | de_dust2.opt | 400K |
| 核子危机 | de_nuke.opt | 180K |
| 死亡游乐园 | de_overpass.opt | 1.3M |
| 殒命大厦 | de_vertigo.opt | 200K |
| 远古遗迹 | de_ancient.opt | 900K |
| 死城之谜 | de_cache.opt | 1.5M |
| 列车停放站 | de_train.opt | 1.5M |
| 死斗地图 | aim_map2_go_pwa.opt | 13K |

## 控制台功能

(暂无控制台命令，所有功能通过菜单操作)

## 构建

### 环境要求

- CMake 3.20+
- C++20 编译器 (MinGW GCC 13+ / MSVC 2022)
- DirectX 11 SDK
- ImGui (已集成)

### 编译

```bash
cd cs2_hvh
cmake -B build -G "Ninja"
cmake --build build
```

输出: `cs2_hvh/bin/cs2_hvh.exe`

## 偏移更新

游戏更新后需更新偏移：

1. 运行 `cs2-dumper` 重新生成输出
2. 检查 `cs2_hvh/src/core/offsets.cpp` 中的 fallback RVA
3. 检查 `cs2_hvh/src/utils/sdk.h` 中的 schema/netvar 偏移
4. 重新编译

## 致谢

- [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper) — 偏移生成
- [Read1dno/VisCheckCS2](https://github.com/Read1dno/VisCheckCS2) — VisCheck 参考实现
- [IMXNOOBX/cs2-external-esp](https://github.com/IMXNOOBX/cs2-external-esp) — 骨骼绘制参考
- [ValveResourceFormat](https://github.com/ValveResourceFormat/ValveResourceFormat) — CS2 资源提取
