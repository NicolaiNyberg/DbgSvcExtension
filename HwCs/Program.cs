using System;

namespace HwCs
{
    class Program
    {
        static int MainWrapper(string args)
        {
            return Main(args.Split(' '));
        }
        static int Main(string[] args)
        {
            Console.WriteLine($"Hello world: {string.Join("|", args)}");
            return 0;
        }
    }
}
