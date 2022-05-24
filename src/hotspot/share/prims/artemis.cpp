#include <iostream>
#include "prims/artemis.hpp"
#include "prims/whitebox.hpp"
#include "prims/whitebox.inline.hpp"
#include "oops/instanceKlass.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "utilities/exceptions.hpp"

bool Artemis::_used = false;

// Native Artemis methods, some of then are directly forwarded to WhiteBox

#define AX_ENTRY WB_ENTRY
#define AX_END WB_END

AX_ENTRY(jboolean, AX_IsBeingInterpreted(JNIEnv* env, jclass artemis))
  std::cout << "IsBeingInterpreted()" << std::endl;
  return true;
AX_END

AX_ENTRY(jboolean, AX_EnsureJitCompiled(JNIEnv* env, jclass artemis))
  std::cout << "EnsureJitCompiled()" << std::endl;
  return true;
AX_END

AX_ENTRY(jboolean, AX_EnsureDeoptimized(JNIEnv* env, jclass artemis))
  std::cout << "EnsureDeoptimized()" << std::endl;
  return true;
AX_END

// Register native methods to the Java-level Artemis class

#define CC (char*)
static JNINativeMethod methods[] = {
  {CC"isBeingInterpreted", CC"()Z", (void*)&AX_IsBeingInterpreted},
  {CC"ensureJitCompiled", CC"()Z", (void*)&AX_EnsureJitCompiled},
  {CC"ensureDeoptimized", CC"()Z", (void*)&AX_EnsureDeoptimized}
};
#undef CC

void Artemis::register_methods(JNIEnv* env, jclass artemis, JavaThread* thread) {
  WhiteBox::register_methods(env, artemis, thread, methods, sizeof(methods) / sizeof(methods[0]));
}
