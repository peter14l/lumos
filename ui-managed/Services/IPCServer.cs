using System;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using Lumos.Contracts;

namespace Lumos.UI.Services
{
    public class IPCServer
    {
        private const string PipeName = "LumosPreview";
        private static readonly string LogPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "Lumos", "ui-debug.log");
        
        private CancellationTokenSource? _cancellationTokenSource;
        private Task? _serverTask;

        private static void Log(string message)
        {
            try
            {
                var dir = Path.GetDirectoryName(LogPath);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                    Directory.CreateDirectory(dir);
                    
                var line = $"[{DateTime.Now:HH:mm:ss.fff}] {message}{Environment.NewLine}";
                File.AppendAllText(LogPath, line);
            }
            catch { }
        }

        public void Start()
        {
            Log("IPCServer starting...");
            _cancellationTokenSource = new CancellationTokenSource();
            _serverTask = Task.Run(() => ServerLoop(_cancellationTokenSource.Token));
            Log("IPCServer started");
        }

        public void Stop()
        {
            _cancellationTokenSource?.Cancel();
            _serverTask?.Wait(TimeSpan.FromSeconds(2));
        }

        private async Task ServerLoop(CancellationToken cancellationToken)
        {
            Log("ServerLoop started, waiting for connections...");
            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    using var pipeServer = new NamedPipeServerStream(
                        PipeName,
                        PipeDirection.InOut,
                        1,
                        PipeTransmissionMode.Byte,
                        PipeOptions.Asynchronous
                    );

                    Log("Pipe created, waiting for connection...");
                    
                    // Wait for client connection
                    await pipeServer.WaitForConnectionAsync(cancellationToken);
                    Log("Client connected to pipe");

                    // Read message
                    using var reader = new StreamReader(pipeServer, Encoding.UTF8);
                    var json = await reader.ReadToEndAsync();
                    Log($"Received JSON: {json}");

                    if (!string.IsNullOrEmpty(json))
                    {
                        // Deserialize preview request
                        var request = JsonSerializer.Deserialize<PreviewRequest>(json);
                        Log($"Deserialized request - Path: {request?.Path}, Extension: {request?.Extension}");
                        
                        if (request != null)
                        {
                            // Dispatch to UI thread
                            Log("Dispatching to UI thread...");
                            await Application.Current.Dispatcher.InvokeAsync(async () =>
                            {
                                try
                                {
                                    Log("On UI thread now");
                                    var window = Application.Current.MainWindow as PreviewWindow;
                                    Log($"MainWindow is PreviewWindow: {window != null}");
                                    
                                    if (window != null)
                                    {
                                        Log("Calling ShowPreview...");
                                        await window.ShowPreview(request);
                                        Log("ShowPreview completed");
                                    }
                                    else
                                    {
                                        Log("MainWindow is null or not PreviewWindow!");
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Log($"Error in UI dispatch: {ex}");
                                }
                            });
                        }
                    }
                }
                catch (OperationCanceledException)
                {
                    Log("Server loop cancelled");
                    break;
                }
                catch (Exception ex)
                {
                    Log($"IPC Server error: {ex}");
                }
            }
            Log("ServerLoop ended");
        }
    }
}
