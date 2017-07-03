namespace DbgSvcConfiguration
{
    public class ConfigureDbgSvc
    {
        readonly CmdLineArgs _args;

        public ConfigureDbgSvc(CmdLineArgs args)
        {
            _args = args;
        }

        public void Execute()
        {
            // create config.xml from process names

            // crate ServiceState.xml from process names

            // foreach process name, create per-process-vbs file

            // create DbgSvc.vbs from process names
        }
    }
}
