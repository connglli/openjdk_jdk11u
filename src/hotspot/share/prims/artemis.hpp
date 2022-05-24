#ifndef SHARE_VM_PRIMS_ARTEMIS_HPP
#define SHARE_VM_PRIMS_ARTEMIS_HPP

#include "jni.h"
#include "runtime/thread.hpp"

class Artemis : public AllStatic {
private:
  static bool _used;
public:
  static void register_methods(JNIEnv* env, jclass artemis, JavaThread* thread);
  static void set_used() {
    _used = true;
  }
  static bool is_used() {
    return _used;
  }
};

#endif // SHARE_VM_PRIMS_ARTEMIS_HPP
