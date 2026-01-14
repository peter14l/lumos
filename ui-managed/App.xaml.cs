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

            // Start IPC server
            _ipcServer = new IPCServer();
            _ipcServer.Start();

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
