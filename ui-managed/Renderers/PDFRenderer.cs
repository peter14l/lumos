using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Web.WebView2.Wpf;

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
                var webView = new WebView2
                {
                    Width = 800,
                    Height = 600
                };

                await webView.EnsureCoreWebView2Async();
                
                cancellationToken.ThrowIfCancellationRequested();

                // Load PDF using file:// protocol
                webView.Source = new Uri(filePath, UriKind.Absolute);

                return webView;
            }
            catch (Exception ex)
            {
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
