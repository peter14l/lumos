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
        private CancellationTokenSource? _cancellationTokenSource;
        private Task? _serverTask;

        public void Start()
        {
            _cancellationTokenSource = new CancellationTokenSource();
            _serverTask = Task.Run(() => ServerLoop(_cancellationTokenSource.Token));
        }

        public void Stop()
        {
            _cancellationTokenSource?.Cancel();
            _serverTask?.Wait(TimeSpan.FromSeconds(2));
        }

        private async Task ServerLoop(CancellationToken cancellationToken)
        {
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

                    // Wait for client connection
                    await pipeServer.WaitForConnectionAsync(cancellationToken);
                    Logger.Log("Client connected to pipe");

                    // Read message
                    using var reader = new StreamReader(pipeServer, Encoding.UTF8);
                    var json = await reader.ReadToEndAsync();
                    Logger.Log($"Received JSON: {json}");

                    if (!string.IsNullOrEmpty(json))
                    {
                        // Deserialize preview request
                        var request = JsonSerializer.Deserialize<PreviewRequest>(json);
                        Logger.Log($"Deserialized request - Path: {request?.Path}, Extension: {request?.Extension}");
                        if (request != null)
                        {
                            // Dispatch to UI thread
                            await Application.Current.Dispatcher.InvokeAsync(async () =>
                            {
                                Logger.Log("Dispatched to UI thread");
                                var window = Application.Current.MainWindow as PreviewWindow;
                                Logger.Log($"MainWindow is PreviewWindow: {window != null}");
                                if (window != null)
                                {
                                    Logger.Log("Calling ShowPreview...");
                                    await window.ShowPreview(request);
                                    Logger.Log("ShowPreview completed");
                                }
                            });
                        }
                    }
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    Logger.LogError("IPC Server error", ex);
                }
            }
        }
    }
}
