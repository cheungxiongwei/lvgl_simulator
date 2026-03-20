# 🧪 LVGL Simulator

[中文](README.zh-CN.md)

> A multi-backend simulator suite for **LVGL (Light and Versatile Graphics Library)**  
> Supporting **Qt Widgets**, **Qt Quick (QML)**, and **SDL3** — three rendering paths, one UI soul.

---

## ✨ Features

- 🧩 **Three Independent Simulators**
  - 🖼️ Qt Widgets (traditional desktop UI)
  - 🎬 Qt Quick / QML (GPU-accelerated, declarative UI)
  - 🎮 SDL3 (lightweight, cross-platform rendering)

- 🔌 **Unified LVGL Core**
  - All simulators share the same LVGL submodule
  - Easy to compare rendering backends

- ⚙️ **Modular Structure**
  - Clean separation of platform layers
  - Easy to extend or replace rendering backend

---

## 📁 Project Structure

```text
lvgl_simulator/
├── LVGLWidget/           # Qt Widgets simulator
├── LVGLQML/              # Qt Quick (QML) simulator
├── SDL3/                 # SDL3 simulator
├── 3rdparty/
│   ├── lvgl/             # LVGL core (submodule)
│   ├── SDL/              # SDL3 (submodule)
│   └── freetype/         # Font rendering (submodule)
└── README.md
````

---

## 🔗 Git Submodules

This project depends on the following submodules:

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

## 🚀 Getting Started

### 1️⃣ Clone with Submodules

```bash
git clone --recursive https://github.com/yourname/lvgl_simulator.git
```

Or if already cloned:

```bash
git submodule update --init --recursive
```

---

## 🧱 Build Instructions

### 🖼️ Qt Widgets Simulator

```bash
cd LVGLWidget
mkdir build && cd build
cmake ..
cmake --build .
```

---

### 🎬 Qt Quick (QML) Simulator

```bash
cd LVGLQML
mkdir build && cd build
cmake ..
cmake --build .
```

---

### 🎮 SDL3 Simulator

```bash
cd SDL3
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 🧪 Simulator Overview

| Backend        | Rendering Type       | Use Case                      |
| -------------- | -------------------- | ----------------------------- |
| Qt Widgets     | CPU / Raster         | Traditional UI integration    |
| Qt Quick (QML) | GPU Accelerated      | Modern UI / animations        |
| SDL3           | Lightweight / Native | Cross-platform & embedded sim |

---

## ⚙️ Dependencies

* 🧠 LVGL (UI core)
* 🖥️ Qt (Widgets / Quick)
* 🎮 SDL3
* 🔤 FreeType (font rendering)
* 🛠️ CMake ≥ 3.x

---

## 🎯 Design Philosophy

* 🧱 **Single Responsibility** — Each simulator is isolated
* 🔄 **Code Reuse** — Shared LVGL core via submodule
* 🔌 **Loose Coupling** — Backend-specific implementation only

---

## 🧭 When to Use Which?

* Choose **Qt Widgets** if you need classic desktop embedding
* Choose **Qt Quick** if you want fluid animations and modern UI
* Choose **SDL3** if you prefer minimal dependencies and portability

---

## 🛠️ Customization

* Replace display driver in each backend
* Integrate custom input devices
* Enable LVGL configs via `lv_conf.h`

---

## 📌 Notes

* Ensure submodules are initialized before building
* Qt version should match your system (Qt5 or Qt6)
* SDL3 is required (not SDL2)

---

## 📄 License

This project follows the licenses of its dependencies:

* LVGL
* SDL3
* FreeType

---

## 🧩 Future Ideas

* 📱 Touch simulation improvements
* 🧪 Performance benchmarking across backends
* 🌐 WebAssembly backend (experimental?)

---

## 🤝 Contributing

PRs, issues, and experiments are welcome.
Bring your renderer, and let LVGL wear it.

---

## 🧵 Closing Thought

Three simulators, one UI engine —
like the same script performed on three stages:
a theater, a cinema, and a handheld console.

Each tells the same story, just through a different lens.