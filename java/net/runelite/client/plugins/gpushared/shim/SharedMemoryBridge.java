package net.runelite.client.plugins.gpushared.shim;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class SharedMemoryBridge
{
    static {
        System.loadLibrary("rl_gpushared_shim");
    }

    public long nativeHandle = 0;
    public ByteBuffer cameraBuf;
    public ByteBuffer frontBufferInfo;
    public ByteBuffer backBufferInfo;

    public native long openSharedMemory(String name);
    public native void closeSharedMemory(long handle);
    public native ByteBuffer mapCamera(long handle);
    public native ByteBuffer mapFrontBufferInfo(long handle);

    public native ByteBuffer mapBackBufferInfo(long handle);

    public void init(String shmName)
    {
        nativeHandle = openSharedMemory(shmName);
        if (nativeHandle == 0)
            throw new RuntimeException("Failed to open shared memory: " + shmName);

        cameraBuf      = mapCamera(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        frontBufferInfo = mapFrontBufferInfo(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        backBufferInfo = mapBackBufferInfo(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        setFrontBufferInfo(-1, -1, false, true);
        setBackBufferInfo(-1, -1, false, true);
    }

    public void shutdown()
    {
        if (nativeHandle != 0)
        {
            closeSharedMemory(nativeHandle);
            nativeHandle = 0;
        }
    }

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

    public void setFrontBufferInfo(int width, int height, boolean ready, boolean consumed) {
        frontBufferInfo.clear();
        frontBufferInfo.putInt(width);
        frontBufferInfo.putInt(height);
        frontBufferInfo.put((byte) (ready ? 1 : 0));
        frontBufferInfo.put((byte) (consumed ? 1 : 0));
        frontBufferInfo.rewind();
    }

    public void setBackBufferInfo(int width, int height, boolean ready, boolean consumed) {
        backBufferInfo.clear();
        backBufferInfo.putInt(width);
        backBufferInfo.putInt(height);
        backBufferInfo.put((byte) (ready ? 1 : 0));
        backBufferInfo.put((byte) (consumed ? 1 : 0));
        backBufferInfo.rewind();
    }
}
