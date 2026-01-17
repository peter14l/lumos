using System;
using System.IO;

namespace Lumos.UI.Services
{
    public static class Logger
    {
        private static readonly string LogPath = Path.Combine(Path.GetTempPath(), "Lumos_UI.log");
        private static readonly object LockObj = new object();

        public static void Log(string message)
        {
            try
            {
                lock (LockObj)
                {
                    var logMessage = $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff}] {message}\n";
                    File.AppendAllText(LogPath, logMessage);
                }
            }
            catch
            {
                // Ignore logging errors
            }
        }

        public static void LogError(string message, Exception ex)
        {
            Log($"[ERROR] {message}: {ex.Message}\nStack Trace: {ex.StackTrace}");
        }

        public static void LogWarning(string message)
        {
            Log($"[WARNING] {message}");
        }
    }
}
