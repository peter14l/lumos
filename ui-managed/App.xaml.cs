using System.Windows;
using Lumos.UI.Services;

namespace Lumos.UI
{
    public partial class App : Application
    {
        private IPCServer? _ipcServer;

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            Logger.Log("Lumos UI starting up...");

            // Setup global exception handling
            AppDomain.CurrentDomain.UnhandledException += (s, args) =>
            {
                Logger.LogError("Unhandled AppDomain exception", (Exception)args.ExceptionObject);
            };

            DispatcherUnhandledException += (s, args) =>
            {
                Logger.LogError("Unhandled Dispatcher exception", args.Exception);
            };

            try
            {
                Logger.Log("Starting IPC Server...");
                _ipcServer = new IPCServer();
                _ipcServer.Start();
                Logger.Log("IPC Server started");
            }
            catch (Exception ex)
            {
                Logger.LogError("Failed to start IPC Server", ex);
            }

            // Don't show window on startup - it will be shown when preview request arrives
            if (MainWindow != null)
            {
                MainWindow.Hide();
            }
        }

        protected override void OnExit(ExitEventArgs e)
        {
            _ipcServer?.Stop();
            base.OnExit(e);
        }
    }
}
