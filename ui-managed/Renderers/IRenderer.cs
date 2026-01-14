using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace Lumos.UI.Renderers
{
    public interface IRenderer
    {
        Task<UIElement> RenderAsync(string filePath, CancellationToken cancellationToken);
        bool CanHandle(string extension);
    }
}
