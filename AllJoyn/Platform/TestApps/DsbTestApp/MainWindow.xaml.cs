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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices.WindowsRuntime;
using BridgeNet;

namespace DsbTestApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        BridgeJs bridge;
        public MainWindow()
        {
            InitializeComponent();
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
                await Task.Run(async () =>
                {
                    // Load the schema
                    string baseXml = System.IO.File.ReadAllText(schemaFile);
                    // Load the javascript
                    string script = System.IO.File.ReadAllText(jsFile);

                    await this.bridge.AddDeviceAsync(device, baseXml, script, modPath);
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
