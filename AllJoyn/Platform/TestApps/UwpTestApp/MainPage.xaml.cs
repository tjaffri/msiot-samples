//
// Copyright (c) 2015, Microsoft Corporation
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

using System;
using Windows.ApplicationModel;
using Windows.Storage;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using System.Threading.Tasks;

using BridgeRT;
using Windows.Foundation.Collections;
using Windows.ApplicationModel.DataTransfer;
using System.IO;
using Windows.ApplicationModel.AppService;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace UwpTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        BridgeJs bridge;
        bool gotError;
        AppServiceConnection openT2TBridgeArchitestrationService = null;

        public MainPage()
        {
            this.InitializeComponent();

            bridge = new BridgeJs();
            bridge.Error += Bridge_Error;

            // Set the default values.
            deviceName.Text = "My test device";
            vendor.Text = "Test vendor";
            model.Text = "light";
            props.Text = "{\"id\":\"1833441\",\"access_token\":\"ZxYq1n4Sl_c72e3zPFzq51yv580IhX9c\"}";
            javascriptFileName.Text = "device.js";
            schemaFileName.Text = "device.xml";
            modulesPath.Text = ".";
        }

        private async void Bridge_Error(object sender, string e)
        {
            this.gotError = true;
            await this.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                msg.Text = e;
            });
        }

        private void InitButton_Click(object sender, RoutedEventArgs e)
        {
            int hr = bridge.Initialize();
            if (hr != 0)
            {
                msg.Text = "Initialization failed, hr=" + hr.ToString();
            }
            else
            {
                msg.Text = "DSB Bridge initialization done";
            }
        }

        private async void AddButton_Click(object sender, RoutedEventArgs e)
        {
            this.gotError = false;
            try
            {
                DeviceInfo device = new DeviceInfo();
                device.Name = deviceName.Text;
                device.Vendor = vendor.Text;
                device.Model = model.Text;
                device.Props = props.Text;

                string schemaFile = schemaFileName.Text;
                string jsFile = javascriptFileName.Text;
                string modPath = modulesPath.Text;

                if (AppSvcChkBox.IsChecked == false) // Directly call into bridge
                {
                    // Load the schema
                    string baseXml = await FileIO.ReadTextAsync(await Package.Current.InstalledLocation.GetFileAsync(schemaFile));
                    // Load the javascript
                    string script = await FileIO.ReadTextAsync(await Package.Current.InstalledLocation.GetFileAsync(jsFile));

                    await bridge.AddDeviceAsync(device, baseXml, script, modPath);
                    msg.Text = "Device added";
                }
                else // Go through App Service, which in turns call into and hosts the bridge.
                {
                    // Add the connection.
                    if (openT2TBridgeArchitestrationService == null)
                    {
                        openT2TBridgeArchitestrationService = new AppServiceConnection();

                        openT2TBridgeArchitestrationService.AppServiceName = "CommunicationService";
                        openT2TBridgeArchitestrationService.PackageFamilyName = "OpenT2TAllJoynBridgeHost_a6049pvczh34w"; // Microsoft.AppServiceBridge_8wekyb3d8bbwe";

                        var status = await openT2TBridgeArchitestrationService.OpenAsync();
                        if (status != AppServiceConnectionStatus.Success)
                        {
                            msg.Text = "Failed to connect to App Service: " + status.ToString();
                            return;
                        }
                    }
                    openT2TBridgeArchitestrationService.RequestReceived += OpenT2TBridgeArchitestrationService_RequestReceived;
                    openT2TBridgeArchitestrationService.ServiceClosed += OpenT2TBridgeArchitestrationService_ServiceClosed;

                    // Get the schema and translaor files files
                    var deviceInfo = await GetBridgeAppServiceValueSet(device.Name, device.Props, jsFile, schemaFile);
                    var response = await openT2TBridgeArchitestrationService.SendMessageAsync(deviceInfo);
                    if (response.Status == AppServiceResponseStatus.Success)
                    {
                        msg.Text = response.Message["response"] as string;
                    }
                    else
                    {
                        msg.Text = "App service Response failure code: " + response.Status.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                if (!this.gotError)
                {
                    msg.Text = "AddDevice exception: " + ex.Message;
                }
            }
        }

        private void OpenT2TBridgeArchitestrationService_ServiceClosed(AppServiceConnection sender, AppServiceClosedEventArgs args)
        {
            msg.Text = "App Service closed";
            openT2TBridgeArchitestrationService = null;
        }

        private void OpenT2TBridgeArchitestrationService_RequestReceived(AppServiceConnection sender, AppServiceRequestReceivedEventArgs args)
        {
            throw new NotImplementedException();
        }

        private async Task<ValueSet> GetBridgeAppServiceValueSet(string name, string props, string jsFile, string schemaFile)
        {
            // Get the data from application data (cached). If not present, create and add.
            var appData = Windows.Storage.ApplicationData.Current.LocalSettings;
            var translatorsContainer = appData.CreateContainer("Translators", ApplicationDataCreateDisposition.Always);

            object translatorJsSharingToken = null;
            if (!translatorsContainer.Values.TryGetValue("translatorJs", out translatorJsSharingToken))
            {
                StorageFile translatorJs = await Package.Current.InstalledLocation.GetFileAsync(jsFile);
                translatorJsSharingToken = SharedStorageAccessManager.AddFile(translatorJs);
                translatorsContainer.Values.Add("translatorJs", translatorJsSharingToken);
            }

            ValidateForNonNullData(translatorJsSharingToken, "Translator JavaScript File sharing token");

            object schemaSharingToken = null;
            if (!translatorsContainer.Values.TryGetValue("schema", out schemaSharingToken))
            {
                StorageFile schema = await Package.Current.InstalledLocation.GetFileAsync(schemaFile);
                schemaSharingToken = SharedStorageAccessManager.AddFile(schema);
                translatorsContainer.Values.Add("schema", schemaSharingToken);
            }
            ValidateForNonNullData(schemaSharingToken, "Schema File sharing token");

            // Build Valueset
            ValueSet deviceInfo = new ValueSet();
            deviceInfo.Add("command", "AddDevice");
            deviceInfo.Add("translatorJs", translatorJsSharingToken);
            deviceInfo.Add("schema", schemaSharingToken);
            deviceInfo.Add("name", name);
            deviceInfo.Add("props", props); // Props must contain 'id:<deviceid>', to (locally) unique identify the device.
            deviceInfo.Add("category", "TestDevice"); // Used to create a data container at the app service side, to cache the onbairding details. ex: WinkLightBulb, WinkThermostat, NestThermostat

            return deviceInfo;
        }
        private void ValidateForNonNullData(object o, string name)
        {
            if (o == null)
            {
                throw new InvalidDataException(name + " must be valid");
            }
        }
    }
}
