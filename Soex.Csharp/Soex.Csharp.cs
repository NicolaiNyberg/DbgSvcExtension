using System.Threading;

namespace Soex.Csharp
{
    class Program
    {
        const int SmallSize = 1024;
        unsafe struct Small
        {
            public fixed byte Bytes[SmallSize];
        }

        static void Main(string[] args)
        {
            Thread.Sleep(1 * 1000); // this is enough to give DbgSvc time to attach to the process
            TriggerSo();
            AccessViolation();
        }

        static unsafe void TriggerSo()
        {
            Small small;
            small.Bytes[SmallSize - 1] = 1;
            TriggerSo();
        }

        static unsafe void AccessViolation()
        {
            var a = new byte[] { 3 };
            fixed (byte* fixedA = a)
            {
                byte* pa = fixedA;
                pa -= pa;
                *pa = 0;
            }
        }

        static int MainWrapper(string args)
        {
            Main(args.Split(' '));
            return 0;
        }
    }
}


