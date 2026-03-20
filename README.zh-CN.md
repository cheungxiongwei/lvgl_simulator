# 🧪 LVGL 模拟器

[English](README.md)

> 一个多后端模拟器套件，适用于 **LVGL (Light and Versatile Graphics Library)**
> 支持 **Qt Widgets**、**Qt Quick (QML)** 和 **SDL3** — 三条渲染路径，一个 UI 核心。

---

## ✨ 特性

* 🧩 **三种独立模拟器**

  * 🖼️ Qt Widgets（传统桌面 UI）
  * 🎬 Qt Quick / QML（GPU 加速、声明式 UI）
  * 🎮 SDL3（轻量、跨平台渲染）

* 🔌 **统一 LVGL 核心**

  * 所有模拟器共享同一 LVGL 子模块
  * 方便对比不同渲染后端效果

* ⚙️ **模块化结构**

  * 平台层清晰分离
  * 易于扩展或替换渲染后端

---

## 📁 项目结构

```text
lvgl_simulator/
├── LVGLWidget/           # Qt Widgets 模拟器
├── LVGLQML/              # Qt Quick (QML) 模拟器
├── SDL3/                 # SDL3 模拟器
├── 3rdparty/
│   ├── lvgl/             # LVGL 核心 (子模块)
│   ├── SDL/              # SDL3 (子模块)
│   └── freetype/         # 字体渲染 (子模块)
└── README_cn.md
```

---

## 🔗 Git 子模块

本项目依赖以下子模块：

```text
[submodule "3rdparty/lvgl"]
    path = 3rdparty/lvgl
    url = https://github.com/lvgl/lvgl.git

[submodule "3rdparty/SDL"]
    path = 3rdparty/SDL
    url = https://github.com/libsdl-org/SDL.git

[submodule "3rdparty/freetype"]
    path = 3rdparty/freetype
    url = https://github.com/freetype/freetype.git
```

---

## 🚀 快速开始

### 1️⃣ 克隆仓库（含子模块）

```bash
git clone --recursive https://github.com/yourname/lvgl_simulator.git
```

如果已克隆：

```bash
git submodule update --init --recursive
```

---

## 🧱 编译说明

### 🖼️ Qt Widgets 模拟器

```bash
cd LVGLWidget
mkdir build && cd build
cmake ..
cmake --build .
```

---

### 🎬 Qt Quick (QML) 模拟器

```bash
cd LVGLQML
mkdir build && cd build
cmake ..
cmake --build .
```

---

### 🎮 SDL3 模拟器

```bash
cd SDL3
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 🧪 模拟器概览

| 后端             | 渲染类型     | 使用场景        |
| -------------- | -------- | ----------- |
| Qt Widgets     | CPU / 光栅 | 传统桌面 UI 集成  |
| Qt Quick (QML) | GPU 加速   | 现代 UI / 动画  |
| SDL3           | 轻量 / 原生  | 跨平台 & 嵌入式模拟 |

---

## ⚙️ 依赖

* 🧠 LVGL（UI 核心）
* 🖥️ Qt（Widgets / Quick）
* 🎮 SDL3
* 🔤 FreeType（字体渲染）
* 🛠️ CMake ≥ 3.x

---

## 🎯 设计理念

* 🧱 **单一职责** — 每个模拟器独立
* 🔄 **代码复用** — 通过子模块共享 LVGL 核心
* 🔌 **低耦合** — 后端特定实现独立

---

## 🧭 何时选择哪个模拟器？

* **Qt Widgets**：需要传统桌面嵌入
* **Qt Quick**：想要流畅动画和现代 UI
* **SDL3**：偏好最小依赖和高移植性

---

## 🛠️ 自定义

* 替换各后端的显示驱动
* 集成自定义输入设备
* 通过 `lv_conf.h` 配置 LVGL

---

## 📌 注意事项

* 编译前确保已初始化子模块
* Qt 版本应匹配系统（Qt5 或 Qt6）
* 需要 SDL3（不支持 SDL2）

---

## 📄 许可证

本项目遵循其依赖库的许可协议：

* LVGL
* SDL3
* FreeType

---

## 🧩 未来规划

* 📱 改进触控模拟
* 🧪 跨后端性能基准测试
* 🌐 WebAssembly 后端（实验性）

---

## 🤝 贡献

欢迎提交 PR、提出 Issue 或进行实验。
带上你的渲染器，让 LVGL 在不同平台上展示它的风采。

---

## 🧵 结束语

三种模拟器，一个 UI 引擎 —
就像同一剧本在三种舞台上演：剧院、影院和掌机。

每种舞台讲述同一个故事，只是透过不同的镜头。
