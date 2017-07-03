using System;
using System.Data.Odbc;
using System.Text;

namespace DbgSvcConfiguration
{
    public class CmdLineArgs
    {
        readonly string[] _args;

        public string OutDir { get; set; }
        public string[] ProcessNames { get; set; }

        public CmdLineArgs(string[] args)
        {
            _args = args;
            ProcessNames = new string[0];
        }

        public void Parse(StringBuilder errors)
        {
            for (var i = 0; i < _args.Length; i++)
            {
                var a = _args[i];
                switch (a)
                {
                    case "-od":
                        OutDir = _args[++i];
                        break;
                    case "-pn":
                        ProcessNames = _args[++i].Split(',');
                        break;
                    default:
                        errors.AppendLine($"Unknown argument: {a}");
                        break;
                }
            }
        }

        public void Verify(StringBuilder errors)
        {

            if (string.IsNullOrWhiteSpace(OutDir))
                errors.AppendLine("Missing OutDir");
            if (ProcessNames.Length==0)
                errors.AppendLine("No process names found");
            if (errors.Length==0)
                OutDir.EnsureDirectoryExist();
        }
    }
}
