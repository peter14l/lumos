using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Animation;
using Lumos.Contracts;
using Lumos.UI.Renderers;

namespace Lumos.UI
{
    public partial class PreviewWindow : Window
    {
        private CancellationTokenSource? _renderCancellation;
        private readonly RendererFactory _rendererFactory;

        public PreviewWindow()
        {
            InitializeComponent();
            _rendererFactory = new RendererFactory();
            Opacity = 0;
        }

        public async Task ShowPreview(PreviewRequest request)
        {
            // Cancel any ongoing render
            _renderCancellation?.Cancel();
            _renderCancellation = new CancellationTokenSource();

            // Show loading state
            LoadingText.Visibility = Visibility.Visible;
            ErrorText.Visibility = Visibility.Collapsed;
            ContentPresenter.Content = null;

            try
            {
                // Get appropriate renderer
                var renderer = _rendererFactory.GetRenderer(request.Extension);
                if (renderer == null)
                {
                    ShowError($"Unsupported file type: {request.Extension}");
                    return;
                }

                // Render content
                var content = await renderer.RenderAsync(request.Path, _renderCancellation.Token);
                
                if (!_renderCancellation.Token.IsCancellationRequested)
                {
                    LoadingText.Visibility = Visibility.Collapsed;
                    ContentPresenter.Content = content;

                    // Position window near cursor
                    PositionWindowNearCursor();

                    // Show window with fade-in animation
                    Show();
                    var fadeIn = (Storyboard)Resources["FadeInAnimation"];
                    fadeIn.Begin(this);
                }
            }
            catch (Exception ex)
            {
                ShowError($"Error loading preview: {ex.Message}");
            }
        }

        private void ShowError(string message)
        {
            LoadingText.Visibility = Visibility.Collapsed;
            ErrorText.Text = message;
            ErrorText.Visibility = Visibility.Visible;
            
            PositionWindowNearCursor();
            Show();
            var fadeIn = (Storyboard)Resources["FadeInAnimation"];
            fadeIn.Begin(this);
        }

        private void PositionWindowNearCursor()
        {
            // Get cursor position
            var cursorPos = System.Windows.Forms.Cursor.Position;

            // Get screen dimensions
            var screen = System.Windows.Forms.Screen.FromPoint(cursorPos);
            var screenWidth = screen.WorkingArea.Width;
            var screenHeight = screen.WorkingArea.Height;

            // Calculate max window size (80% of screen)
            var maxWidth = screenWidth * 0.8;
            var maxHeight = screenHeight * 0.8;

            // Update layout to get actual size
            UpdateLayout();

            // Center near cursor, but ensure it's fully visible
            var left = cursorPos.X - (ActualWidth / 2);
            var top = cursorPos.Y - (ActualHeight / 2);

            // Constrain to screen bounds
            left = Math.Max(screen.WorkingArea.Left, Math.Min(left, screen.WorkingArea.Right - ActualWidth));
            top = Math.Max(screen.WorkingArea.Top, Math.Min(top, screen.WorkingArea.Bottom - ActualHeight));

            Left = left;
            Top = top;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // Hide initially
            Hide();
        }

        private void Window_Deactivated(object sender, EventArgs e)
        {
            // Close when focus is lost
            ClosePreview();
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            // Close on Esc or Space
            if (e.Key == Key.Escape || e.Key == Key.Space)
            {
                ClosePreview();
            }
        }

        private void Window_MouseDown(object sender, MouseButtonEventArgs e)
        {
            // Close on click
            ClosePreview();
        }

        private void ClosePreview()
        {
            _renderCancellation?.Cancel();
            Hide();
            ContentPresenter.Content = null;
        }
    }
}
