using System;
using System.IO;
using System.IO.Compression;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace Lumos.UI.Renderers
{
    public class OfficeRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = { ".docx", ".xlsx", ".pptx", ".odt", ".ods", ".odp" };

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            // Gather info on background thread
            var info = await Task.Run(() =>
            {
                try
                {
                    var fileInfo = new FileInfo(filePath);
                    return new
                    {
                        Name = fileInfo.Name,
                        Size = $"{fileInfo.Length / 1024.0:F1} KB",
                        Created = fileInfo.CreationTime.ToString("g"),
                        Modified = fileInfo.LastWriteTime.ToString("g"),
                        Type = fileInfo.Extension.ToUpper() + " Document"
                    };
                }
                catch
                {
                    return null;
                }
            }, cancellationToken);

            if (info == null) return new TextBlock { Text = "Error reading file." };

            // UI construction
            var grid = new Grid
            {
                Width = 350,
                Height = 250,
                Background = Brushes.White
            };

            var stack = new StackPanel { Margin = new Thickness(20), VerticalAlignment = VerticalAlignment.Center };

            // Icon/Type
            stack.Children.Add(new TextBlock 
            { 
                Text = "📄", 
                FontSize = 48, 
                HorizontalAlignment = HorizontalAlignment.Center,
                Margin = new Thickness(0,0,0,10)
            });

            stack.Children.Add(new TextBlock 
            { 
                Text = info.Name, 
                FontSize = 16, 
                FontWeight = FontWeights.Bold, 
                TextWrapping = TextWrapping.Wrap,
                HorizontalAlignment = HorizontalAlignment.Center,
                TextAlignment = TextAlignment.Center
            });

            stack.Children.Add(new TextBlock 
            { 
                Text = info.Type, 
                Foreground = Brushes.Gray,
                HorizontalAlignment = HorizontalAlignment.Center,
                Margin = new Thickness(0, 5, 0, 15)
            });

             stack.Children.Add(new TextBlock { Text = $"Size: {info.Size}" });
             stack.Children.Add(new TextBlock { Text = $"Modified: {info.Modified}" });

             grid.Children.Add(stack);
             return grid;
        }
    }
}
