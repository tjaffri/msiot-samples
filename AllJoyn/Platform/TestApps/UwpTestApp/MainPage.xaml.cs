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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using BridgeRT;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace UwpTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        BridgeJs bridge;

        public MainPage()
        {
            this.InitializeComponent();

            bridge = new BridgeJs();


            // Set the default values.
            deviceName.Text = "My test device";
            vendor.Text = "Test vendor";
            model.Text = "light";
            props.Text = "{\"AuthToken\": \"d26e46dd-e8e3-4048-9164-3cb3c79d7dac\", \"ControlId\": \"81d21115-cf03-4ebb-8517-7b5e851bf0d9\", \"ControlString\": \"/switch\" }";
            javascriptFileName.Text = "device.js";
            schemaFileName.Text = "device.xml";
            modulesPath.Text = ".";
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

                // make this async as it could take awhile
                var s = await Task.Run(() =>
                {
                    // Load the schema
                    string baseXml = File.ReadAllText(schemaFile);
                    // Load the javascript
                    string script = File.ReadAllText(jsFile);

                    bridge.AddDevice(device, baseXml, script, modPath);
                    return 0;
                });
                msg.Text = "Device added";
            }
            catch (Exception ex)
            {
                msg.Text = "AddDevice exception: " + ex.Message;
            }
        }
    }
}
