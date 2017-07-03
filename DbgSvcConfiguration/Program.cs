using System;
using System.Text;

namespace DbgSvcConfiguration
{
    class Program
    {
        static void Usage(string header, Exception ex = null)
        {
            var w = Console.Out;
            if (!string.IsNullOrEmpty(header))
            {
                w.WriteLine("Usage: -od outdir -pn comma-separated-list-of-process-names");
                w.WriteLine(header);
            }
            if (ex!=null)
                w.WriteLine(ex.Message);
            w.Flush();
        }

        static int Main(string[] args)
        {
            try
            {
                var errors = new StringBuilder();
                var cla = new CmdLineArgs(args);
                cla.Parse(errors);
                if (errors.Length == 0)
                    cla.Verify(errors);
                if (errors.Length != 0)
                {
                    Usage(errors.ToString());
                    return 1;
                }
                var c = new ConfigureDbgSvc(cla);
                c.Execute();
                return 0;
            }
            catch (Exception ex)
            {
                Usage(null, ex);
                return 1;
            }
        }
    }
}
