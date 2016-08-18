
## Centennial app that hosts the AllJoyn Device System Bridge :
###  There are two parts in the centennial app
* BridgeHost : This project represents UWP portion fo the app that has hosts the app service, which in turns hosts the bridge (DSB).
* BridgeHostStartupTask : This project is a win32 application that runs on startup (logon), starts the app service and holds on to it so that underlying AllJoyn bridge (DSB) is alive. 

App service stores info (DeviceInfo) about previously onboarded devices in the application data, organized in the following application data container hierarchy. 
This data gets deleted upon uninstall. Ideally, corresponding cloud service should store this information which app can sync to.
That will ensure devices appear on the bus even after uninstall/install and not required to be onboarded again.
The name of the child containers to 'OnboardedDevices' container is generated using (app service)client provided 'Category' (in valueset) concatenated with device ID.
This should change per final design.

## App data Containers:  Used for storing device infomation
App data containers are used to store onboarding information, both transient and persistent.
An OnboardingID is concatenation of category and device id. : 'WinkThermoStat:4876334'
Categories are controlled by the client app.

### Hierarchy
'OnboradingId : DeviceInfo' represent key-value pair.
Container named 'OnboardedDevices' starts clean, on App services start up and stores the deivices that are successfully onboarded.
This is used to ensure that a device is surface to AllJoyn bus, only once.

* OnboardedDevices
*       OnboardingID : DeviceInfo
        OnboardingID : DeviceInfo


There will be other set of containers, one for each device category, that are persistent. 
This data is used to surface/onboard devices to AllJoyn bus, on app service startup.
Below is the hierarchy of containers. 
* WinkThermoStat
*       OnboardingID : DeviceInfo
        OnboardingID : DeviceInfo
* VeeraLightBulb
*       OnboardingID : DeviceInfo
* NestThermostat
*       OnboardingID : DeviceInfo

The Ultimate effect is, user sees the devices (on AJ bus), even after restarting the machine. This is the magic we want.
Console or client app can come and onboard new devices and go. But Bridge/App services stays alive.
Startup task has the health check (does every 5 seconds) and starts app service if not UP.

## Debugging app service in isolation, in visual studio.
	1. First build BridgeHost UWP app usual
	2. goto project debug properties & enable 'Do not launch, but debug my code when it starts'. This helps to break into VS, when any client makes an app service connection.

## Steps to build & deploy
	1. Ensure that the certs in the packaging folder are good.
	2. Ensure windows SDK is installed. Post build steps specially look for makeappx.exe and signtool.exe in "C:\Program Files (x86)\Windows Kits\10\bin\<arch>"
	3. Just build the solution/project. It generates OpenT2TCent.appx in the release folder.

	** NOTE: We can create appx bundle that includes the OpenT2T Console/client app as well. 

### Testing the app On the target machine
	1. install appcert.cer to personal store.
	2. install opent2tca.cer to machine trusted root store
	3. install the appx (double click & install)

### How actually the Centennial app is built, if done manually.
	1. First build BridgeHost UWP app usual
	2. Manually tweak AppManifest.xml that is generated, to add Centennial specific information
	3. Generate certs required to be sign the appx (local dev testing)
		? Making test root cert (CA). Install in trusted root store.
		makecert -n "CN=OpenT2T.Org" -r -sv OpenT2TKey.pvk OpenT2TCA.cer
		? Making the cert signed by test CA created above. Install this in user store.
		makecert -sk SignedByOpenT2TCA -iv OpenT2TKey.pvk -n "CN=OpenT2T.Org" -ic OpenT2TCA.cer appcert.cer -sr currentuser -ss My
	4. Define AppxSources.txt (name do not matter) with following contents (sample only. Modify per needs)
		[Files]
		".\AppxManifest.xml" "AppxManifest.xml"
		"Assets\LockScreenLogo.scale-200.png" "Assets\LockScreenLogo.scale-200.png"
		"Assets\SplashScreen.scale-200.png" "Assets\SplashScreen.scale-200.png"
		"Assets\Square150x150Logo.scale-200.png" "Assets\Square150x150Logo.scale-200.png"
		"Assets\Square44x44Logo.scale-200.png" "Assets\Square44x44Logo.scale-200.png"
		"Assets\StoreLogo.png" "Assets\StoreLogo.png"
		"Assets\Wide310x150Logo.scale-200.png" "Assets\Wide310x150Logo.scale-200.png"
		"Assets\Square44x44Logo.targetsize-24_altform-unplated.png" "Assets\Square44x44Logo.targetsize-24_altform-unplated.png"
		".\centennialapp.exe" "centennialapp.exe"
		".\UWP2.exe" "uwp2.exe"
		
		a. Modify AppxSources and binaries placement to add only required packages (node.dll or jx.dll)
	5. Create Appx package
		makeappx.exe pack /o /v /f AppxSources.txt /p OpenT2TCent.appx
	6. Sign the appx
		signtool sign /a /v /fd SHA256 /f appcert.cer OpenT2TCent.appx


certificates: https://msdn.microsoft.com/en-us/library/windows/desktop/jj835832(v=vs.85).aspx
MakeAppx: https://msdn.microsoft.com/en-us/library/windows/desktop/hh446767(v=vs.85).aspx
	(MakeAppx & signtool are available in razzle environment as well.!)

## Future/Pending
* Add additional app service commands for removing onboarded device, enumerating, partial update (device id) etc...
* Add additional properties to DeviceProps class (used for serialize the device props, when calling into Bridge) to match DeviceInfo in BridgeRT

### Main files that have the implementation.
Files that you would care are below. Rest are details.

Startup Task: AllJoyn/Platform/BridgeHostStartupTask/Program.cs
App Service: AllJoyn/Platform/BridgeHost/App.xaml.cs

Test app:
AllJoyn/Platform/TestApps/UwpTestApp/MainPage.xaml.cs