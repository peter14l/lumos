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
    public class FolderRenderer : IRenderer
    {
        public bool CanHandle(string extension)
        {
            // Handle explicit .folder extension or if we can determine it's a directory
            // Note: The IPC request should ideally send "directory" or similar, but for now checking empty extension or specific flag
            return extension == ".folder" || string.IsNullOrEmpty(extension); 
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
             // 1. Gather data on background thread
             var (dirs, files, error) = await Task.Run(() => 
             {
                 try 
                 {
                     if (!Directory.Exists(filePath)) return ((string[]?)null, (string[]?)null, "Directory not found");
                     
                     var d = Directory.GetDirectories(filePath).Take(10).Select(p => Path.GetFileName(p)).ToArray();
                     var f = Directory.GetFiles(filePath).Take(20).Select(p => Path.GetFileName(p)).ToArray();
                     return ((string[]?)d, (string[]?)f, (string?)null);
                 }
                 catch (Exception ex)
                 {
                     return ((string[]?)null, (string[]?)null, ex.Message);
                 }
             }, cancellationToken);

             // 2. Create UI on UI thread
             if (error != null) return CreateErrorText(error);

             var grid = new Grid
             {
                 Width = 400,
                 Height = 400,
                 Background = Brushes.White
             };

             grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
             grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });

             // Header
             var header = new TextBlock 
             { 
                Text = "📁 " + Path.GetFileName(filePath), 
                FontSize = 16, 
                FontWeight = FontWeights.Bold, 
                Padding = new Thickness(10),
                Background = new SolidColorBrush(Color.FromRgb(240, 240, 240))
             };
             Grid.SetRow(header, 0);
             grid.Children.Add(header);

             var stack = new StackPanel { Margin = new Thickness(10) };
             
             if (dirs != null)
             {
                foreach (var d in dirs) stack.Children.Add(new TextBlock { Text = "📁 " + d, Margin = new Thickness(2), FontWeight = FontWeights.SemiBold });
             }
             
             if (files != null)
             {
                foreach (var f in files) stack.Children.Add(new TextBlock { Text = "📄 " + f, Margin = new Thickness(2, 2, 2, 6), Foreground = Brushes.DarkSlateGray });
             }
             
             // Add "...and more" if likely truncated (heuristic)
             if ((dirs?.Length ?? 0) == 10 || (files?.Length ?? 0) == 20)
             {
                 stack.Children.Add(new TextBlock { Text = "... and more items", FontStyle = FontStyles.Italic, Foreground = Brushes.Gray, Margin = new Thickness(5) });
             }

             var scroll = new ScrollViewer { Content = stack, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };
             Grid.SetRow(scroll, 1);
             grid.Children.Add(scroll);

             return grid;
        }

        private UIElement CreateErrorText(string message)
        {
            return new TextBlock
            {
                Text = message,
                Foreground = Brushes.Red,
                Padding = new Thickness(10)
            };
        }
    }
}
