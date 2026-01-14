#pragma once
#include <string>
#include <cstdint>

namespace Lumos {
    struct PreviewRequest {
        std::wstring path;
        std::wstring extension;
        uint64_t size;
        
        // Serialize to JSON string
        std::string ToJson() const;
        
        // Deserialize from JSON string
        static PreviewRequest FromJson(const std::string& json);
    };
}
