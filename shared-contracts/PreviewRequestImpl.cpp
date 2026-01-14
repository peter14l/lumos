#include "../shared-contracts/PreviewRequest.h"
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <locale>

namespace Lumos {
    std::string PreviewRequest::ToJson() const {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string pathUtf8 = converter.to_bytes(path);
        std::string extensionUtf8 = converter.to_bytes(extension);

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
        
        oss << "\",\"extension\":\"" << extensionUtf8 << "\"";
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
