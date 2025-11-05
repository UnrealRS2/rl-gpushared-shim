# rl-gpushared-shim

**External Renderer Shim for RuneLite**

---

## Overview

`rl-gpushared-shim` is a lightweight shared memory bridge that allows RuneLite to output scene and UI frame data to an external renderer, such as Unreal Engine.  
It enables:

- High-performance rendering off-process
- Double-buffered scene and UI frames
- Shared camera state and resolution awareness
- Input event forwarding from the external renderer back to RuneLite

The goal is to allow RuneLite to act as a backend scene/data provider while an external engine handles rendering, without modifying RuneLite's/OSRS's core rendering pipeline.

---

## Features

- **Shared memory-based communication** for low-latency frame transfer
- **Double-buffered frame data** for smooth rendering
- **Independent resolution tracking** for RuneLite and external renderer
- **Atomic frame ready/consumed flags** for safe producer/consumer coordination
- **Input queue** to forward key/mouse events from the external renderer to RuneLite

---

## Memory Layout

The shared memory region contains:

- **CameraState** — x, y, z, yaw, pitch, zoom
- **Frames** — double-buffered:
    - `FrameHeader` — ready/consumed flags, lengths, frame ID
    - `sceneData` — raw scene geometry
    - `uiData` — rendered UI frame
- **InputQueue** — atomic ring buffer for input events
- **Resolution** — independent `runeliteResolution` and `externalResolution`

---

## Building

1. Set `set(JAVA_HOME "C:/Program Files/Java/jdk-17")` in CMakeLists to your JDK installation.
2. Generate JNI headers:

```bash
javac -h native -d out java/net/runelite/client/plugins/gpushared/shim/SharedMemoryBridge.java
```

You should then be able to build the project.