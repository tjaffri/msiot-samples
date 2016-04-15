#include "com_ot2t_bridge_Bridge.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <android/log.h>
#include "IBridge.h"
#include "AdapterDevice.h"
#include "IJsAdapter.h"

using namespace Bridge;
using namespace AdapterLib;

std::string ConvertJString(JNIEnv* env, jstring jStr)
{
	std::string stdStr;

	if (jStr)
	{
		const char *rawStr = env->GetStringUTFChars(jStr, nullptr);
		stdStr = rawStr;
		env->ReleaseStringUTFChars(jStr, rawStr);
	}

	return std::move(stdStr);
}

JNIEXPORT jint JNICALL Java_com_ot2t_bridge_Bridge_initializeBridge(JNIEnv* env, jobject obj)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_ot2t_Bridge_initializeBridge");
	std::shared_ptr<IJsAdapter> pScriptAdapter = IJsAdapter::Get();
	std::shared_ptr<IBridge> pBridge = IBridge::Get();
	pBridge->AddAdapter(pScriptAdapter);
	pBridge->Initialize();
	return 0;
}

JNIEXPORT void JNICALL Java_com_ot2t_bridge_Bridge_shutdownBridge(JNIEnv* env, jobject obj)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_ot2t_Bridge_shutdownBridge");
	std::shared_ptr<IJsAdapter> pScriptAdapter = IJsAdapter::Get();
	std::shared_ptr<IBridge> pBridge = IBridge::Get();
	pScriptAdapter->Shutdown();
	pBridge->Shutdown();
}

JNIEXPORT void JNICALL Java_com_ot2t_bridge_Bridge_addDevice(JNIEnv* env, jobject obj,
	jstring name, jstring vendor, jstring model, jstring version,
    jstring firmwareVersion, jstring serialNumber, jstring description, jstring props,
	jstring baseTypeXml, jstring script, jstring modulesPath)
{
	__android_log_print(ANDROID_LOG_INFO, "BridgeApi", "Java_com_ot2t_Bridge_addDevice");
	std::shared_ptr<IJsAdapter> pScriptAdapter = IJsAdapter::Get();
	std::shared_ptr<IAdapterDevice> pDevice = std::make_shared<AdapterDevice>();

    pDevice->Name(ConvertJString(env, name));
    pDevice->Vendor(ConvertJString(env, vendor));
    pDevice->Model(ConvertJString(env, model));
    pDevice->Version(ConvertJString(env, version));
    pDevice->FirmwareVersion(ConvertJString(env, firmwareVersion));
    pDevice->SerialNumber(ConvertJString(env, serialNumber));
    pDevice->Description(ConvertJString(env, description));
    pDevice->Props(ConvertJString(env, props));

    std::string baseTypeXmlStr = ConvertJString(env, baseTypeXml);
	std::string scriptStr = ConvertJString(env, script);
	std::string modulesPathStr = ConvertJString(env, modulesPath);

	pScriptAdapter->AddDevice(pDevice, baseTypeXmlStr, scriptStr, modulesPathStr);
}