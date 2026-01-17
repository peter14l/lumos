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

                // Initialize asynchronously without blocking the UI thread or window creation
                InitializeWebView(webView, filePath);

                return Task.FromResult<UIElement>(webView);
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

        private async void InitializeWebView(WebView2 webView, string filePath)
        {
            try
            {
                Logger.Log("PDFRenderer: Initializing WebView2 (Async)...");
                await webView.EnsureCoreWebView2Async();
                Logger.Log("PDFRenderer: WebView2 initialized successfully");

                // Load PDF using file:// protocol
                Logger.Log($"PDFRenderer: Navigating to {filePath}");
                webView.Source = new Uri(filePath, UriKind.Absolute);
            }
            catch (Exception ex)
            {
                Logger.LogError("PDFRenderer: WebView2 initialization failed", ex);
            }
        }
    }
}
