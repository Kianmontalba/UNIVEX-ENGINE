#include "univex/render/ShaderProgram.h"

#include <utility>
#include <vector>

namespace univex::render {

namespace {

GLuint CompileStage(GLenum stage, std::string_view source, std::string& outError) {
    const GLuint shader = glCreateShader(stage);
    const auto* data = source.data();
    const auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &data, &length);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(static_cast<std::size_t>(logLength > 1 ? logLength : 1), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());

    outError = (stage == GL_VERTEX_SHADER ? "vertex shader: " : "fragment shader: ");
    outError += log.data();
    glDeleteShader(shader);
    return 0;
}

} // namespace

ShaderProgram::~ShaderProgram() { Destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : program_(std::exchange(other.program_, 0)),
      uniformCache_(std::move(other.uniformCache_)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        Destroy();
        program_ = std::exchange(other.program_, 0);
        uniformCache_ = std::move(other.uniformCache_);
    }
    return *this;
}

void ShaderProgram::Destroy() noexcept {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    uniformCache_.clear();
}

std::optional<ShaderProgram> ShaderProgram::Build(std::string_view vertexSource,
                                                  std::string_view fragmentSource,
                                                  std::string& outError) {
    outError.clear();

    const GLuint vertex = CompileStage(GL_VERTEX_SHADER, vertexSource, outError);
    if (vertex == 0) return std::nullopt;

    const GLuint fragment = CompileStage(GL_FRAGMENT_SHADER, fragmentSource, outError);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return std::nullopt;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    // The shader objects are reference-counted by the program once linked.
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(logLength > 1 ? logLength : 1), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        outError = std::string("link: ") + log.data();
        glDeleteProgram(program);
        return std::nullopt;
    }

    return ShaderProgram(program);
}

GLint ShaderProgram::UniformLocation(const std::string& name) const {
    if (const auto it = uniformCache_.find(name); it != uniformCache_.end()) return it->second;
    const GLint location = glGetUniformLocation(program_, name.c_str());
    uniformCache_.emplace(name, location);
    return location;
}

void ShaderProgram::SetFloat(const std::string& name, float value) const {
    glUniform1f(UniformLocation(name), value);
}

void ShaderProgram::SetVec2(const std::string& name, float x, float y) const {
    glUniform2f(UniformLocation(name), x, y);
}

void ShaderProgram::SetVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(UniformLocation(name), x, y, z);
}

void ShaderProgram::SetVec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(UniformLocation(name), x, y, z, w);
}

void ShaderProgram::SetMat4(const std::string& name, const float* columnMajor16) const {
    glUniformMatrix4fv(UniformLocation(name), 1, GL_FALSE, columnMajor16);
}

} // namespace univex::render
