using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;

namespace Lumos.UI.Renderers
{
    public class ImageRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = {
            ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".ico"
        };

        private const int MaxResolution = 3840; // 4K

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            // Load bitmap on background thread
            var bitmap = await Task.Run(() =>
            {
                cancellationToken.ThrowIfCancellationRequested();

                var bmp = new BitmapImage();
                bmp.BeginInit();
                bmp.CacheOption = BitmapCacheOption.OnLoad;
                bmp.DecodePixelWidth = MaxResolution; // Cap resolution
                bmp.UriSource = new Uri(filePath, UriKind.Absolute);
                bmp.EndInit();
                bmp.Freeze(); // Make thread-safe

                cancellationToken.ThrowIfCancellationRequested();

                return bmp;
            }, cancellationToken);

            // Create Image control on UI thread (this method is called from UI thread via Dispatcher)
            var screenWidth = SystemParameters.PrimaryScreenWidth;
            var screenHeight = SystemParameters.PrimaryScreenHeight;

            var image = new Image
            {
                MaxWidth = screenWidth * 0.5,
                MaxHeight = screenHeight * 0.5,
                Stretch = System.Windows.Media.Stretch.Uniform,
                Source = bitmap
            };

            return image;
        }
    }
}
