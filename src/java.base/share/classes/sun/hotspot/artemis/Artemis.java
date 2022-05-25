package sun.hotspot.artemis;

import java.lang.reflect.Method;

public class Artemis {

    /**
     * Return whether the method being invoked (caller of isBeingInterpreted()) is being interpreted.
     */
    public static native boolean isBeingInterpreted();

    /**
     * Return whether the method being invoked (caller of isJitCompiled()) is JIT compiled.
     * Note, a method is JIT compiled does not mean the method is running in the JIT/OSR code.
     */
    public static native boolean isJitCompiled();

    /**
     * Return whether the given method is JIT compiled.
     */
    public static native boolean isMethodJitCompiled(Method method);

    /**
     * Return whether the given method is JIT compiled.
     */
    public static boolean isMethodJitCompiled(Class<?> clazz,
                                              String methodName,
                                              Class<?>... args) throws NoSuchMethodException, SecurityException {
        return isMethodJitCompiled(clazz.getDeclaredMethod(methodName, args));
    }

    /**
     * Force to JIT compile a given method. Return whether the method is successfully JIT compiled.
     */
    public static native boolean ensureMethodJitCompiled(Method method);

    /**
     * Force to JIT compile a given method. Return whether the method is successfully JIT compiled.
     */
    public static boolean ensureMethodJitCompiled(Class<?> clazz,
                                                  String methodName,
                                                  Class<?>... args) throws NoSuchMethodException, SecurityException {
        return ensureMethodJitCompiled(clazz.getDeclaredMethod(methodName, args));
    }

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
