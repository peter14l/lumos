using System;

namespace Lumos.Contracts
{
    public class PreviewRequest
    {
        public string Path { get; set; }
        public string Extension { get; set; }
        public long Size { get; set; }
    }
}
