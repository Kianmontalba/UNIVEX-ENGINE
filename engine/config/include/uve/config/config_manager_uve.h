// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/config/i_config_manager_uve.h"

namespace UVE::Config {

/// ConfigManagerUVE is the concrete, engine-standard implementation of
/// IConfigManagerUVE, backed by nlohmann::json. The JSON library is
/// entirely confined to config_manager_uve.cpp via the private ImplUVE
/// PIMPL — this header, and every consumer of ConfigManagerUVE, never sees
/// the nlohmann::json type (see docs/CODING_STANDARDS.md's dependency
/// confinement note).
/// Thread-safety: thread-safe. Every method is guarded by an internal
/// mutex (`mutable std::mutex m_mutex;` in ImplUVE) — safe to call
/// concurrently from any thread.
class ConfigManagerUVE final : public IConfigManagerUVE {
public:
    ConfigManagerUVE();
    ~ConfigManagerUVE() override;

    ConfigManagerUVE(const ConfigManagerUVE&) = delete;
    ConfigManagerUVE& operator=(const ConfigManagerUVE&) = delete;

    bool LoadUVE(const std::filesystem::path& path) override;
    bool SaveUVE() override;
    bool SaveUVE(const std::filesystem::path& path) override;

    [[nodiscard]] std::string GetStringUVE(std::string_view keyPath,
                                            std::string_view defaultValue) const override;
    [[nodiscard]] std::int64_t GetIntUVE(std::string_view keyPath,
                                          std::int64_t defaultValue) const override;
    [[nodiscard]] double GetDoubleUVE(std::string_view keyPath, double defaultValue) const override;
    [[nodiscard]] bool GetBoolUVE(std::string_view keyPath, bool defaultValue) const override;

    void SetStringUVE(std::string_view keyPath, std::string value) override;
    void SetIntUVE(std::string_view keyPath, std::int64_t value) override;
    void SetDoubleUVE(std::string_view keyPath, double value) override;
    void SetBoolUVE(std::string_view keyPath, bool value) override;

    [[nodiscard]] bool HasKeyUVE(std::string_view keyPath) const override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Config
