using System.IO;

namespace DbgSvcConfiguration
{
    public static class StringExtensions
    {
        public static void EnsureDirectoryExist(this string d)
        {
            if (!Directory.Exists(d))
                Directory.CreateDirectory(d);
        }
    }
}
