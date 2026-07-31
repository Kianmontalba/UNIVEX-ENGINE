//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace UVE::Config {

/// IConfigManagerUVE is the engine's JSON-backed key-value settings store
/// interface (the `.uvesettings` file — engine/editor settings such as
/// keybinds, theme, layout, and platform preferences). Keys are addressed
/// by dot-separated path (e.g. "editor.theme") into a nested JSON
/// document; the JSON library itself never appears in this interface or
/// any implementation's public header (see ConfigManagerUVE). Only scalar
/// value types are supported — string, 64-bit integer, double, and bool.
/// The in-memory document reserves a root-level "version" integer field as
/// a forward-compatibility convention for future `.uvesettings` format
/// changes; this increment implements no migration logic against it.
/// Thread-safety: implementations must be safe to call from any thread
/// concurrently, guarded by an internal mutex.
class IConfigManagerUVE {
public:
    virtual ~IConfigManagerUVE() = default;

    /// Loads and parses the JSON document at `path`, replacing the
    /// in-memory document on success and remembering `path` as the default
    /// target for SaveUVE(). If `path` does not exist, logs a Warning and
    /// leaves the in-memory document as an empty object (not fatal — a
    /// first-run engine has no settings file yet). If `path` exists but
    /// fails to parse as JSON, logs an Error (including the parser's
    /// message) and leaves the previous in-memory document untouched.
    /// Returns true only on a fully successful load.
    virtual bool LoadUVE(const std::filesystem::path& path) = 0;

    /// Pretty-prints the in-memory document to the path most recently
    /// passed to LoadUVE() or SaveUVE(). Returns false (and logs an Error
    /// containing the target path and failure reason) if no path is known
    /// yet, or if the file cannot be opened for writing — this does not
    /// automatically create missing parent directories. Never fatal.
    virtual bool SaveUVE() = 0;

    /// Pretty-prints the in-memory document to `path`, remembering it as
    /// the new default target for future no-argument SaveUVE() calls.
    /// Same failure behavior as SaveUVE() above.
    virtual bool SaveUVE(const std::filesystem::path& path) = 0;

    /// Returns the string value at dot-path `keyPath`, or `defaultValue`
    /// if the path does not exist or does not resolve to a string.
    [[nodiscard]] virtual std::string GetStringUVE(std::string_view keyPath,
                                                     std::string_view defaultValue) const = 0;
    /// Returns the 64-bit integer value at dot-path `keyPath`, or
    /// `defaultValue` if the path does not exist or is not an integer.
    [[nodiscard]] virtual std::int64_t GetIntUVE(std::string_view keyPath,
                                                  std::int64_t defaultValue) const = 0;
    /// Returns the double value at dot-path `keyPath`, or `defaultValue`
    /// if the path does not exist or is not a number.
    [[nodiscard]] virtual double GetDoubleUVE(std::string_view keyPath, double defaultValue) const = 0;
    /// Returns the bool value at dot-path `keyPath`, or `defaultValue` if
    /// the path does not exist or is not a bool.
    [[nodiscard]] virtual bool GetBoolUVE(std::string_view keyPath, bool defaultValue) const = 0;

    /// Sets the string value at dot-path `keyPath`, creating any missing
    /// intermediate objects along the path.
    virtual void SetStringUVE(std::string_view keyPath, std::string value) = 0;
    /// Sets the 64-bit integer value at dot-path `keyPath`, creating any
    /// missing intermediate objects along the path.
    virtual void SetIntUVE(std::string_view keyPath, std::int64_t value) = 0;
    /// Sets the double value at dot-path `keyPath`, creating any missing
    /// intermediate objects along the path.
    virtual void SetDoubleUVE(std::string_view keyPath, double value) = 0;
    /// Sets the bool value at dot-path `keyPath`, creating any missing
    /// intermediate objects along the path.
    virtual void SetBoolUVE(std::string_view keyPath, bool value) = 0;

    /// True iff `keyPath` exists and resolves to a leaf value (not merely
    /// an intermediate object).
    [[nodiscard]] virtual bool HasKeyUVE(std::string_view keyPath) const = 0;
};

} // namespace UVE::Config
