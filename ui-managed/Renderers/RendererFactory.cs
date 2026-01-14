using System;
using System.Collections.Generic;
using System.Linq;

namespace Lumos.UI.Renderers
{
    public class RendererFactory
    {
        private readonly List<IRenderer> _renderers;

        public RendererFactory()
        {
            _renderers = new List<IRenderer>
            {
                new ImageRenderer(),
                new TextRenderer(),
                new PDFRenderer(),
                new AudioRenderer(),
                new VideoRenderer()
            };
        }

        public IRenderer? GetRenderer(string extension)
        {
            extension = extension.ToLowerInvariant();
            return _renderers.FirstOrDefault(r => r.CanHandle(extension));
        }
    }
}
