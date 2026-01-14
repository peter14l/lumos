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
            return await Task.Run(() =>
            {
                cancellationToken.ThrowIfCancellationRequested();

                var image = new Image
                {
                    MaxWidth = MaxResolution,
                    MaxHeight = MaxResolution,
                    Stretch = System.Windows.Media.Stretch.Uniform
                };

                var bitmap = new BitmapImage();
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.DecodePixelWidth = MaxResolution; // Cap resolution
                bitmap.UriSource = new Uri(filePath, UriKind.Absolute);
                bitmap.EndInit();
                bitmap.Freeze(); // Make thread-safe

                cancellationToken.ThrowIfCancellationRequested();

                image.Source = bitmap;
                return image;
            }, cancellationToken);
        }
    }
}
