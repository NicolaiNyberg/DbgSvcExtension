using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace SoexCh.Cs
{
    class Program
    {
        const int SmallSize = 1024;
        unsafe struct Small
        {
            public fixed byte Bytes[SmallSize];
        }

        static unsafe void TriggerSo()
        {
            Small small;
            small.Bytes[SmallSize - 1] = 1;
            TriggerSo();
        }

        static unsafe void AccessViolation()
        {
            var a = new byte[] {3};
            fixed (byte* fixedA = a)
            {
                byte* pa = fixedA;
                pa -= pa;
                *pa = 0;
            }
        }

        static void Main(string[] args)
        {
            var dumpName = Process.GetCurrentProcess().ProcessName + ".dmp";
            ConfigureUnhandledExceptionHandler(dumpName, 0xffffffff, true, OnException);
            if (args.Length > 0)
            {
                switch (args[0])
                {
                    case "av":
                        AccessViolation();
                        break;
                    case "so":
                        TriggerSo();
                        break;
                }                
            }
            ThrowOneExceptionAndCatchIt();
            RemoveExceptionHandlers();
        }

        static void ThrowOneExceptionAndCatchIt()
        {
            try
            {
                throw new Exception("sample exception");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Caught .Net exception: {ex.Message}");
            }
        }



        static void OnException(uint code)
        {
            Console.WriteLine($"Handled exception: 0x{code:X8}");
        }

        const string CrashHandlerDll = "CrashHandler.dll";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void ExceptionCallback(uint exceptionCode);

        [DllImport(CrashHandlerDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        static extern void ConfigureUnhandledExceptionHandler(string dumpFileName, uint dumpType, bool failInVectoredHandler, ExceptionCallback exceptionCallback);

        [DllImport(CrashHandlerDll, CallingConvention = CallingConvention.Cdecl)]
        static extern void RemoveExceptionHandlers();

    }
}
