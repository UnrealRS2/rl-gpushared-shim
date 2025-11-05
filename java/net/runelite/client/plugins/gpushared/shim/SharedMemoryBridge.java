package net.runelite.client.plugins.gpushared.shim;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicBoolean;

public class SharedMemoryBridge
{
    public static final int MAX_SCENE_SIZE = 8 * 1024 * 1024;  // 3D scene / 8 MB
    public static final int MAX_UI_SIZE = 3840 * 2160 * 4;     // RGBA framebuffer / 4K
    public static final int BUFFER_COUNT = 2;
    public static final int INPUT_QUEUE_SIZE = 256;

    // Double-buffered memory
    private final ByteBuffer[] sceneBuffers = new ByteBuffer[BUFFER_COUNT];
    private final ByteBuffer[] uiBuffers = new ByteBuffer[BUFFER_COUNT];
    private final FrameHeader[] frameHeaders = new FrameHeader[BUFFER_COUNT];
    private final CameraState camera = new CameraState();
    private final InputQueue inputQueue = new InputQueue();
    private final AtomicInteger activeIndex = new AtomicInteger(0);
    private long nativeHandle = 0;

    // Two resolutions
    public final Resolution runeliteResolution = new Resolution();
    public final Resolution externalResolution = new Resolution();

    static { System.loadLibrary("rl_gpushared_shim"); }

    public SharedMemoryBridge()
    {
        for (int i = 0; i < BUFFER_COUNT; i++)
        {
            sceneBuffers[i] = ByteBuffer.allocateDirect(MAX_SCENE_SIZE).order(ByteOrder.LITTLE_ENDIAN);
            uiBuffers[i] = ByteBuffer.allocateDirect(MAX_UI_SIZE).order(ByteOrder.LITTLE_ENDIAN);
            frameHeaders[i] = new FrameHeader();
        }
    }

    // JNI native functions
    public native long openSharedMemory(String name);
    public native void closeSharedMemory(long handle);

    public void init(String shmName) { nativeHandle = openSharedMemory(shmName); }
    public void shutdown() { if(nativeHandle != 0) { closeSharedMemory(nativeHandle); nativeHandle = 0; } }

    // Camera state
    public void setCamera(float x, float y, float z, float yaw, float pitch, float zoom)
    {
        camera.x = x; camera.y = y; camera.z = z;
        camera.yaw = yaw; camera.pitch = pitch; camera.zoom = zoom;
    }

    // Publish scene + UI frame
    public void publishFrame(byte[] sceneData, int sceneLength, byte[] uiData, int uiLength)
    {
        int nextIndex = (activeIndex.get() + 1) % BUFFER_COUNT;
        FrameHeader header = frameHeaders[nextIndex];
        ByteBuffer sceneBuf = sceneBuffers[nextIndex];
        ByteBuffer uiBuf = uiBuffers[nextIndex];

        while (header.ready.get() && !header.consumed.get()) { Thread.onSpinWait(); }

        sceneBuf.clear();
        sceneBuf.put(sceneData, 0, sceneLength);

        uiBuf.clear();
        uiBuf.put(uiData, 0, uiLength);

        header.frameId++;
        header.sceneDataLength = sceneLength;
        header.uiDataLength = uiLength;
        header.consumed.set(false);
        header.ready.set(true);

        activeIndex.set(nextIndex);
    }

    // Poll next input event from Unreal
    public InputEvent pollInput()
    {
        int tail = inputQueue.tail.get();
        int head = inputQueue.head.get();

        if (tail == head) return null;

        InputEvent event = inputQueue.events[tail % INPUT_QUEUE_SIZE];
        inputQueue.tail.set((tail + 1) % INPUT_QUEUE_SIZE);
        return event;
    }

    /**
     * Frame state / info
     */
    public static class FrameHeader
    {
        public final AtomicBoolean ready = new AtomicBoolean(false);
        public final AtomicBoolean consumed = new AtomicBoolean(true);
        public int frameId = 0;
        public int sceneDataLength = 0;
        public int uiDataLength = 0;
    }

    /**
     * runelite camera model
     */
    public static class CameraState { public float x, y, z, yaw, pitch, zoom; }

    /**
     * runelite input model
     */
    public static class InputEvent
    {
        public enum Type { KeyDown, KeyUp, MouseDown, MouseUp, MouseMove }
        public Type type;
        public int keyCode;
        public int mouseButton;
        public float mouseX, mouseY;
    }

    /**
     * In-order events from external renderer to be processed by runelite
     */
    public static class InputQueue
    {
        public final AtomicInteger head = new AtomicInteger(0); // written by Unreal
        public final AtomicInteger tail = new AtomicInteger(0); // read by Java
        public final InputEvent[] events = new InputEvent[INPUT_QUEUE_SIZE];

        public InputQueue()
        {
            for(int i=0;i<INPUT_QUEUE_SIZE;i++) events[i] = new InputEvent();
        }
    }

    /**
     * It helps for both sides of the bridge to be aware of each-others resolutions
     */
    public static class Resolution
    {
        public final AtomicInteger width = new AtomicInteger(0);
        public final AtomicInteger height = new AtomicInteger(0);
    }
}
