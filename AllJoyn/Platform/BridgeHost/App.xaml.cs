﻿//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

using System;
using Windows.ApplicationModel.Activation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using Windows.ApplicationModel.Background;
using Windows.ApplicationModel.AppService;
using Windows.Foundation.Collections;
using System.Linq;
using BridgeRT;
using Windows.Storage;
using Windows.ApplicationModel;
using System.Text;
using Windows.ApplicationModel.DataTransfer;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Threading.Tasks;
using System.Diagnostics;

// The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=402347&clcid=0x409

namespace OpenT2T.Bridge
{
    [DataContract]
    internal class DeviceProps
    {
        public DeviceProps(string identifier)
        {
            id = identifier;
        }

        [DataMember]
        internal string id;
    }

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    sealed partial class App : Application
    {
        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Construct();
        }

        private AppServiceConnection appServiceConnection;
        private BackgroundTaskDeferral appServiceDeferral;
        public static bool isStartUpTaskRunning = false;

        /// <summary>
        ///  App data containers are used to store onboarding information, both transient and persistent.
        ///  An OnboardingID is a concatenation of category and device id. : 'WinkThermoStat:4876334'
        ///  Categories are controlled by the client app.
        /// Container named 'OnboardedDevices' starts clean, on App services start up and stores the deivices that are successfully onboarded.
        /// This is used to ensure that a device is surface to AllJoyn bus, only once.
        /// Hierarchy
        ///  OnboardedDevices
        ///      OnboardingID : DeviceInfo
        ///      OnboardingID : DeviceInfo
        ///      OnboardingID : DeviceInfo
        /// There will be other set of containers, one for each device category, that are persistent. 
        /// This data is used to surface/onboard devices to AllJoyn bus, on app service startup.
        /// Below is the hierarchy of containers. 
        ///      WinkThermoStat
        ///          OnboardingID : DeviceInfo
        ///          OnboardingID : DeviceInfo
        ///      VeeraLightBulb
        ///          OnboardingID : DeviceInfo
        ///      NestThermostat
        ///          OnboardingID : DeviceInfo
        //// </summary>
        private const string OnboardedDevicesContainerName = "OnboardedDevices"; // 

        /// <summary>
        /// Initializes the app service on the host process 
        /// </summary>
        protected async override void OnBackgroundActivated(BackgroundActivatedEventArgs args)
        {
            Debug.WriteLine("OnBackgroundActivated ");
            base.OnBackgroundActivated(args);

            IBackgroundTaskInstance taskInstance = args.TaskInstance;
            string taskName = taskInstance.Task.Name;
            Debug.WriteLine("TaskName = " + taskName);
            var triggerDetails = args.TaskInstance.TriggerDetails as AppServiceTriggerDetails;
            bool isStartupTask = triggerDetails.CallerPackageFamilyName.Equals(Windows.ApplicationModel.Package.Current.Id.FamilyName);

            if (isStartupTask ||
                IsSupportedAppServiceClient(triggerDetails.CallerPackageFamilyName))
            {
                appServiceDeferral = taskInstance.GetDeferral();
                taskInstance.Canceled += TaskInstance_Canceled;

                var appServiceDetails = triggerDetails as AppServiceTriggerDetails;
                Debug.WriteLine("New Connection from " + appServiceDetails.CallerPackageFamilyName);
                appServiceConnection = appServiceDetails.AppServiceConnection;
                appServiceConnection.RequestReceived += AppServiceConnection_RequestReceived;
                appServiceConnection.ServiceClosed += AppServiceConnection_ServiceClosed;
            }

            if (isStartupTask)
            {
                isStartUpTaskRunning = true;
            }
            else
            {
                await StartOpenT2TAppServiceAnchorProcess(); // Background process is not yet launched. Launch it so that the reference is kept alive.
            }

            // Onboard the devices that were onboarded previously.
            await OnboardRegisteredDevices();
        }

        private bool IsSupportedAppServiceClient(string callerPackageFamilyName)
        {
            return callerPackageFamilyName.Equals("org.opent2t.console_h35559jr9hy9m", StringComparison.OrdinalIgnoreCase) || // OpenT2T Console app
                callerPackageFamilyName.Equals("OpenT2TAllJoynBridgeUWPTestClient_a6049pvczh34w", StringComparison.OrdinalIgnoreCase); // Test app
        }

        private async void AppServiceConnection_RequestReceived(AppServiceConnection sender, AppServiceRequestReceivedEventArgs args)
        {
            AppServiceDeferral msgDeferral = args.GetDeferral();
            string response = "failed";

            var onboardingData = args.Request.Message;
            if ((onboardingData != null) && onboardingData.Keys.Contains("command"))
            {
                string command = onboardingData["command"] as string;
                Debug.WriteLine("Command Received: " + command);
                if (command.Equals("AddDevice"))
                {
                    try
                    {
                        // Extract ID from the props and insert into the value set.
                        DataContractJsonSerializer js = new DataContractJsonSerializer(typeof(DeviceProps));
                        MemoryStream ms = new MemoryStream(System.Text.UTF8Encoding.UTF8.GetBytes(onboardingData["props"] as string));
                        DeviceProps deviceProps = (DeviceProps)js.ReadObject(ms);

                        if((deviceProps.id == null) || (deviceProps.id == ""))
                        {
                            throw new Exception("Device props do not contain 'id', which is required to uniquely identify the device. This is used for storing and matching the device to avoid duplicate onboarding of the same device.");
                        }

                        onboardingData.Add("id", deviceProps.id);
                        await AddDeviceToAllJoynAsync(onboardingData);
                        response = "succeeded";
                    }
                    catch (Exception e)
                    {
                        response = e.Message;
                    }
                }
                else if ((command != null) && command.Equals("healthcheck"))
                {
                    response = "healthy";
                }
            }
            ValueSet valueSet = new ValueSet();
            valueSet.Add("response", response);
            await args.Request.SendResponseAsync(valueSet);
            msgDeferral.Complete();
        }

        private void TaskInstance_Canceled(IBackgroundTaskInstance sender, BackgroundTaskCancellationReason reason)
        {
            appServiceDeferral.Complete();
            System.Diagnostics.Debug.WriteLine("Task instance cancelled: {0}-{1}", sender.Task.Name, reason.ToString());
        }

        private  void AppServiceConnection_ServiceClosed(AppServiceConnection sender, AppServiceClosedEventArgs args)
        {
            appServiceDeferral.Complete();
            System.Diagnostics.Debug.WriteLine("service close ConnectionForBackGroundProcess_ServiceClosed");
        }

        async Task OnboardRegisteredDevices()
        {
            // Cache the onboarding data and add the device to the bridge 
            var appData = Windows.Storage.ApplicationData.Current.LocalSettings;

            // Clear the cache the onboarded devices
            var onboardedDevicesContainer = appData.CreateContainer(OnboardedDevicesContainerName, ApplicationDataCreateDisposition.Always);
            onboardedDevicesContainer.Values.Clear();

            foreach (var containerKey in appData.Containers.Keys)
            {
                var categoryContainer = appData.Containers[containerKey];
                if(categoryContainer.Name.Equals(OnboardedDevicesContainerName)){
                    continue; // Skip the temporary cache container.
                }

                foreach (var onboardingData in categoryContainer.Values.Values)
                {
                    try
                    {
                        var onboardingDataValueSet = AppDataCompositeValueToValueSet(onboardingData as ApplicationDataCompositeValue);
                        await AddDeviceToAllJoynAsync(onboardingDataValueSet);

                        // Store curently onboarded devices to AllJoyn bridge
                        onboardedDevicesContainer.Values.Add(GetOnboardingId(onboardingDataValueSet), onboardingData as ApplicationDataCompositeValue);
                    }
                    catch (Exception e)
                    {
                        Debug.WriteLine("Exception during onboarding: " + e.Message);
                    }
                }
            }
        }
        string GetOnboardingId(ValueSet onboardingDataValueSet)
        {
            string onboardingID =  onboardingDataValueSet["category"] as string + ":" + onboardingDataValueSet["id"];
            return onboardingID;
        }
        async Task StartOpenT2TAppServiceAnchorProcess()
        {
            if (!isStartUpTaskRunning)
            {
                await Windows.ApplicationModel.FullTrustProcessLauncher.LaunchFullTrustProcessForCurrentAppAsync();
            }
        }

        ApplicationDataCompositeValue ValueSetToAppDataCompositeValue(ValueSet valuset)
        {
            ApplicationDataCompositeValue appCompositeData = new ApplicationDataCompositeValue();
            foreach (var key in valuset.Keys)
            {
                appCompositeData.Add(key, valuset[key]);
            }
            return appCompositeData;
        }

        ValueSet AppDataCompositeValueToValueSet(ApplicationDataCompositeValue appCompositeData)
        {
            ValueSet values = new ValueSet();
            foreach (var key in appCompositeData.Keys)
            {
                values.Add(key, appCompositeData[key]);
            }
            return values;
        }

        async Task AddDeviceToAllJoynAsync(ValueSet onboardingData)
        {
            // Check the cached onboarding devices, whether the specified device is already onboarded.
            var appData = Windows.Storage.ApplicationData.Current.LocalSettings;
            var onboardedDevicesContainer = appData.CreateContainer(OnboardedDevicesContainerName, ApplicationDataCreateDisposition.Always);
            
            if (!onboardedDevicesContainer.Values.ContainsKey(GetOnboardingId(onboardingData)))
            {
                /// Populate DeviceInfo object
                var device = new BridgeRT.DeviceInfo();
                device.Name = onboardingData["name"] as string;
                device.Props = onboardingData["props"] as string;

                var translatorJsSharingToken = onboardingData["translatorJs"] as string;
                var schemaSharingToken = onboardingData["schema"] as string;

                var schemaFile = await SharedStorageAccessManager.RedeemTokenForFileAsync(schemaSharingToken);
                var schema = await FileIO.ReadTextAsync(schemaFile);
                var translatorJsFile = await SharedStorageAccessManager.RedeemTokenForFileAsync(translatorJsSharingToken);
                var translatorJs = await FileIO.ReadTextAsync(translatorJsFile);

                var bridge = new BridgeRT.BridgeJs();

                await bridge.AddDeviceAsync(device, schema, translatorJs, ".");
                StringBuilder responseBuilder = new StringBuilder();

                // Cache the onboarding data and add the device to the bridge
                var translatorsContainer = appData.CreateContainer(onboardingData["category"] as string, ApplicationDataCreateDisposition.Always);
                ApplicationDataCompositeValue onboardingDataCompositeValue = ValueSetToAppDataCompositeValue(onboardingData);
                if (!translatorsContainer.Values.ContainsKey(GetOnboardingId(onboardingData)))
                {
                    translatorsContainer.Values.Add(GetOnboardingId(onboardingData), onboardingDataCompositeValue);
                }

                // Cache the device info so that it can be onboarded when the app service starts again.
                onboardedDevicesContainer.Values.Add(GetOnboardingId(onboardingData), onboardingDataCompositeValue);
            }
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used such as when the application is launched to open a specific file.
        /// </summary>
        /// <param name="e">Details about the launch request and process.</param>
        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {

#if DEBUG
            if (System.Diagnostics.Debugger.IsAttached)
            {
                this.DebugSettings.EnableFrameRateCounter = false;
            }
#endif

            Frame rootFrame = Window.Current.Content as Frame;

            // Do not repeat app initialization when the Window already has content,
            // just ensure that the window is active
            if (rootFrame == null)
            {
                // Create a Frame to act as the navigation context and navigate to the first page
                rootFrame = new Frame();
                // Set the default language
                rootFrame.Language = Windows.Globalization.ApplicationLanguages.Languages[0];

                rootFrame.NavigationFailed += OnNavigationFailed;

                if (e.PreviousExecutionState == ApplicationExecutionState.Terminated)
                {
                    //TODO: Load state from previously suspended application
                }

                // Place the frame in the current Window
                Window.Current.Content = rootFrame;
            }

            if (rootFrame.Content == null)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame.Navigate(typeof(MainPage), e.Arguments);
            }
            // Ensure the current window is active
            Window.Current.Activate();
        }

        private Frame CreateRootFrame()
        {
            Frame rootFrame = Window.Current.Content as Frame;

            // Do not repeat app initialization when the Window already has content,
            // just ensure that the window is active
            if (rootFrame == null)
            {
                // Create a Frame to act as the navigation context and navigate to the first page
                rootFrame = new Frame();

                // Set the default language
                rootFrame.Language = Windows.Globalization.ApplicationLanguages.Languages[0];
                rootFrame.NavigationFailed += OnNavigationFailed;

                // Place the frame in the current Window
                Window.Current.Content = rootFrame;
            }

            return rootFrame;
        }

        /// <summary>
        /// Invoked when Navigation to a certain page fails
        /// </summary>
        /// <param name="sender">The Frame which failed navigation</param>
        /// <param name="e">Details about the navigation failure</param>
        void OnNavigationFailed(object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }

        // Add any application contructor code in here.
        partial void Construct();
    }
}
