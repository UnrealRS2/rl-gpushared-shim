# rl-gpushared-shim

**External Renderer Shim for RuneLite**

<img width="2879" height="1619" alt="image" src="https://github.com/user-attachments/assets/645b6f71-5956-498b-999d-28fd0257e1b6" />

---

## Overview

`rl-gpushared-shim` is a lightweight shared memory bridge for RuneLite that allows High-performance shared game state between Java and C++:

The goal is to allow RuneLite to act as a backend scene/data provider while an external engine handles rendering, without modifying RuneLite's/OSRS's core rendering pipeline.
  
I should note that I will not release the source code for the Unreal frontend.

---

## Features

- **Shared memory-based communication** for low-latency frame transfer
- **Double-buffered frame data** for smooth rendering

---

## Memory Layout

The shared memory region contains:

- **CameraState** — x, y, z, yaw, pitch, zoom
- **FrameBufferInfo** — double-buffered:
    - `FrontBufferInfo` — width, height, ready, consumed
    - `BackBufferInfo` — width, height, ready, consumed
---

## Building

1. Set `set(JAVA_HOME "C:/Program Files/Java/jdk-17")` in CMakeLists to your JDK installation.

You can then build, and load the dll into the SharedMemoryBridge (it must be on PATH)
