using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace Lumos.UI.Renderers
{
    public class AudioRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = {
            ".mp3", ".wav", ".flac", ".m4a", ".wma", ".aac", ".ogg"
        };

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            // All UI elements must be created on UI thread
            var grid = new Grid
            {
                Width = 400,
                Height = 200,
                Background = new SolidColorBrush(Color.FromRgb(240, 240, 240))
            };

            grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            // Audio icon/info
            var infoPanel = new StackPanel
            {
                VerticalAlignment = VerticalAlignment.Center,
                HorizontalAlignment = HorizontalAlignment.Center
            };

            infoPanel.Children.Add(new TextBlock
            {
                Text = "🎵",
                FontSize = 48,
                HorizontalAlignment = HorizontalAlignment.Center,
                Margin = new Thickness(0, 0, 0, 10)
            });

            infoPanel.Children.Add(new TextBlock
            {
                Text = System.IO.Path.GetFileName(filePath),
                FontSize = 14,
                HorizontalAlignment = HorizontalAlignment.Center,
                TextWrapping = TextWrapping.Wrap,
                MaxWidth = 350
            });

            Grid.SetRow(infoPanel, 0);
            grid.Children.Add(infoPanel);

            // Media element
            var mediaElement = new MediaElement
            {
                Source = new Uri(filePath, UriKind.Absolute),
                LoadedBehavior = MediaState.Manual,
                Height = 50,
                Margin = new Thickness(10)
            };

            Grid.SetRow(mediaElement, 1);
            grid.Children.Add(mediaElement);

            // Auto-play (muted for preview)
            mediaElement.Volume = 0.5;
            mediaElement.Loaded += (s, e) => mediaElement.Play();

            return Task.FromResult<UIElement>(grid);
        }
    }
}
