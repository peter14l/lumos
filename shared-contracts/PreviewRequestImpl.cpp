#include "../shared-contracts/PreviewRequest.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

namespace Lumos {
    std::string PreviewRequest::ToJson() const {
        std::ostringstream oss;
        oss << "{";
        oss << "\"path\":\"";
        
        // Escape backslashes and quotes in path
        for (wchar_t c : path) {
            if (c == L'\\') {
                oss << "\\\\";
            } else if (c == L'"') {
                oss << "\\\"";
            } else if (c < 128) {
                oss << static_cast<char>(c);
            }
        }
        
        // Convert extension to UTF-8 using Windows API
        int extensionSize = WideCharToMultiByte(CP_UTF8, 0, extension.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (extensionSize > 0) {
            std::string extensionUtf8(extensionSize - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, extension.c_str(), -1, &extensionUtf8[0], extensionSize, nullptr, nullptr);
            oss << "\",\"extension\":\"" << extensionUtf8 << "\"";
        } else {
            oss << "\",\"extension\":\"\"";
        }
        
        oss << ",\"size\":" << size;
        oss << "}";

        return oss.str();
    }

    PreviewRequest PreviewRequest::FromJson(const std::string& json) {
        // Simple JSON parsing (in production, use a proper JSON library)
        PreviewRequest request;
        
        // This is a simplified implementation
        // In a real application, use nlohmann/json or similar
        
        return request;
    }
}
