using System;

namespace Lumos.Contracts
{
    public class PreviewRequest
    {
        public required string Path { get; set; }
        public required string Extension { get; set; }
        public long Size { get; set; }
    }
}
