#include "ShaderManager.h"
#include "utils/DebugConsole.h"
#include <QFile>
#include <QTextStream>

// Built-in basic vertex shader
const char* ShaderManager::s_basicVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_texCoord;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
)";

// Built-in basic fragment shader
const char* ShaderManager::s_basicFragmentShader = R"(
#version 330 core

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_texture;

void main()
{
    FragColor = texture(u_texture, v_texCoord);
}
)";

// Built-in lighting vertex shader
const char* ShaderManager::s_lightingVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_texCoord;
out vec2 v_worldPos;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 0.0, 1.0);
    gl_Position = u_projection * u_view * worldPos;

    v_texCoord = a_texCoord;
    v_worldPos = worldPos.xy;
}
)";

// Built-in lighting fragment shader with HDR tone mapping
const char* ShaderManager::s_lightingFragmentShader = R"(
#version 330 core

in vec2 v_texCoord;
in vec2 v_worldPos;
out vec4 FragColor;

uniform sampler2D u_texture;
uniform float u_ambientLight;
uniform int u_timeOfDay; // 0=Dawn, 1=Day, 2=Dusk, 3=Night
uniform int u_numPointLights;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform PointLight u_pointLights[8]; // Max 8 point lights

// Time of day color tints
vec3 getTimeOfDayTint(int timeOfDay) {
    switch(timeOfDay) {
        case 0: return vec3(1.0, 0.8, 0.6); // Dawn - warm orange
        case 1: return vec3(1.0, 1.0, 1.0); // Day - neutral white
        case 2: return vec3(0.9, 0.7, 0.5); // Dusk - warm amber
        case 3: return vec3(0.3, 0.4, 0.7); // Night - cool blue
        default: return vec3(1.0, 1.0, 1.0);
    }
}

// HDR tone mapping function (ACES approximation)
vec3 toneMapACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
    // Sample base texture
    vec4 baseColor = texture(u_texture, v_texCoord);

    // Start with ambient lighting
    vec3 finalColor = baseColor.rgb * u_ambientLight;

    // Add point light contributions
    for(int i = 0; i < u_numPointLights && i < 8; ++i) {
        vec3 lightPos = u_pointLights[i].position;
        vec3 lightColor = u_pointLights[i].color;
        float lightIntensity = u_pointLights[i].intensity;

        // Calculate distance and attenuation
        float distance = length(lightPos.xy - v_worldPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // Add light contribution with attenuation
        vec3 lightContribution = lightColor * lightIntensity * attenuation;
        finalColor += baseColor.rgb * lightContribution;
    }

    // Apply time of day tint
    vec3 timeOfDayTint = getTimeOfDayTint(u_timeOfDay);
    finalColor *= timeOfDayTint;

    // Apply HDR tone mapping to prevent overexposure
    finalColor = toneMapACES(finalColor);

    FragColor = vec4(finalColor, baseColor.a);
}
)";

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
    clearCache();
}

std::shared_ptr<QOpenGLShaderProgram> ShaderManager::loadShaderProgram(const QString& name)
{
    // Check if already loaded
    if (hasShaderProgram(name)) {
        return getShaderProgram(name);
    }

    QString vertexSource;
    QString fragmentSource;

    // Load built-in shaders
    if (name == "basic") {
        vertexSource = s_basicVertexShader;
        fragmentSource = s_basicFragmentShader;
    }
    else if (name == "lighting") {
        vertexSource = s_lightingVertexShader;
        fragmentSource = s_lightingFragmentShader;
    }
    else {
        // Try to load from resources or files
        QString vertexPath = QString(":/shaders/%1.vert").arg(name);
        QString fragmentPath = QString(":/shaders/%1.frag").arg(name);

        if (!loadShaderFromResource(QOpenGLShader::Vertex, vertexPath, vertexSource)) {
            setError(QString("Failed to load vertex shader for '%1'").arg(name));
            return nullptr;
        }

        if (!loadShaderFromResource(QOpenGLShader::Fragment, fragmentPath, fragmentSource)) {
            setError(QString("Failed to load fragment shader for '%1'").arg(name));
            return nullptr;
        }
    }

    // Compile shader program
    if (!compileShaderProgram(name, vertexSource, fragmentSource)) {
        return nullptr;
    }

    return getShaderProgram(name);
}

std::shared_ptr<QOpenGLShaderProgram> ShaderManager::getShaderProgram(const QString& name) const
{
    auto it = m_shaderPrograms.find(name);
    return (it != m_shaderPrograms.end()) ? it.value() : nullptr;
}

bool ShaderManager::hasShaderProgram(const QString& name) const
{
    return m_shaderPrograms.contains(name);
}

bool ShaderManager::loadShaderFromFile(QOpenGLShader::ShaderType type, const QString& filePath, QString& source)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(QString("Cannot open shader file: %1").arg(filePath));
        return false;
    }

    QTextStream stream(&file);
    source = stream.readAll();
    file.close();

    return true;
}

bool ShaderManager::loadShaderFromResource(QOpenGLShader::ShaderType type, const QString& resourcePath, QString& source)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(QString("Cannot open shader resource: %1").arg(resourcePath));
        return false;
    }

    QTextStream stream(&file);
    source = stream.readAll();
    file.close();

    return true;
}

bool ShaderManager::compileShaderProgram(const QString& name, const QString& vertexSource, const QString& fragmentSource)
{
    auto program = std::make_shared<QOpenGLShaderProgram>();

    // Compile vertex shader
    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        setError(QString("Failed to compile vertex shader for '%1': %2").arg(name, program->log()));
        return false;
    }

    // Compile fragment shader
    if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        setError(QString("Failed to compile fragment shader for '%1': %2").arg(name, program->log()));
        return false;
    }

    // Link program
    if (!program->link()) {
        setError(QString("Failed to link shader program '%1': %2").arg(name, program->log()));
        return false;
    }

    // Store in cache
    m_shaderPrograms[name] = program;
    DebugConsole::system(QString("Shader program '%1' compiled and linked successfully").arg(name), "OpenGL");

    return true;
}

void ShaderManager::setError(const QString& error)
{
    m_lastError = error;
    DebugConsole::error(QString("ShaderManager error: %1").arg(error), "OpenGL");
}

void ShaderManager::clearCache()
{
    // Clear all shader programs (shared_ptr handles deletion automatically)
    m_shaderPrograms.clear();
}

QStringList ShaderManager::getLoadedShaderNames() const
{
    return m_shaderPrograms.keys();
}