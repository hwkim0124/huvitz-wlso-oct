using System;
using WsoNativeLib;
using WsoNativeLib2;

namespace HelloWorld
{
    class Program
    {
        static WsoCallback.WsoLogMsgCallback logMsgCallback = new WsoCallback.WsoLogMsgCallback(CallbackLogMsg);

        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");
            // WsoAnalytics.InitializeWsoAnalytics();
            Bootstrapper.InitializeWsoSystem(clb: logMsgCallback, true);
        }

        static void CallbackLogMsg(string msg, int type)
        {
            Console.WriteLine($"WsoSystem Log: {msg}");
        }
    }
}