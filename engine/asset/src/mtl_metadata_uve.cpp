// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/mtl_metadata_uve.h"
#include <charconv>
#include <cctype>
#include <cmath>
#include <new>
#include <system_error>
#include <utility>
namespace UVE::Asset { namespace {
constexpr std::uint32_t kMax = 1'000'000U;
bool Inc(std::uint32_t& value) noexcept { if (value >= kMax) return false; ++value; return true; }
std::string_view Token(std::string_view& rest) noexcept { while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front())) != 0) rest.remove_prefix(1U); const auto end = rest.find_first_of(" \t\r\n"); const auto token = rest.substr(0U, end); rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end); return token; }
bool Has(std::string_view& rest, const std::size_t count) noexcept { for (std::size_t i=0U;i<count;++i) if (Token(rest).empty()) return false; return true; }
}

bool ValidateMtlTextureReferenceUVE(const std::string_view path) noexcept {
    if (path.empty() || path.size() > kMaximumMtlTextureReferenceBytesUVE ||
        path.front() == '/' || path.front() == '\\' || path.find('\0') != std::string_view::npos ||
        (path.size() >= 2U && path[1U] == ':')) {
        return false;
    }
    std::size_t segmentStart = 0U;
    while (segmentStart <= path.size()) {
        const std::size_t separator = path.find_first_of("/\\", segmentStart);
        const std::string_view segment = path.substr(
            segmentStart, separator == std::string_view::npos ? std::string_view::npos : separator - segmentStart);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        segmentStart = separator + 1U;
    }
    return true;
}

bool ParseMtlMaterialPropertyUVE(const std::string_view sourceLine, MtlMaterialPropertyUVE& outProperty) {
    if (sourceLine.empty() || sourceLine.size() > kMaximumMtlTextureReferenceBytesUVE) {
        return false;
    }
    const auto parseFloat = [](const std::string_view text, float& outValue) noexcept {
        if (text.empty()) return false;
        float value = 0.0F;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value,
                                            std::chars_format::general);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !std::isfinite(value)) {
            return false;
        }
        outValue = value;
        return true;
    };
    try {
        std::string_view rest = sourceLine;
        const std::string_view directive = Token(rest);
        MtlMaterialPropertyUVE candidate;
        if (directive == "Kd" || directive == "Ka" || directive == "Ks" || directive == "Ke" || directive == "Tf") {
            candidate.kind = MtlMaterialPropertyKindUVE::Vector3;
            for (std::size_t index = 0U; index < candidate.vectorValue.size(); ++index) {
                if (!parseFloat(Token(rest), candidate.vectorValue[index])) return false;
            }
            if (!Token(rest).empty()) return false;
        } else if (directive == "Ns" || directive == "Ni" || directive == "d" || directive == "Tr" || directive == "illum") {
            candidate.kind = MtlMaterialPropertyKindUVE::Scalar;
            if (!parseFloat(Token(rest), candidate.scalarValue) || !Token(rest).empty()) return false;
        } else if (directive.rfind("map_", 0U) == 0U) {
            const std::string_view reference = Token(rest);
            if (!ValidateMtlTextureReferenceUVE(reference) || !Token(rest).empty()) return false;
            candidate.kind = MtlMaterialPropertyKindUVE::TextureReference;
            candidate.textureReference.assign(reference);
        } else {
            return false;
        }
        outProperty = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

std::optional<MtlMetadataUVE> ParseMtlMetadataUVE(const std::string_view source) {
    MtlMetadataUVE out; std::size_t start=0U;
    while (start <= source.size()) {
        const auto end=source.find('\n', start); auto line=source.substr(start, end==std::string_view::npos?std::string_view::npos:end-start); if(!line.empty()&&line.back()=='\r') line.remove_suffix(1U);
        while(!line.empty()&&(line.front()==' '||line.front()=='\t')) line.remove_prefix(1U);
        if(!line.empty()&&line.front()!='#') { auto rest=line; const auto directive=Token(rest);
            if(directive=="newmtl") { if(!Has(rest,1U)||!Inc(out.materialCount)) return std::nullopt; }
            else if(directive.rfind("map_",0U)==0U) { if(!Has(rest,1U)||!Inc(out.textureMapCount)) return std::nullopt; }
            else if(directive=="Kd"||directive=="Ka"||directive=="Ks"||directive=="Ke"||directive=="Tf") { if(!Has(rest,3U)||!Inc(out.vectorPropertyCount)) return std::nullopt; }
            else if(directive=="Ns"||directive=="Ni"||directive=="d"||directive=="Tr"||directive=="illum") { if(!Has(rest,1U)||!Inc(out.scalarPropertyCount)) return std::nullopt; }
            else if(!Inc(out.ignoredStatementCount)) return std::nullopt;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }
    return out;
}
} // namespace UVE::Asset
