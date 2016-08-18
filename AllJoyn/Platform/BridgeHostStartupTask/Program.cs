//*********************************************************
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
using System.Linq;
using System.Threading;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.ApplicationModel.AppService;
using System.Threading.Tasks;

namespace OpenT2T.Bridge
{
    class Program
    {
        static AppServiceConnection connection = null;
        static bool isConnectionClosed = true;

        /// <summary>
        /// Creates an app service thread. Does ping-pong (health check) with app service and Restarts the app service, if required.
        /// Keeps running in the background, unless explicitly terminated.
        /// </summary>
        static void Main(string[] args)
        {
            Thread appServiceThread = new Thread(new ThreadStart(ThreadProc));
            appServiceThread.Start();
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("*****************************");
            Console.WriteLine("**** OpenT2T AllJoyn Bridge's background startup task ****");
            Console.WriteLine("*****************************");
            Console.ReadLine();
            while (true) ;
        }

        /// <summary>
        /// Creates the app service connection
        /// </summary>
        static async void ThreadProc()
        {
            while (true)
            {
                if (isConnectionClosed ||
                    !(await isHealthy()))
                {
                    connection = new AppServiceConnection();
                    connection.AppServiceName = "CommunicationService";
                    connection.PackageFamilyName = "OpenT2TAllJoynBridgeHost_a6049pvczh34w";
                    isConnectionClosed = false;
                    AppServiceConnectionStatus status = await connection.OpenAsync();
                    switch (status)
                    {
                        case AppServiceConnectionStatus.Success:
                            Console.ForegroundColor = ConsoleColor.Green;
                            connection.RequestReceived += Connection_RequestReceived;
                            connection.ServiceClosed += Connection_ServiceClosed;
                            Console.WriteLine("Connection established - waiting for requests");
                            Console.WriteLine();
                            break;
                        case AppServiceConnectionStatus.AppNotInstalled:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("The app AppServicesProvider is not installed.");
                            break;
                        case AppServiceConnectionStatus.AppUnavailable:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("The app AppServicesProvider is not available.");
                            break;
                        case AppServiceConnectionStatus.AppServiceUnavailable:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine(string.Format("The app AppServicesProvider is installed but it does not provide the app service {0}.", connection.AppServiceName));
                            break;
                        case AppServiceConnectionStatus.Unknown:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine(string.Format("An unkown error occurred while we were trying to open an AppServiceConnection."));
                            break;
                    }
                }

                Thread.Sleep(5000); // Do Health checks every 5 seconds
            } // while()
        }

        private static void Connection_ServiceClosed(AppServiceConnection sender, AppServiceClosedEventArgs args)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            isConnectionClosed = true;
            Console.WriteLine(string.Format("AppServiceConnection is closed!: Status : " + args.Status));
        }

        /// <summary>
        /// Receives message from UWP app and sends a response back
        /// </summary>
        private static void Connection_RequestReceived(AppServiceConnection sender, AppServiceRequestReceivedEventArgs args)
        {
            throw new NotImplementedException();
        }
        private static async  Task<bool> isHealthy()
        {
            bool healthy = false;
            if (connection != null)
            {
                ValueSet valueSet = new ValueSet();
                valueSet.Add("command", "healthcheck");
                try
                {
                    AppServiceResponse response = await connection.SendMessageAsync(valueSet);
                    healthy = (response.Message["response"] as string).Equals("healthy");
                    Console.WriteLine("Is Healthy : " + healthy);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Exception during health check: " + e.Message);
                    healthy = false;
                }
            }

            return healthy;
        }
    }
}
