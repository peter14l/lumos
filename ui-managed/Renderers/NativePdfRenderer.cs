using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using Windows.Data.Pdf;
using Windows.Storage;
using Windows.Storage.Streams;
using Lumos.UI.Services;

namespace Lumos.UI.Renderers
{
    public class NativePdfRenderer : IRenderer
    {
        public bool CanRender(string extension)
        {
            return extension.Equals(".pdf", StringComparison.OrdinalIgnoreCase);
        }

        public async Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken)
        {
            try
            {
                Logger.Log($"NativePdfRenderer: Starting render for {filePath}");

                var scrollView = new ScrollViewer
                {
                    VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                    HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled,
                    PanningMode = PanningMode.VerticalOnly
                };

                var stackPanel = new StackPanel
                {
                    Orientation = Orientation.Vertical,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                scrollView.Content = stackPanel;

                // Load PDF explicitly on a background thread to avoid UI freeze
                await Task.Run(async () =>
                {
                    try
                    {
                        var storageFile = await StorageFile.GetFileFromPathAsync(filePath);
                        var pdfDocument = await PdfDocument.LoadFromFileAsync(storageFile);

                        Logger.Log($"NativePdfRenderer: Loaded PDF with {pdfDocument.PageCount} pages");

                        // We will render pages one by one and add them to the UI
                        // Limit to first 10 pages for performance in "Quick Look" scenario
                        uint pageCount = Math.Min(pdfDocument.PageCount, 10);

                        for (uint i = 0; i < pageCount; i++)
                        {
                            if (cancellationToken.IsCancellationRequested) break;

                            using (var pdfPage = pdfDocument.GetPage(i))
                            {
                                 // Render to stream
                                var stream = new InMemoryRandomAccessStream();
                                await pdfPage.RenderToStreamAsync(stream);

                                // Create BitmapImage on UI thread
                                await Application.Current.Dispatcher.InvokeAsync(async () =>
                                {
                                    var image = new Image
                                    {
                                        Margin = new Thickness(0, 0, 0, 10),
                                        Stretch = System.Windows.Media.Stretch.Uniform,
                                        MaxWidth = 1000 // Reasonable max width
                                    };

                                    var bitmap = new BitmapImage();
                                    bitmap.BeginInit();
                                    bitmap.StreamSource = stream.AsStreamForRead();
                                    bitmap.CacheOption = BitmapCacheOption.OnLoad;
                                    bitmap.EndInit();
                                    bitmap.Freeze(); // Make cross-thread accessible if needed (though we are on UI here)

                                    image.Source = bitmap;
                                    stackPanel.Children.Add(image);
                                });
                            }
                        }
                        
                        if (pdfDocument.PageCount > 10)
                        {
                             await Application.Current.Dispatcher.InvokeAsync(() =>
                             {
                                 stackPanel.Children.Add(new TextBlock 
                                 { 
                                     Text = $"... and {pdfDocument.PageCount - 10} more pages",
                                     HorizontalAlignment = HorizontalAlignment.Center,
                                     Margin = new Thickness(10),
                                     Foreground = System.Windows.Media.Brushes.Gray
                                 });
                             });
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.LogError("NativePdfRenderer background error", ex);
                        await Application.Current.Dispatcher.InvokeAsync(() =>
                        {
                             stackPanel.Children.Add(new TextBlock { Text = "Error rendering PDF pages.", Foreground = System.Windows.Media.Brushes.Red });
                        });
                    }
                }, cancellationToken);

                return scrollView;
            }
            catch (Exception ex)
            {
                Logger.LogError("NativePdfRenderer error", ex);
                return new TextBlock { Text = $"Error: {ex.Message}", Foreground = System.Windows.Media.Brushes.Red };
            }
        }
    }
}
