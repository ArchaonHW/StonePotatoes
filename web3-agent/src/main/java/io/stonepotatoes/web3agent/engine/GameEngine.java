package io.stonepotatoes.web3agent.engine;

/**
 * JNI bridge to the StonePotatoes C++ Game Engine.
 * Provides access to GPU acceleration and game engine functionality.
 */
public class GameEngine {
    static {
        try {
            System.loadLibrary("stonepotatoes_engine");
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Failed to load stonepotatoes_engine native library: " + e.getMessage());
        }
    }

    /**
     * Check if the game engine is available.
     */
    public static native boolean isAvailable();

    /**
     * Initialize the game engine with specified device index.
     */
    public static native boolean initialize(int deviceIndex);

    /**
     * Shutdown the game engine.
     */
    public static native void shutdown();

    /**
     * Get device capabilities.
     */
    public static native DeviceCaps getDeviceCaps();

    /**
     * Create a GPU buffer.
     */
    public static native long createBuffer(long bytes);

    /**
     * Destroy a GPU buffer.
     */
    public static native void destroyBuffer(long bufferHandle);

    /**
     * Write data to GPU buffer.
     */
    public static native boolean writeBuffer(long bufferHandle, byte[] data, long offset);

    /**
     * Read data from GPU buffer.
     */
    public static native byte[] readBuffer(long bufferHandle, long bytes, long offset);

    /**
     * Create a compute kernel from SPIR-V data.
     */
    public static native long createKernel(byte[] spirv, int bufferCount, int pushConstantBytes);

    /**
     * Dispatch a compute kernel.
     */
    public static native DispatchResult dispatch(long kernelHandle, long[] bufferHandles,
                                                 int groupsX, int groupsY, int groupsZ,
                                                 byte[] pushConstants);

    /**
     * Destroy a compute kernel.
     */
    public static native void destroyKernel(long kernelHandle);

    // Helper classes
    public static class DeviceCaps {
        public String name;
        public String vendor;
        public long totalMemory;
        public int computeUnits;
        public boolean supportsTimestampQueries;

        public DeviceCaps() {}

        public DeviceCaps(String name, String vendor, long totalMemory, int computeUnits, boolean supportsTimestampQueries) {
            this.name = name;
            this.vendor = vendor;
            this.totalMemory = totalMemory;
            this.computeUnits = computeUnits;
            this.supportsTimestampQueries = supportsTimestampQueries;
        }

        @Override
        public String toString() {
            return String.format("DeviceCaps{name='%s', vendor='%s', memory=%dMB, units=%d, timestamp=%b}",
                    name, vendor, totalMemory / (1024 * 1024), computeUnits, supportsTimestampQueries);
        }
    }

    public static class DispatchResult {
        public boolean ok;
        public double gpuMs;
        public double wallMs;

        public DispatchResult() {}

        public DispatchResult(boolean ok, double gpuMs, double wallMs) {
            this.ok = ok;
            this.gpuMs = gpuMs;
            this.wallMs = wallMs;
        }

        @Override
        public String toString() {
            return String.format("DispatchResult{ok=%b, gpu=%.2fms, wall=%.2fms}", ok, gpuMs, wallMs);
        }
    }
}
