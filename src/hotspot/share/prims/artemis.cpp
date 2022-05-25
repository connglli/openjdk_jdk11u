#include <iostream>
#include "prims/artemis.hpp"
#include "prims/whitebox.hpp"
#include "prims/whitebox.inline.hpp"
#include "oops/instanceKlass.hpp"
#include "oops/method.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "runtime/vframe.hpp"
#include "runtime/vmOperations.hpp"
#include "runtime/vmThread.hpp"
#include "utilities/exceptions.hpp"

// Helper macros, methods /////////////////////////////////////////////////////

#define AX_ENTRY WB_ENTRY
#define AX_END WB_END

javaVFrame* get_javaVframe_at(JavaThread* thread, int depth,
                              bool must_exist = true) {
  if (!thread->has_last_Java_frame()) {
    return NULL;
  }
  RegisterMap reg_map(thread);
  javaVFrame* cur_vf = thread->last_java_vframe(&reg_map);
  for (int cur_dp = 0; cur_vf != NULL && cur_dp < depth;) {
    cur_vf = cur_vf->java_sender();
    cur_dp += 1;
  }
  if (must_exist) {
    assert(cur_vf != NULL && cur_vf->is_java_frame(),
          "Frame at depth %d is not a Java frame or is NULL.", depth);
  }
  return cur_vf;
}

bool is_being_managed_on_stack(JavaThread* thread, Method* goal) {
  if (!thread->has_last_Java_frame()) {
    return NULL;
  }
  RegisterMap reg_map(thread);
  javaVFrame* cur_vf = thread->last_java_vframe(&reg_map);
  while (cur_vf != NULL) {
    if (cur_vf->method() == goal) {
      return true;
    }
    cur_vf = cur_vf->java_sender();
  }
  return false;
}

bool is_being_interpreted_at(JavaThread* thread, int depth) {
  javaVFrame* vf = get_javaVframe_at(thread, depth);
  Method* method = vf->method();
  std::cout << "method=" << method->external_name()
            << ", depth=" << depth
            << ", interpreting=" << vf->is_interpreted_frame()
            << std::endl;
  return vf->is_interpreted_frame();
}

jmethodID reflected_method_to_jmethod_id(JavaThread* thread, JNIEnv* env, jobject method) {
  assert(method != NULL, "method should not be null");
  ThreadToNativeFromVM ttn(thread);
  return env->FromReflectedMethod(method);
}

bool is_method_compiled(JavaThread* thread, jmethodID jmid) {
  // The following is coped from whitebox.cpp
  MutexLockerEx mu(Compile_lock);
  methodHandle mh(thread, Method::checked_resolve_jmethod_id(jmid));
  // InstanceKlass maintains a OSR list each is a nmethod with its BCI and CompLevel.
  // Use InvocationEntryBci to check all nmethods of a signle method.
  // - if match_levle=true, only return nmethod with the specifc CompLevel
  // - otherwise, return the nmethod that have the highest and upper than
  //   the given CompLevel (in here, its CompLevel_none).
  // See InstanceKlass::lookup_osr_nmethod().
  CompiledMethod* code = mh->lookup_osr_nmethod_for(InvocationEntryBci,
      CompLevel_none, /*match_level=*/ false);
  // No OSR nmethod, just find an non-OSR method
  if (code == NULL) {
    code = mh->code();
  }
  return code != NULL && code->is_alive() && !code->is_marked_for_deoptimization();
}

// Native Artemis fields/methods //////////////////////////////////////////////

bool Artemis::_used = false;

AX_ENTRY(jboolean, AX_IsBeingInterpreted(JNIEnv* env, jclass artemis))
  return is_being_interpreted_at(thread, /*depth=*/ 1);
AX_END

AX_ENTRY(jboolean, AX_IsMethodJitCompiled(JNIEnv* env,
                                          jclass artemis,
                                          jobject rmethod))
  jmethodID jmid = reflected_method_to_jmethod_id(thread, env, rmethod);
  return is_method_compiled(thread, jmid);
AX_END

AX_ENTRY(jboolean, AX_IsJitCompiled(JNIEnv* env, jclass artemis))
  jmethodID jmid = get_javaVframe_at(thread, /*depth=*/ 1)->method()->jmethod_id();
  return is_method_compiled(thread, jmid);
AX_END

AX_ENTRY(jboolean, AX_EnsureMethodJitCompiled(JNIEnv* env,
                                              jclass artemis,
                                              jobject rmethod))
  jmethodID jmid = reflected_method_to_jmethod_id(thread, env, rmethod);

  // No need to compile the method
  if (is_method_compiled(thread, jmid)) {
    return false;
  }

  Method* method = Method::checked_resolve_jmethod_id(jmid);

  // Do not support compile a method being managed on stack
  if (is_being_managed_on_stack(thread, method)) {
    return false;
  }

  std::cout << "EnsureMethodJitCompiled()"
            << ": method=" << method->external_name()
            << ", bci=InvocationEntryBci"
            << std::endl;

  return WhiteBox::compile_method(method, CompLevel_full_optimization,
                                  InvocationEntryBci, thread);
AX_END

AX_ENTRY(jboolean, AX_EnsureJitCompiled(JNIEnv* env, jclass artemis))
  javaVFrame* vf = get_javaVframe_at(thread, /*depth=*/ 1);
  Method* method = vf->method();
  std::cout << "EnsureJitCompiled()"
            << ": method=" << method->external_name()
            << ", bci=" << vf->bci()
            << std::endl;
  // Bug(congli): Not every bytecode index can OSR compile a method.
  // The following makes an assertion in ciTypeFlow::block_at() fail
  // where requires ciblk->start() == osr_bci. So seems only bci of
  // block start can be used in here.
  return WhiteBox::compile_method(method, CompLevel_full_optimization,
                                  vf->bci(), thread);
AX_END

AX_ENTRY(jboolean, AX_EnsureDeoptimized(JNIEnv* env, jclass artemis))
  javaVFrame* vf = get_javaVframe_at(thread, /*depth=*/ 1);
  Method* method = vf->method();
  std::cout << "EnsureDeoptimized()"
            << ": method=" << method->external_name()
            << std::endl;
  Deoptimization::deoptimize_frame(thread, vf->frame_pointer()->id());
  return true;
AX_END

// Register native methods to the Java-level Artemis class ////////////////////

#define CC (char*)
static JNINativeMethod methods[] = {
  {CC"isBeingInterpreted", CC"()Z", (void*)&AX_IsBeingInterpreted},
  {CC"isJitCompiled", CC"()Z", (void*)&AX_IsJitCompiled},
  {CC"isMethodJitCompiled", CC"(Ljava/lang/reflect/Method;)Z", (void*)&AX_IsMethodJitCompiled},
  {CC"ensureJitCompiled", CC"()Z", (void*)&AX_EnsureJitCompiled},
  {CC"ensureMethodJitCompiled", CC"(Ljava/lang/reflect/Method;)Z", (void*)&AX_EnsureMethodJitCompiled},
  {CC"ensureDeoptimized", CC"()Z", (void*)&AX_EnsureDeoptimized}
};
#undef CC

void Artemis::register_methods(JNIEnv* env, jclass artemis, JavaThread* thread) {
  WhiteBox::register_methods(env, artemis, thread,
                             methods, sizeof(methods) / sizeof(methods[0]));
}
