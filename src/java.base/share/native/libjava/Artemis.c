#include "jni.h"
#include "jvm.h"

#include "sun_hotspot_artemis_Artemis.h"

JNIEXPORT void JNICALL
Java_sun_hotspot_artemis_Artemis_registerNatives(JNIEnv *env, jclass cls)
{
    JVM_RegisterArtemisMethods(env, cls);
}
