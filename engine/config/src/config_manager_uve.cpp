// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/config/config_manager_uve.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <mutex>
#include <utility>

#include <nlohmann/json.hpp>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Config {

namespace {

/// Splits a dot-separated key path ("editor.theme") into its ordered
/// segments ({"editor", "theme"}). An empty `keyPath` yields a single
/// empty-string segment.
std::vector<std::string> SplitKeyPathUVE(std::string_view keyPath) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (true) {
        const std::size_t dot = keyPath.find('.', start);
        if (dot == std::string_view::npos) {
            segments.emplace_back(keyPath.substr(start));
            break;
        }
        segments.emplace_back(keyPath.substr(start, dot - start));
        start = dot + 1;
    }
    return segments;
}

/// Walks `segments` through `document`, returning a pointer to the node
/// reached, or nullptr if any segment along the way is missing or the
/// current node is not an object to walk into.
const nlohmann::json* ResolveConstUVE(const nlohmann::json& document,
                                       const std::vector<std::string>& segments) {
    const nlohmann::json* current = &document;
    for (const std::string& segment : segments) {
        if (!current->is_object()) {
            return nullptr;
        }
        const auto memberIt = current->find(segment);
        if (memberIt == current->end()) {
            return nullptr;
        }
        current = &(*memberIt);
    }
    return current;
}

/// Walks `segments` through `document`, creating intermediate objects
/// along the way as needed (overwriting any non-object node found in an
/// intermediate position), and returns a reference to the final node.
nlohmann::json& ResolveOrCreateUVE(nlohmann::json& document, const std::vector<std::string>& segments) {
    nlohmann::json* current = &document;
    for (const std::string& segment : segments) {
        if (!current->is_object()) {
            *current = nlohmann::json::object();
        }
        current = &(*current)[segment];
    }
    return *current;
}

/// Pretty-prints `document` to `path`. Does not create missing parent
/// directories. On failure, logs a detailed Error containing both the
/// target path and the failure reason.
bool SaveDocumentToFileUVE(const nlohmann::json& document, const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        UVE_ERROR("ConfigManagerUVE: failed to open \"{}\" for writing: {}", path.string(),
                   std::strerror(errno));
        return false;
    }

    file << document.dump(4);
    if (!file.good()) {
        UVE_ERROR("ConfigManagerUVE: failed to write settings to \"{}\": stream error after write",
                   path.string());
        return false;
    }
    return true;
}

} // namespace

/// The hidden implementation behind ConfigManagerUVE's PIMPL: the actual
/// nlohmann::json document, the path it was last loaded/saved with, and
/// the mutex guarding both. Defined only in this translation unit, so
/// nlohmann::json never appears in any header this module exposes.
struct ConfigManagerUVE::ImplUVE {
    mutable std::mutex mutex;
    nlohmann::json document = nlohmann::json::object();
    std::filesystem::path loadedPath;
};

ConfigManagerUVE::ConfigManagerUVE() : m_impl(std::make_unique<ImplUVE>()) {}

ConfigManagerUVE::~ConfigManagerUVE() = default;

bool ConfigManagerUVE::LoadUVE(const std::filesystem::path& path) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->loadedPath = path;

    std::ifstream file(path);
    if (!file.is_open()) {
        UVE_WARNING("ConfigManagerUVE: settings file not found at \"{}\" - starting with an "
                    "empty configuration",
                    path.string());
        m_impl->document = nlohmann::json::object();
        return false;
    }

    try {
        nlohmann::json parsed;
        file >> parsed;
        m_impl->document = std::move(parsed);
        return true;
    } catch (const nlohmann::json::parse_error& parseError) {
        UVE_ERROR("ConfigManagerUVE: failed to parse settings file \"{}\": {}", path.string(),
                   parseError.what());
        return false;
    }
}

bool ConfigManagerUVE::SaveUVE() {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->loadedPath.empty()) {
        UVE_ERROR("ConfigManagerUVE: SaveUVE() called with no known target path - call LoadUVE() "
                   "or SaveUVE(path) first");
        return false;
    }
    return SaveDocumentToFileUVE(m_impl->document, m_impl->loadedPath);
}

bool ConfigManagerUVE::SaveUVE(const std::filesystem::path& path) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->loadedPath = path;
    return SaveDocumentToFileUVE(m_impl->document, path);
}

std::string ConfigManagerUVE::GetStringUVE(std::string_view keyPath, std::string_view defaultValue) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const nlohmann::json* const value = ResolveConstUVE(m_impl->document, SplitKeyPathUVE(keyPath));
    if (value == nullptr || !value->is_string()) {
        return std::string(defaultValue);
    }
    return value->get<std::string>();
}

std::int64_t ConfigManagerUVE::GetIntUVE(std::string_view keyPath, std::int64_t defaultValue) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const nlohmann::json* const value = ResolveConstUVE(m_impl->document, SplitKeyPathUVE(keyPath));
    if (value == nullptr || !value->is_number_integer()) {
        return defaultValue;
    }
    return value->get<std::int64_t>();
}

double ConfigManagerUVE::GetDoubleUVE(std::string_view keyPath, double defaultValue) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const nlohmann::json* const value = ResolveConstUVE(m_impl->document, SplitKeyPathUVE(keyPath));
    if (value == nullptr || !value->is_number()) {
        return defaultValue;
    }
    return value->get<double>();
}

bool ConfigManagerUVE::GetBoolUVE(std::string_view keyPath, bool defaultValue) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const nlohmann::json* const value = ResolveConstUVE(m_impl->document, SplitKeyPathUVE(keyPath));
    if (value == nullptr || !value->is_boolean()) {
        return defaultValue;
    }
    return value->get<bool>();
}

void ConfigManagerUVE::SetStringUVE(std::string_view keyPath, std::string value) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    ResolveOrCreateUVE(m_impl->document, SplitKeyPathUVE(keyPath)) = std::move(value);
}

void ConfigManagerUVE::SetIntUVE(std::string_view keyPath, std::int64_t value) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    ResolveOrCreateUVE(m_impl->document, SplitKeyPathUVE(keyPath)) = value;
}

void ConfigManagerUVE::SetDoubleUVE(std::string_view keyPath, double value) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    ResolveOrCreateUVE(m_impl->document, SplitKeyPathUVE(keyPath)) = value;
}

void ConfigManagerUVE::SetBoolUVE(std::string_view keyPath, bool value) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    ResolveOrCreateUVE(m_impl->document, SplitKeyPathUVE(keyPath)) = value;
}

bool ConfigManagerUVE::HasKeyUVE(std::string_view keyPath) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const nlohmann::json* const value = ResolveConstUVE(m_impl->document, SplitKeyPathUVE(keyPath));
    return value != nullptr && !value->is_object();
}

} // namespace UVE::Config
