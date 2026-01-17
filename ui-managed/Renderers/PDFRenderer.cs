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

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            try
            {
                Logger.Log($"PDFRenderer: Starting render for {filePath}");
                
                // MSIX apps cannot write to the install directory.
                var userDataFolder = System.IO.Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "Lumos",
                    "WebView2");

                Logger.Log($"PDFRenderer: Setting WebView2 User Data Folder to: {userDataFolder}");

                var webView = new WebView2
                {
                    Width = 800,
                    Height = 600,
                    CreationProperties = new CoreWebView2CreationProperties
                    {
                        UserDataFolder = userDataFolder
                    }
                };

                Logger.Log("PDFRenderer: Initializing WebView2 via CreationProperties...");
                await webView.EnsureCoreWebView2Async();
                Logger.Log("PDFRenderer: WebView2 initialized");
                
                cancellationToken.ThrowIfCancellationRequested();

                // Load PDF using file:// protocol
                Logger.Log($"PDFRenderer: Navigating to {filePath}");
                webView.Source = new Uri(filePath, UriKind.Absolute);

                return webView;
            }
            catch (Exception ex)
            {
                Logger.LogError("PDFRenderer error", ex);
                return new TextBlock
                {
                    Text = $"PDF preview requires WebView2 runtime.\n\nError: {ex.Message}\n\nPlease install WebView2 runtime from Microsoft.",
                    Foreground = System.Windows.Media.Brushes.Red,
                    FontSize = 14,
                    Padding = new Thickness(20),
                    TextWrapping = TextWrapping.Wrap,
                    MaxWidth = 400
                };
            }
        }
    }
}
