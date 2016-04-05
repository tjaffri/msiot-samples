#include "com_ot2t_bridge_Bridge.h"
#include <android/log.h>

JNIEXPORT jint JNICALL Java_com_opent2t_bridge_Bridge_initializeBridge(JNIEnv* env, jobject obj)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_opent2t_Bridge_initializeBridge");
	return 0;
}

JNIEXPORT void JNICALL Java_com_opent2t_bridge_Bridge_shutdownBridge(JNIEnv* env, jobject obj)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_opent2t_Bridge_shutdownBridge");
}

JNIEXPORT void JNICALL Java_com_opent2t_bridge_Bridge_addDevice(JNIEnv* env, jobject obj,
	jstring name, jstring vendor, jstring model, jstring version,
    jstring firmwareVersion, jstring serialNumber, jstring description, jstring props,
	jstring baseTypeXml, jstring script, jstring modulesPath)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_opent2t_Bridge_addDevice");
}