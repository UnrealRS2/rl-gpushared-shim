# rl-gpushared-shim

**External Renderer Shim for RuneLite**

![0pmStEXGDZ](https://github.com/user-attachments/assets/f5880cc6-eb47-4150-a84c-96ab184da0af)

---

## Overview

`rl-gpushared-shim` is a lightweight shared memory bridge for RuneLite that allows high-throughput sync between Java and C++:

The goal is to allow RuneLite to act as a backend scene/data provider while an external engine handles rendering, without modifying RuneLite's/OSRS's core rendering pipeline.

---

## Features

RuneLite -> Shim:
- Camera
- FrameBuffer

Shim -> RuneLite
- Resolution
- Mouse Move / Press / Release

---

## Memory Layout

The shared memory region contains:

- **RLCameraState** — x, y, z, yaw, pitch, zoom
- **RLFrameBufferInfo** — width, height, ready, consumed
- **SResolution** — width, height, consumed
- **SMouseMove** — x, y, consumed
- **SMousePress** — button, consumed
- **SMouseRelease** — button, consumed
---

## Building

1. Set `set(JAVA_HOME "C:/Program Files/Java/jdk-17")` in CMakeLists to your JDK installation.

You can then build, and load the dll into the SharedMemoryBridge (it must be on PATH)
