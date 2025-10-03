#ifndef OPENGLMAPDISPLAY_H
#define OPENGLMAPDISPLAY_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QImage>
#include <memory>

class ShaderManager;
class PostProcessingSystem;
class QTimer;

/**
 * OpenGL-based map display widget for high-performance rendering
 * Designed to replace QPainter-based rendering for improved lighting performance
 */
class OpenGLMapDisplay : public QOpenGLWidget, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit OpenGLMapDisplay(QWidget *parent = nullptr);
    ~OpenGLMapDisplay();

    // Core functionality
    bool loadTexture(const QImage& image);
    void setProjectionMatrix(const QMatrix4x4& projection);
    void setViewMatrix(const QMatrix4x4& view);
    void setModelMatrix(const QMatrix4x4& model);

    // Lighting control
    void setLightingEnabled(bool enabled);
    void setAmbientLightLevel(float level);
    void setTimeOfDay(int timeOfDay); // 0=Dawn, 1=Day, 2=Dusk, 3=Night

    // HDR control
    void setHDREnabled(bool enabled);
    void setExposure(float exposure);
    bool isHDREnabled() const { return m_hdrEnabled; }
    float getExposure() const { return m_exposure; }

    // Post-processing effects control
    void setBloomEnabled(bool enabled);
    void setBloomThreshold(float threshold);
    void setBloomIntensity(float intensity);
    void setBloomRadius(float radius);
    void setShadowMappingEnabled(bool enabled);
    void setShadowMapSize(int size);
    void setVolumetricLightingEnabled(bool enabled);
    void setVolumetricIntensity(float intensity);
    void setLightShaftsEnabled(bool enabled);
    void setLightShaftsIntensity(float intensity);
    void setMSAAEnabled(bool enabled);
    void setMSAASamples(int samples);

    // Post-processing getters
    bool isBloomEnabled() const;
    bool isShadowMappingEnabled() const;
    bool isVolumetricLightingEnabled() const;
    bool isLightShaftsEnabled() const;
    bool isMSAAEnabled() const;

    // Post-processing parameter getters
    float getBloomThreshold() const;
    float getBloomIntensity() const;
    float getBloomRadius() const;
    int getShadowMapSize() const;
    float getVolumetricIntensity() const;
    float getLightShaftsIntensity() const;
    int getMSAASamples() const;

    // Utility methods
    QSize getTextureSize() const;
    bool isInitialized() const { return m_initialized; }
    bool hasValidTexture() const { return m_mapTexture && m_mapTexture->textureId() != 0; }

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    void setupVertexBuffer();
    void setupShaders();
    void updateUniforms();
    void renderQuad();

    // OpenGL objects (using smart pointers for better memory management)
    std::unique_ptr<QOpenGLTexture> m_mapTexture;
    std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
    std::unique_ptr<QOpenGLBuffer> m_vertexBuffer;
    std::unique_ptr<QOpenGLBuffer> m_indexBuffer;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<PostProcessingSystem> m_postProcessingSystem;

    // Shader programs (using shared_ptr to prevent double-delete with ShaderManager)
    std::shared_ptr<QOpenGLShaderProgram> m_basicShader;
    std::shared_ptr<QOpenGLShaderProgram> m_lightingShader;

    // Transform matrices
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;

    // Lighting state
    bool m_lightingEnabled;
    float m_ambientLightLevel;
    int m_timeOfDay;

    // HDR state
    bool m_hdrEnabled;
    float m_exposure;

    // State flags
    bool m_initialized;
    QSize m_textureSize;
    // Pending image to upload once GL is initialized (avoids context mismatch)
    QImage m_pendingTextureImage;

    // Vertex data for fullscreen quad
    static const float s_quadVertices[];
    static const unsigned int s_quadIndices[];
};

#endif // OPENGLMAPDISPLAY_H
