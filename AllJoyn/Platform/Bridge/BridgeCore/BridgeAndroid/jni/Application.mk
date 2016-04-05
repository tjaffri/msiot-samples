APP_PLATFORM := android-19
APP_STL      := c++_shared
APP_CPPFLAGS += -std=c++14 -fexceptions
NDK_TOOLCHAIN_VERSION := clang
LOCAL_C_INCLUDES := ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libcxx/include
LOCAL_C_INCLUDES += ${ANDROID_NDK}/platforms/android-20/arch-arm/usr/include
APP_ABI := armeabi-v7a
