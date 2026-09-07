// univex/render/ShaderProgram.h
// -----------------------------------------------------------------------
// RAII wrapper around a compiled+linked GL program. Move-only: the GL
// object has exactly one owner, and copying a handle that a destructor
// will later glDeleteProgram() is a use-after-free waiting to happen.
//
// Compile and link failures are returned, not thrown and not swallowed —
// the driver's actual info log comes back in the error string, because a
// shader that silently fails to compile is the single most annoying way
// to lose an afternoon.
// -----------------------------------------------------------------------
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "univex/render/GlApi.h"

namespace univex::render {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    // Returns nullopt on failure and fills `outError` with the driver's
    // info log (prefixed with which stage failed).
    [[nodiscard]] static std::optional<ShaderProgram> Build(std::string_view vertexSource,
                                                            std::string_view fragmentSource,
                                                            std::string& outError);

    [[nodiscard]] bool Valid() const { return program_ != 0; }
    [[nodiscard]] GLuint Handle() const { return program_; }

    void Use() const { glUseProgram(program_); }

    // Uniform locations are looked up once and cached; an unknown name
    // resolves to -1, which every glUniform* call ignores, so a renamed
    // uniform degrades to "that value stops updating" rather than a crash.
    [[nodiscard]] GLint UniformLocation(const std::string& name) const;

    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, float x, float y) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;
    void SetVec4(const std::string& name, float x, float y, float z, float w) const;
    void SetMat4(const std::string& name, const float* columnMajor16) const;

private:
    explicit ShaderProgram(GLuint program) : program_(program) {}
    void Destroy() noexcept;

    GLuint program_ = 0;
    mutable std::unordered_map<std::string, GLint> uniformCache_;
};

} // namespace univex::render
