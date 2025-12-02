package net.runelite.client.plugins.gpushared.shim;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

public class SharedMemoryBridge
{
    static {
        System.loadLibrary("rl_gpushared_shim");
    }

    public long nativeHandle = 0;
    public ByteBuffer cameraBuf;
    public ByteBuffer frontBufferInfo;
    public ByteBuffer resolutionBuffer;
    public ByteBuffer mouseMoveBuffer;
    public ByteBuffer mousePressBuffer;
    public ByteBuffer mouseReleaseBuffer;

    public native long openSharedMemory(String name);
    public native void closeSharedMemory(long handle);
    public native ByteBuffer mapCamera(long handle);
    public native ByteBuffer mapFrameBuffer(long handle);
    public native ByteBuffer mapResolution(long handle);
    public native ByteBuffer mapMouseMove(long handle);
    public native ByteBuffer mapMousePress(long handle);
    public native ByteBuffer mapMouseRelease(long handle);

    public void init(String shmName)
    {
        nativeHandle = openSharedMemory(shmName);
        if (nativeHandle == 0)
            throw new RuntimeException("Failed to open shared memory: " + shmName);

        cameraBuf      = mapCamera(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        frontBufferInfo = mapFrameBuffer(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        resolutionBuffer = mapResolution(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        mouseMoveBuffer = mapMouseMove(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        mousePressBuffer = mapMousePress(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        mouseReleaseBuffer = mapMouseRelease(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        setFrameBuffer(-1, -1, false, true, null);
        setResolution(-1, -1, true);
        setMouseMove(-1, -1, true);
        setMousePress(-1, true);
        setMouseRelease(-1, true);
    }

    public void shutdown()
    {
        if (nativeHandle != 0)
        {
            closeSharedMemory(nativeHandle);
            nativeHandle = 0;
        }
    }

    // RuneLite -> SHIM

    public void setCamera(int x, int y, int z, int yaw, int pitch, int zoom)
    {
        if (cameraBuf == null) return;
        cameraBuf.clear();
        cameraBuf.putInt(x);
        cameraBuf.putInt(y);
        cameraBuf.putInt(z);
        cameraBuf.putInt(yaw);
        cameraBuf.putInt(pitch);
        cameraBuf.putInt(zoom);
        cameraBuf.rewind();
    }

    public void setFrameBuffer(int width, int height, boolean ready, boolean consumed, int[] pixels) {
        frontBufferInfo.clear();
        frontBufferInfo.putInt(width);
        frontBufferInfo.putInt(height);
        frontBufferInfo.put((byte) (ready ? 1 : 0));
        frontBufferInfo.put((byte) (consumed ? 1 : 0));
        if (pixels != null) {
            // 2-byte padding so next int[] is 4-byte aligned
            frontBufferInfo.put((byte) 0);
            frontBufferInfo.put((byte) 0);
            IntBuffer intView = frontBufferInfo.asIntBuffer();
            intView.put(pixels);
        }
        frontBufferInfo.rewind();
    }

    // SHIM -> RuneLite

    public static Resolution resolution;

    public static class Resolution {
        public int width;
        public int height;
        public boolean consumed;

        public Resolution(int width, int height, boolean consumed) {
            this.width = width;
            this.height = height;
            this.consumed = consumed;
        }
    }

    public void setResolution(int width, int height, boolean consumed) {
        resolutionBuffer.clear();
        resolutionBuffer.putInt(width);
        resolutionBuffer.putInt(height);
        resolutionBuffer.put((byte) (consumed ? 1 : 0));
        resolutionBuffer.rewind();
    }

    public void getResolution() {
        resolution = new Resolution(
                resolutionBuffer.getInt(),
                resolutionBuffer.getInt(),
                resolutionBuffer.get() == 1);
        resolutionBuffer.rewind();
    }

    public static MouseMove mouseMove;

    public static class MouseMove {
        public int x;
        public int y;
        public boolean consumed;

        public MouseMove(int x, int y, boolean consumed) {
            this.x = x;
            this.y = y;
            this.consumed = consumed;
        }
    }

    public void setMouseMove(int x, int y, boolean consumed) {
        mouseMoveBuffer.clear();
        mouseMoveBuffer.putInt(x);
        mouseMoveBuffer.putInt(y);
        mouseMoveBuffer.put((byte) (consumed ? 1 : 0));
        mouseMoveBuffer.rewind();
    }

    public void getMouseMove() {
        mouseMove = new MouseMove(
                mouseMoveBuffer.getInt(),
                mouseMoveBuffer.getInt(),
                mouseMoveBuffer.get() == 1);
        mouseMoveBuffer.rewind();
    }

    public static MousePress mousePress;

    public static class MousePress {
        public int button;
        public boolean consumed;

        public MousePress(int button, boolean consumed) {
            this.button = button;
            this.consumed = consumed;
        }
    }

    public void setMousePress(int button, boolean consumed) {
        mousePressBuffer.clear();
        mousePressBuffer.putInt(button);
        mousePressBuffer.put((byte) (consumed ? 1 : 0));
        mousePressBuffer.rewind();
    }

    public void getMousePress() {
        mousePress = new MousePress(
                mousePressBuffer.getInt(),
                mousePressBuffer.get() == 1);
        mousePressBuffer.rewind();
    }

    public static MouseRelease mouseRelease;

    public static class MouseRelease {
        public int button;
        public boolean consumed;

        public MouseRelease(int button, boolean consumed) {
            this.button = button;
            this.consumed = consumed;
        }
    }

    public void setMouseRelease(int button, boolean consumed) {
        mouseReleaseBuffer.clear();
        mouseReleaseBuffer.putInt(button);
        mouseReleaseBuffer.put((byte) (consumed ? 1 : 0));
        mouseReleaseBuffer.rewind();
    }

    public void getMouseRelease() {
        mouseRelease = new MouseRelease(
                mouseReleaseBuffer.getInt(),
                mouseReleaseBuffer.get() == 1);
        mouseReleaseBuffer.rewind();
    }
}
