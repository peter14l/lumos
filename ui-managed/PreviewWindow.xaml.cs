using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Animation;
using Lumos.Contracts;
using Lumos.UI.Renderers;
using Lumos.UI.Services;

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
                    Logger.Log($"Unsupported file type: {request.Extension}");
                    ShowError($"Unsupported file type: {request.Extension}");
                    return;
                }

                Logger.Log($"Using renderer: {renderer.GetType().Name}");

                // Render content
                var content = await renderer.RenderAsync(request.Path, _renderCancellation.Token);
                
                if (!_renderCancellation.Token.IsCancellationRequested)
                {
                    LoadingText.Visibility = Visibility.Collapsed;
                    ContentPresenter.Content = content;

                    Logger.Log("Content rendered, positioning window...");
                    // Position window centered on screen
                    PositionWindowCentered();

                    // Show window with fade-in animation
                    Show();
                    Activate(); // Ensure window is active
                    var fadeIn = (Storyboard)Resources["FadeInAnimation"];
                    fadeIn.Begin(this);
                    Logger.Log("Window shown and animation started");
                }
            }
            catch (Exception ex)
            {
                Logger.LogError("Error loading preview", ex);
                ShowError($"Error loading preview: {ex.Message}");
            }
        }

        private void ShowError(string message)
        {
            LoadingText.Visibility = Visibility.Collapsed;
            ErrorText.Text = message;
            ErrorText.Visibility = Visibility.Visible;
            
            PositionWindowCentered();
            Show();
            var fadeIn = (Storyboard)Resources["FadeInAnimation"];
            fadeIn.Begin(this);
        }

        private void PositionWindowCentered()
        {
            // Get primary screen dimensions
            var screenWidth = SystemParameters.PrimaryScreenWidth;
            var screenHeight = SystemParameters.PrimaryScreenHeight;

            // Update layout to get actual size
            UpdateLayout();
            Logger.Log($"Window size: {ActualWidth}x{ActualHeight}");

            // Center window
            var left = (screenWidth - ActualWidth) / 2;
            var top = (screenHeight - ActualHeight) / 2;

            Logger.Log($"Final window position: {left}, {top}");

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
