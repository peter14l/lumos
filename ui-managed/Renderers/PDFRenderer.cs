using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.Wpf;
using Lumos.UI.Services;

namespace Lumos.UI.Renderers
{
    public class PDFRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = { ".pdf" };

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            try
            {
                Logger.Log($"PDFRenderer: Creating WebView2 for {filePath}");
                
                // Container to hold either WebView2 or Native Renderer result
                var container = new Border { Background = System.Windows.Media.Brushes.White };

                // MSIX apps cannot write to the install directory.
                var userDataFolder = System.IO.Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "Lumos",
                    "WebView2");

                var webView = new WebView2
                {
                    Width = 800,
                    Height = 600,
                    CreationProperties = new CoreWebView2CreationProperties
                    {
                        UserDataFolder = userDataFolder
                    }
                };

                container.Child = webView;

                // Fire-and-forget initialization with fallback logic
                InitializeWithFallbackAsync(container, webView, filePath, cancellationToken);

                return Task.FromResult<UIElement>(container);
            }
            catch (Exception ex)
            {
                Logger.LogError("PDFRenderer error", ex);
                return Task.FromResult<UIElement>(new TextBlock
                {
                    Text = $"Error creating PDF preview: {ex.Message}",
                    Foreground = System.Windows.Media.Brushes.Red
                });
            }
        }

        private async void InitializeWithFallbackAsync(Border container, WebView2 webView, string filePath, CancellationToken cancellationToken)
        {
            try
            {
                Logger.Log("PDFRenderer: Initializing WebView2 (Async)...");
                
                // Create a cancellation token source for the timeout
                using var timeoutCts = new CancellationTokenSource(TimeSpan.FromSeconds(2));
                using var linkedCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, timeoutCts.Token);

                try 
                {
                    await webView.EnsureCoreWebView2Async().WaitAsync(linkedCts.Token);
                    Logger.Log("PDFRenderer: WebView2 initialized successfully");
                    
                    if (!cancellationToken.IsCancellationRequested)
                    {
                        Logger.Log($"PDFRenderer: Navigating to {filePath}");
                        webView.Source = new Uri(filePath, UriKind.Absolute);
                    }
                }
                catch (OperationCanceledException) when (timeoutCts.IsCancellationRequested)
                {
                     Logger.LogWarning("PDFRenderer: WebView2 initialization timed out. Switching to Native fallback.");
                     await SwitchToNativeRenderer(container, filePath, cancellationToken);
                }
                catch (Exception ex)
                {
                    Logger.LogError("PDFRenderer: WebView2 initialization failed. Switching to Native fallback.", ex);
                    await SwitchToNativeRenderer(container, filePath, cancellationToken);
                }
            }
            catch (Exception ex)
            {
                Logger.LogError("PDFRenderer critical error in async init", ex);
            }
        }

        private async Task SwitchToNativeRenderer(Border container, string filePath, CancellationToken cancellationToken)
        {
            // Must be on UI thread to modify container.Child
            await Application.Current.Dispatcher.InvokeAsync(async () => 
            {
                try
                {
                    // Dispose WebView2 if it exists
                    if (container.Child is WebView2 wv)
                    {
                        wv.Dispose();
                    }

                    container.Child = new TextBlock { Text = "Loading native preview...", HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };

                    var nativeRenderer = new NativePdfRenderer();
                    var nativeContent = await nativeRenderer.RenderAsync(filePath, cancellationToken);
                    
                    container.Child = nativeContent;
                    Logger.Log("PDFRenderer: Switched to Native Render successfully.");
                }
                catch (Exception ex)
                {
                     Logger.LogError("PDFRenderer: Fallback to native failed.", ex);
                     container.Child = new TextBlock { Text = "Preview failed.", Foreground = System.Windows.Media.Brushes.Red };
                }
            });
        }
    }
}
