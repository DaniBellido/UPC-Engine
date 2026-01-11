# STAGE3DX

The **STAGE3DX Engine** is a DirectX 12 learning project developed as part of the Advanced Programming for AAA Video Games Master's program at the Universitat Politècnica de Catalunya (UPC).
The project is based on the [EngineDX](https://github.com/ilgenio/EngineDX) and provides a modular platform for experimenting with rendering techniques, resource management, and game engine development in C++.


---
🔹 Purpose
---
STAGE3DX aims to provide an educational and extensible environment for:

Learning and practicing DirectX 12 from scratch.

Implementing a 3D viewer with textured quads, grids, and Unity-like camera controls.

Integrating Dear ImGui for debugging, profiling, and runtime control.

Serving as a foundation for future graphics projects or AAA game prototypes.

---
🔹 Main Features
---

DirectX 12 rendering.

Texture loading with mipmaps, filtering, and addressing options.

GLTF Model loading (meshes and materials).

Includes basic material, Phong, and PBR Phong. 

Translate, rotate, and scale models with ImGuizmo. 

3D grid and axes for reference.

Unity-style camera controls:

RMB + WASD for movement.

Alt + LMB for orbiting.

Mouse wheel button to pan.

Mouse wheel for zoom.

'F' to focus on an object.

SHIFT to sprint.

Dear ImGui window for:

FPS display.

Toggle grid and axes visibility.

Change texture filtering modes.

Change View Mode.

Real-time debug.

Maintains aspect ratio when resizing the ImGui window.

Extensibility for new modules.

---
🔹 Requirements
---
Windows 10/11

Visual Studio 2022 with C++17 support

DirectX 12 SDK (included in Windows 10+ SDK)

Additional libraries:

DirectXMath

Dear ImGui

---
🔹 Repository
---
https://github.com/DaniBellido/UPC-Engine
---
🔹 Usage
---

Run the project to see a selection of exercises and click on one of them.

Use the camera controls to explore the environment.

Open the ImGui window to toggle elements or change texture filters.

The engine is ready to add new entities, lights, and post-processing effects.


---
🔹 Credits
---

UPC – Advanced Programming for AAA Video Games: Educational material and academic support.

EngineDX
: Project base and initial structure.

Dear ImGui
: GUI library for debugging and profiling.

Microsoft
: DirectX12

---
🔹 License
---

STAGE3DX is distributed under the MIT License.
You are free to use, modify, and distribute the project as long as the original credits are maintained.

---
