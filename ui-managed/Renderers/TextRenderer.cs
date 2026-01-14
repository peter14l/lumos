using System;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace Lumos.UI.Renderers
{
    public class TextRenderer : IRenderer
    {
        private static readonly string[] SupportedExtensions = {
            ".txt", ".md", ".json", ".xml", ".log", ".cs", ".cpp", ".h", ".hpp",
            ".py", ".js", ".ts", ".html", ".css", ".yaml", ".yml", ".ini", ".cfg"
        };

        private const int MaxLines = 10000;
        private const int MaxFileSize = 10 * 1024 * 1024; // 10MB

        public bool CanHandle(string extension)
        {
            return Array.Exists(SupportedExtensions, ext => ext.Equals(extension, StringComparison.OrdinalIgnoreCase));
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            var fileInfo = new FileInfo(filePath);
            if (fileInfo.Length > MaxFileSize)
            {
                return CreateErrorText($"File too large to preview ({fileInfo.Length / 1024 / 1024}MB)");
            }

            var content = await File.ReadAllTextAsync(filePath, cancellationToken);
            
            var lines = content.Split('\n');
            if (lines.Length > MaxLines)
            {
                content = string.Join('\n', lines.Take(MaxLines)) + 
                         $"\n\n... (truncated, showing {MaxLines} of {lines.Length} lines)";
            }

            var textBox = new TextBox
            {
                Text = content,
                IsReadOnly = true,
                TextWrapping = TextWrapping.NoWrap,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
                FontFamily = new FontFamily("Consolas, Courier New"),
                FontSize = 12,
                Padding = new Thickness(10),
                MaxWidth = 1000,
                MaxHeight = 800,
                Background = Brushes.White,
                BorderThickness = new Thickness(0)
            };

            return textBox;
        }

        private UIElement CreateErrorText(string message)
        {
            return new TextBlock
            {
                Text = message,
                Foreground = Brushes.Red,
                FontSize = 14,
                Padding = new Thickness(20)
            };
        }
    }
}
