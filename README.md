# CS2 HvH

CS2 外部辅助 — 自瞄 + ESP + 墙体检测

## 使用方法

1. CS2 设为 **无边框窗口模式**
2. 双击 `cs2_hvh.exe` 打开
3. 按 **INSERT** 键打开菜单，按 **END** 紧急关闭
4. 绑定按键可一键开关 ESP

> 仅官匹（国服/外服都行）

## VisCheck 墙体检测

visible_check（自瞄设置里开启）开启后自瞄不会锁墙后敌人。

### 地图数据安装

从 [Releases](https://github.com/jieranyishenmorengui-creator/cs2he/releases) 下载 `vischeck_maps.zip`，解压后把 `data/` 文件夹放到 `cs2_hvh.exe` 同目录。

```
cs2_hvh.exe 所在位置
└── data/
    ├── de_mirage.opt
    ├── de_inferno.opt
    └── ...
```

### 使用

1. 按 INSERT → Misc 选项卡
2. VisCheck Map 下拉框选当前地图
3. 左上角变绿 `VisCheck: de_mirage` 即成功
4. 每次切图后手动选一下

> ⚠️ 没地图数据也能用，自动回退到游戏内置 spotted 机制
