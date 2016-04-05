# Bridge

In this updated version of the bridge, the interface to the specific device is written in Node.js compatible javascript. The bridge makes use of
[https://github.com/jxcore/jxcore](https://github.com/jxcore/jxcore "JxCore") to run the javascript.

The bridge code is mostly cross-platform C++. Platform specific binaries are created by providing thin wrappers that depend on the cross-platform code.

### Project status

* This is a work in progress.
* At this time the bridge builds for x64 only.
* At this time there is no persisted state for the bridge or devices.
* At this time the bridge project includes pre-built binaries for the JxCore component. You can refer to the source here: https://github.com/jxcore/jxcore.
* At this time javascript that depends on a node.js non-native module will not run in the JxCore hosted environment due to issues with requiring relative paths.

### Building

#### Windows
To build this for Windows open the `Bridge.sln` using `Visual Studio 2015`. Select target architecture (`x64`, `x86` or `arm`) and build it.
Or you can build it from command line (requires [NuGet](http://docs.nuget.org/consume/command-line-reference) to be in `PATH`):

```
NuGet.exe restore Bridge.sln
"%VS140COMNTOOLS%vsvars32.bat"
msbuild Bridge.sln /p:Platform=x64 /t:Build
msbuild Bridge.sln /p:Platform=x86 /t:Build
msbuild Bridge.sln /p:Platform=arm /t:Build
```
#### Android
Android build supports `armeabi-v7a` only, support for other architectures is coming soon.

Prerequisites:
 1. Install [Android SDK](http://developer.android.com/sdk/installing/index.html) and [Android NDK](http://developer.android.com/ndk/downloads/index.html)
 2. Environment variable _ANDROID_NDK_HOME_ must be set to the point to the locations where the Android NDK is installed.

To build:
 1. Build _BridgeAndroid_ by running `"%ANDROID_NDK_HOME%\ndk-build"` from `Bridge/BridgeAndroid/jni`
 2. Build _ScriptAdapterLibAndroid_ by running `"%ANDROID_NDK_HOME%\ndk-build"` from `Bridge/ScriptAdapterLibAndroid/jni`

#### iOS
For iOS see the Bridge/BridgeiOS and Bridge/ScriptAdapterLibiOS folders to get started. The iOS builds are not functional at this time.


