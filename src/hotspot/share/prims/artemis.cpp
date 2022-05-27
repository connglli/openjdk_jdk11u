#include "code/codeCache.hpp"
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
#include "utilities/ostream.hpp"

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
  return vf->is_interpreted_frame();
}

jmethodID reflected_method_to_jmethod_id(JavaThread* thread,
                                         JNIEnv* env,
                                         jobject method) {
  assert(method != NULL, "method should not be null");
  ThreadToNativeFromVM ttn(thread);
  return env->FromReflectedMethod(method);
}

bool is_method_compiled(JavaThread* thread,
                        jmethodID jmid,
                        int compLevel,
                        bool is_osr = false) {
  // The following is coped from whitebox.cpp
  MutexLockerEx mu(Compile_lock);
  methodHandle mh(thread, Method::checked_resolve_jmethod_id(jmid));
  // InstanceKlass maintains a OSR list, each is a nmethod with its BCI
  // and CompLevel.
  // Use InvocationEntryBci to check all nmethods of a single method.
  // - if match_level=true, only return nmethod with the specific CompLevel
  // - otherwise, return the nmethod that have the highest and upper than
  //   the given CompLevel (in here, its CompLevel_none).
  // See InstanceKlass::lookup_osr_nmethod().
  CompiledMethod *code = mh->lookup_osr_nmethod_for(InvocationEntryBci,
                                                    CompLevel_none,
                                                    /*match_level=*/ false);
  // Not an OSR nmethod, find a non-OSR nmethod
  if (!is_osr && code == NULL) {
    code = mh->code();
  }
  return code != NULL &&
         code->is_alive() &&
         !code->is_marked_for_deoptimization() &&
         code->comp_level() == compLevel;
}

// Native Artemis fields/methods //////////////////////////////////////////////

bool Artemis::_used = false;

AX_ENTRY(jboolean, AX_IsBeingInterpreted(JNIEnv* env, jclass artemis))
  int depth = 1;
  if (PrintArtemis) {
    Method* method = get_javaVframe_at(thread, depth)->method();
    tty->print_cr(">>"
                  "IsBeingInterpreted()"
                  ": method=\"%s\""
                  "<<",
                  method->external_name());
  }
  return is_being_interpreted_at(thread, depth);
AX_END

AX_ENTRY(jboolean, AX_IsMethodJitCompiled(JNIEnv* env,
                                          jclass artemis,
                                          jobject rmethod,
                                          jint compLevel))
  // TODO(congli): Support other CompLevels like C2.
  assert(compLevel == CompLevel_simple,
         "Only support C1 with SIMPLE level by far");
  jmethodID jmid = reflected_method_to_jmethod_id(thread, env, rmethod);
  if (PrintArtemis) {
    Method* method = Method::checked_resolve_jmethod_id(jmid);
    tty->print_cr(">>"
                  "IsMethodJitCompiled()"
                  ": method=\"%s\""
                  "<<",
                  method->external_name());
  }
  return is_method_compiled(thread, jmid, compLevel);
AX_END

AX_ENTRY(jboolean, AX_IsJitCompiled(JNIEnv* env,
                                    jclass artemis,
                                    jint caller,
                                    jint compLevel))
  // TODO(congli): Support other CompLevels like C2.
  assert(compLevel == CompLevel_simple,
         "Only support C1 with SIMPLE level by far");
  Method* method = get_javaVframe_at(thread, /*depth=*/ caller)->method();
  if (PrintArtemis) {
    tty->print_cr(">>"
                  "IsJitCompiled()"
                  ": method=\"%s\""
                  "<<",
                  method->external_name());
  }
  return is_method_compiled(thread, method->jmethod_id(), compLevel);
AX_END

AX_ENTRY(jboolean, AX_EnsureMethodJitCompiled(JNIEnv* env,
                                              jclass artemis,
                                              jobject rmethod,
                                              jint compLevel))
  // TODO(congli): Support other CompLevels like C2.
  assert(compLevel == CompLevel_simple,
         "Only support C1 with SIMPLE level by far");

  jmethodID jmid = reflected_method_to_jmethod_id(thread, env, rmethod);

  // No need to compile the method
  if (is_method_compiled(thread, jmid, compLevel)) {
    return true;
  }

  Method* method = Method::checked_resolve_jmethod_id(jmid);

  // Do not support compile a method being managed on stack
  if (is_being_managed_on_stack(thread, method)) {
    return false;
  }

  if (PrintArtemis) {
    tty->print_cr(">>"
                  "EnsureMethodJitCompiled()"
                  ": method=\"%s\""
                  ", bci=InvocationEntryBci"
                  ", osr=false"
                  "<<",
                  method->external_name());
  }

  return WhiteBox::compile_method(method, compLevel,
                                  InvocationEntryBci, thread);
AX_END

AX_ENTRY(jboolean, AX_EnsureJitCompiled(JNIEnv* env,
                                        jclass artemis,
                                        jint caller,
                                        jint compLevel))
  // TODO(congli): Support other CompLevels like C2.
  assert(compLevel == CompLevel_simple,
         "Only support C1 with SIMPLE level by far");

  javaVFrame* vf = get_javaVframe_at(thread, /*depth=*/ caller);

  // Already in compiler, don't compile
  // TODO(congli): Currently we don't care about whether the method is
  // marked to be deoptimized in future. May consider supporting this.
  if (vf->is_compiled_frame()) {
    return true;
  }

  Method* method = vf->method();

  // Already OSR compiled, no need any OSR compilation
  if (is_method_compiled(thread, method->jmethod_id(),
                         compLevel, /*is_osr=*/ true)) {
    return true;
  }

  if (PrintArtemis) {
    tty->print_cr(">>"
                  "EnsureJitCompiled()"
                  ": method=\"%s\""
                  ", bci=%d"
                  ", osr=true"
                  "<<",
                  method->external_name(),
                  vf->bci());
  }

  // BUG(congli): Not every bytecode index can OSR compile a method.
  // The following makes an assertion in ciTypeFlow::block_at() fail
  // where requires ciblk->start() == osr_bci. So seems only bci of
  // block start can be used in here. If the Java caller ensures this
  // function (Artemis.ensureJitCompiled()) is always called at the
  // beginning of a block. This the compilation works, but the OSR
  // (i.e., stack migration) is not yet done. So is still interpreting.
  // In HotSpot like x86,
  // - For Template Interpreter, the backedge counter is updated and
  //   OSR is to be executed in the TemplateTable::branch() method,
  //   i.e., templateTable_x86.cpp:branch()
  // - For Cpp Interpreter, this happens inside the bytecode interp.,
  //   i.e., bytecodeInterpreter.cpp:DO_BACKEDGE_CHECKS()
  return WhiteBox::compile_method(method, compLevel, vf->bci(), thread);
AX_END

AX_ENTRY(jboolean, AX_EnsureDeoptimized(JNIEnv* env, jclass artemis))
  javaVFrame* vf = get_javaVframe_at(thread, /*depth=*/ 1);

  // No need to deoptimize since we're already interpreted.
  if (vf->is_interpreted_frame()) {
    return true;
  }

  Method* method = vf->method();

  if (PrintArtemis) {
    tty->print_cr(">>"
                  "EnsureDeoptimized()"
                  ": method=\"%s\""
                  ", osr=true"
                  "<<",
                  method->external_name());
  }

  // TODO(congli): Only deoptimize current frame using Deoptimization::deoptimize_frame()
  // and then mark the method to be invalid like mark CodeCache?

  // Deoptimize the method instead of the sole frame such that all
  // following invocations go into the interpreter. Currently, we
  // deoptimize all nmethods activations (our caller and us) on the stack.
  int result = 0;

  {
    MutexLockerEx mu(Compile_lock);
    methodHandle mh(thread, method);

    // Invalidate all OSR nmethods
    if (mh->lookup_osr_nmethod_for(InvocationEntryBci,
                                   CompLevel_none, /*match_level=*/ false)) {
      result += mh->mark_osr_nmethods();
    }
    // Invalidate all non-OSR nmethods
    if (mh->code() != NULL) {
      mh->code()->mark_for_deoptimization();
      result += 1;
    }
    // Invalidate CodeCache
    result += CodeCache::mark_for_deoptimization(mh());

    if (result > 0) {
      VM_Deoptimize op;
      VMThread::execute(&op);
    }
  }

  return result > 0;
AX_END

// Register native methods to the Java-level Artemis class ////////////////////

#define CC (char*)
static JNINativeMethod methods[] = {
  {CC"isBeingInterpreted", CC"()Z", (void*)&AX_IsBeingInterpreted},
  {CC"isJitCompiled0", CC"(II)Z", (void*)&AX_IsJitCompiled},
  {CC"isMethodJitCompiled0", CC"(Ljava/lang/reflect/Method;I)Z", (void*)&AX_IsMethodJitCompiled},
  {CC"ensureJitCompiled0", CC"(II)Z", (void*)&AX_EnsureJitCompiled},
  {CC"ensureMethodJitCompiled0", CC"(Ljava/lang/reflect/Method;I)Z", (void*)&AX_EnsureMethodJitCompiled},
  {CC"ensureDeoptimized", CC"()Z", (void*)&AX_EnsureDeoptimized}
};
#undef CC

void Artemis::register_methods(JNIEnv* env, jclass artemis, JavaThread* thread) {
  WhiteBox::register_methods(env, artemis, thread,
                             methods, sizeof(methods) / sizeof(methods[0]));
}
