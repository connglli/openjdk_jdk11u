package sun.hotspot.artemis;

import java.lang.reflect.Method;

public class Artemis {

    public enum CompLevel {
        // TODO(congli): Support other CompLevels
        SIMPLE(1);

        private int mValue;

        CompLevel(int value) {
            mValue = value;
        }

        public int getValue() {
            return mValue;
        }
    }

    /**
     * Return whether the method being invoked (caller of isBeingInterpreted()) is being interpreted.
     */
    public static native boolean isBeingInterpreted();

    /**
     * Return whether the method being invoked (caller of isJitCompiled()) is JIT compiled at SIMPLE level.
     * Note, a method is JIT compiled does not mean the method is running in the JIT/OSR code.
     */
    public static boolean isJitCompiled() {
        return isJitCompiled0(/*caller=*/ 2, CompLevel.SIMPLE.getValue());
    }

    /**
     * Return whether the method being invoked (caller of isJitCompiled()) is JIT compiled at given level.
     * Note, a method is JIT compiled does not mean the method is running in the JIT/OSR code.
     */
    public static boolean isJitCompiled(CompLevel level) {
        return isJitCompiled0(/*caller=*/ 2, level.getValue());
    }

    /**
     * Return whether the given method is JIT compiled at SIMPLE level.
     */
    public static boolean isMethodJitCompiled(Method method) {
        return isMethodJitCompiled0(method, CompLevel.SIMPLE.getValue());
    }

    /**
     * Return whether the given method is JIT compiled at given level.
     */
    public static boolean isMethodJitCompiled(CompLevel level, Method method) {
        return isMethodJitCompiled0(method, level.getValue());
    }

    /**
     * Return whether the given method is JIT compiled at SIMPLE level.
     */
    public static boolean isMethodJitCompiled(Class<?> clazz,
                                              String methodName,
                                              Class<?>... args) throws NoSuchMethodException, SecurityException {
        return isMethodJitCompiled0(clazz.getDeclaredMethod(methodName, args), CompLevel.SIMPLE.getValue());
    }

    /**
     * Return whether the given method is JIT compiled at given level.
     */
    public static boolean isMethodJitCompiled(CompLevel level,
                                              Class<?> clazz,
                                              String methodName,
                                              Class<?>... args) throws NoSuchMethodException, SecurityException {
        return isMethodJitCompiled0(clazz.getDeclaredMethod(methodName, args), level.getValue());
    }

    /**
     * Force to JIT compile a given method at SIMPLE level. Return whether the method is successfully JIT compiled.
     */
    public static boolean ensureMethodJitCompiled(Method method) {
        return ensureMethodJitCompiled0(method, CompLevel.SIMPLE.getValue());
    }

    /**
     * Force to JIT compile a given method at a given level. Return whether the method is successfully JIT compiled.
     */
    public static boolean ensureMethodJitCompiled(CompLevel level, Method method) {
        return ensureMethodJitCompiled0(method, level.getValue());
    }

    /**
     * Force to JIT compile a given method at SIMPLE level. Return whether the method is successfully JIT compiled.
     */
    public static boolean ensureMethodJitCompiled(Class<?> clazz,
                                                  String methodName,
                                                  Class<?>... args) throws NoSuchMethodException, SecurityException {
        return ensureMethodJitCompiled0(clazz.getDeclaredMethod(methodName, args), CompLevel.SIMPLE.getValue());
    }

    /**
     * Force to JIT compile a given method at a given level. Return whether the method is successfully JIT compiled.
     */
    public static boolean ensureMethodJitCompiled(CompLevel level,
                                                  Class<?> clazz,
                                                  String methodName,
                                                  Class<?>... args) throws NoSuchMethodException, SecurityException {
        return ensureMethodJitCompiled0(clazz.getDeclaredMethod(methodName, args), level.getValue());
    }

    /**
     * Force to JIT compile current invoking method at SIMPLE level. Return whether the method is successfully JIT compiled.
     * Since there is a delay before the method being JIT compiled is on-stack-replaced, it is recommended
     * to use ensureJitCompiled() together with isBeingInterpreted():
     *     if (!Artemis.ensureJitCompiled()) {
     *         throw new Exception("Failed to JIT");
     *     }
     *     while (Artemis.isBeingInterpreted()) {}
     *     // All following code are run in JITted code
     * Otherwise, some of the rest code are interpreted.
     */
    public static boolean ensureJitCompiled() {
        return ensureJitCompiled0(/*caller=*/ 2, CompLevel.SIMPLE.getValue());
    }

    /**
     * Force to JIT compile current invoking method at given level. Return whether the method is successfully JIT compiled.
     * Since there is a delay before the method being JIT compiled is on-stack-replaced, it is recommended
     * to use ensureJitCompiled() together with isBeingInterpreted():
     *     if (!Artemis.ensureJitCompiled(level)) {
     *         throw new Exception("Failed to JIT");
     *     }
     *     while (Artemis.isBeingInterpreted()) {}
     *     // All following code are run in JITted code
     * Otherwise, some of the rest code are interpreted.
     */
    public static boolean ensureJitCompiled(CompLevel level) {
        return ensureJitCompiled0(/*caller=*/ 2, level.getValue());
    }

    /**
     * Force to depotimize current invoking method. Return whether the method is successfully deoptimized.
     */
    public static native boolean ensureDeoptimized();

    static {
        registerNatives();
    }
    private static native void registerNatives();
    private static native boolean isJitCompiled0(int caller, int level);
    private static native boolean isMethodJitCompiled0(Method method, int level);
    private static native boolean ensureJitCompiled0(int caller, int level);
    private static native boolean ensureMethodJitCompiled0(Method method, int level);
}
