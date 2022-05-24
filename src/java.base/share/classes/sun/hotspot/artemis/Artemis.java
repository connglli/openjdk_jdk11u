package sun.hotspot.artemis;

public class Artemis {

    /**
     * Return whether the method being invoked (caller of isBeingInterpreted()) is being interpreted.
     */
    public static native boolean isBeingInterpreted();

    /**
     * Force to JIT compile current invoking method. Return whether the method is successfully JIT compiled.
     * Since there is a delay before the method being JIT compiled is on-stack-replaced, it is recommended
     * to use ensureJitCompiled() together with isBeingInterpreted():
     *     if (!Artemis.ensureJitCompiled()) {
     *         throw new Exception("Failed to JIT");
     *     }
     *     while (Artemis.isBeingInterpreted()) {}
     *     // All following code are run in JITted code
     * Otherwise, some of the rest code are interpreted.
     */
    public static native boolean ensureJitCompiled();

    /**
     * Force to depotimize current invoking method. Return whether the method is successfully depotimized.
     */
    public static native boolean ensureDeoptimized();

    static {
        registerNatives();
    }
    private static native void registerNatives();
}
