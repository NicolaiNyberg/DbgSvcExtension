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
            ConfigureUnhandledExceptionHandler(dumpName, true);
            if (args.Length>0)
                AccessViolation();
            else
                TriggerSo();
        }

        [DllImport("CrashHandler.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        static extern void ConfigureUnhandledExceptionHandler(string dumpFileName, bool isDotNet);

    }
}
