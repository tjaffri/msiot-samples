#Main Make file to all the libs#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := alljoyn_c
LOCAL_SRC_FILES := ../../../External/AllJoyn/c/lib/Android/liballjoyn_c.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Bridge
LOCAL_SRC_FILES := ../../BridgeAndroid/libs/armeabi-v7a/libBridge.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := LibJxCore
LOCAL_SRC_FILES := ../../../External/JxCore/lib/Android/$(TARGET_ARCH_ABI)/libjxcore.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := ScriptAdapter

NDK_HOME := $(NDK_ROOT)
EXTERNAL_PATH := $(LOCAL_PATH)/../../../External
CORE_BRIDGE_PATH := $(LOCAL_PATH)/../../BridgeCore
CORE_PRIVATE_PATH := $(LOCAL_PATH)/../../ScriptAdapterLib

LOCAL_CFLAGS += -std=c++14 -pthread -DANDROID -D__ANDROID__ -DQCC_OS_GROUP_POSIX -isystem $(EXTERNAL_PATH)/boost -isystem $(EXTERNAL_PATH)/AllJoyn/c/inc -isystem $(NDK_HOME)/platforms/android-19/arch-arm/usr/include
LOCAL_CPP_FEATURES += exceptions rtti
LOCAL_CPP_FLAGS += -include $(PCH_FILE)
LOCAL_LDLIBS += -llog

LOCAL_C_INCLUDES := $(NDK_HOME)/platforms/android-19/arch-arm/usr/include
LOCAL_C_INCLUDES += $(EXTERNAL_PATH)/boost
LOCAL_C_INCLUDES += $(EXTERNAL_PATH)/jxCore/inc
LOCAL_C_INCLUDES += $(EXTERNAL_PATH)/AllJoyn/c/inc
LOCAL_C_INCLUDES += $(CORE_BRIDGE_PATH)
LOCAL_C_INCLUDES += $(CORE_PRIVATE_PATH)

# The following wildcard calls require that LOCAL_PATH is used, which corresponds to the jni folder.
# However LOCAL_SRC_FILES is brilliant and auto-appends jni/ to everything. So these paths must be relative to jni,
# and then below we remove the duplicate jni/

LOCAL_SOURCES := $(wildcard $(LOCAL_PATH)/../../ScriptAdapterLib/*.cpp)
LOCAL_SRC_FILES := $(LOCAL_SOURCES:$(LOCAL_PATH)/%=%)

#LOCAL_SHARED_LIBRARIES := CAres ChromeZlib HttpParser jxCore MozJs OpenSsl SqLite UV alljoyn_c Bridge
LOCAL_SHARED_LIBRARIES := LibJxCore Bridge

include $(BUILD_SHARED_LIBRARY)
