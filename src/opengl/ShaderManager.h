#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QHash>
#include <QString>
#include <memory>

/**
 * Manages OpenGL shader programs for LocalVTT
 * Handles loading, compilation, linking, and caching of shaders
 */
class ShaderManager
{
public:
    ShaderManager();
    ~ShaderManager();

    // Shader program management
    std::shared_ptr<QOpenGLShaderProgram> loadShaderProgram(const QString& name);
    std::shared_ptr<QOpenGLShaderProgram> getShaderProgram(const QString& name) const;
    bool hasShaderProgram(const QString& name) const;

    // Shader loading utilities
    bool loadShaderFromFile(QOpenGLShader::ShaderType type, const QString& filePath, QString& source);
    bool loadShaderFromResource(QOpenGLShader::ShaderType type, const QString& resourcePath, QString& source);

    // Error handling
    QString getLastError() const { return m_lastError; }
    void clearError() { m_lastError.clear(); }

    // Utility methods
    void clearCache();
    QStringList getLoadedShaderNames() const;

private:
    bool compileShaderProgram(const QString& name, const QString& vertexSource, const QString& fragmentSource);
    void setError(const QString& error);

    // Shader cache
    QHash<QString, std::shared_ptr<QOpenGLShaderProgram>> m_shaderPrograms;

    // Error tracking
    QString m_lastError;

    // Built-in shader sources
    static const char* s_basicVertexShader;
    static const char* s_basicFragmentShader;
    static const char* s_lightingVertexShader;
    static const char* s_lightingFragmentShader;
};

#endif // SHADERMANAGER_H