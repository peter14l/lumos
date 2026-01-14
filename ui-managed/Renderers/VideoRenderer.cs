using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace Lumos.UI.Renderers
{
    public class VideoRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = {
            ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm"
        };

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            return await Task.Run(() =>
            {
                var mediaElement = new MediaElement
                {
                    Source = new Uri(filePath, UriKind.Absolute),
                    LoadedBehavior = MediaState.Manual,
                    MaxWidth = 1000,
                    MaxHeight = 700,
                    Stretch = System.Windows.Media.Stretch.Uniform
                };

                // Auto-play with mute
                mediaElement.Volume = 0;
                mediaElement.Loaded += (s, e) => mediaElement.Play();

                return mediaElement;
            }, cancellationToken);
        }
    }
}
