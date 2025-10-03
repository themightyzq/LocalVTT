#ifndef POSTPROCESSINGSYSTEM_H
#define POSTPROCESSINGSYSTEM_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QMatrix4x4>
#include <QSize>
#include <memory>

class ShaderManager;

/**
 * Post-processing system for advanced visual effects
 * Handles bloom, shadow mapping, volumetric lighting, light shafts, and MSAA
 */
class PostProcessingSystem : protected QOpenGLFunctions_3_3_Core
{
public:
    PostProcessingSystem();
    ~PostProcessingSystem();

    // System management
    void initialize(ShaderManager* shaderManager, const QSize& size);
    void cleanup();
    void resize(const QSize& size);
    bool isInitialized() const { return m_initialized; }

    // Bloom controls
    void setBloomEnabled(bool enabled);
    bool isBloomEnabled() const { return m_bloomEnabled; }
    void setBloomThreshold(float threshold);
    float getBloomThreshold() const { return m_bloomThreshold; }
    void setBloomIntensity(float intensity);
    float getBloomIntensity() const { return m_bloomIntensity; }
    void setBloomRadius(float radius);
    float getBloomRadius() const { return m_bloomRadius; }

    // Shadow mapping controls
    void setShadowMappingEnabled(bool enabled);
    bool isShadowMappingEnabled() const { return m_shadowMappingEnabled; }
    void setShadowMapSize(int size);
    int getShadowMapSize() const { return m_shadowMapSize; }

    // Volumetric lighting controls
    void setVolumetricLightingEnabled(bool enabled);
    bool isVolumetricLightingEnabled() const { return m_volumetricLightingEnabled; }
    void setVolumetricIntensity(float intensity);
    float getVolumetricIntensity() const { return m_volumetricIntensity; }

    // Light shafts controls
    void setLightShaftsEnabled(bool enabled);
    bool isLightShaftsEnabled() const { return m_lightShaftsEnabled; }
    void setLightShaftsIntensity(float intensity);
    float getLightShaftsIntensity() const { return m_lightShaftsIntensity; }

    // MSAA controls
    void setMSAAEnabled(bool enabled);
    bool isMSAAEnabled() const { return m_msaaEnabled; }
    void setMSAASamples(int samples);
    int getMSAASamples() const { return m_msaaSamples; }

    // Rendering pipeline
    void beginFrame();
    void endFrame();
    void renderToScreen();

    // Framebuffer management
    QOpenGLFramebufferObject* getSceneFramebuffer() const { return m_sceneFramebuffer.get(); }
    QOpenGLFramebufferObject* getBloomFramebuffer() const { return m_bloomFramebuffer.get(); }

private:
    // Initialization
    void setupFramebuffers();
    void setupShaders();
    void setupQuadGeometry();

    // Bloom implementation
    void renderBloom();
    void extractBrightPixels();
    void blurBrightPixels();
    void combineBloom();

    // Shadow mapping implementation
    void renderShadowMap();
    void setupShadowMapFramebuffer();

    // Volumetric lighting implementation
    void renderVolumetricLighting();

    // Light shafts implementation
    void renderLightShafts();

    // MSAA implementation
    void resolveMSAA();

    // Utility methods
    void renderFullscreenQuad();
    void bindFramebuffer(QOpenGLFramebufferObject* fbo);

    // OpenGL resources
    ShaderManager* m_shaderManager;

    // Framebuffers
    std::unique_ptr<QOpenGLFramebufferObject> m_sceneFramebuffer;
    std::unique_ptr<QOpenGLFramebufferObject> m_bloomFramebuffer;
    std::unique_ptr<QOpenGLFramebufferObject> m_brightPixelsFramebuffer;
    std::unique_ptr<QOpenGLFramebufferObject> m_blurFramebuffer1;
    std::unique_ptr<QOpenGLFramebufferObject> m_blurFramebuffer2;
    std::unique_ptr<QOpenGLFramebufferObject> m_shadowMapFramebuffer;
    std::unique_ptr<QOpenGLFramebufferObject> m_msaaFramebuffer;

    // Shader programs (using shared_ptr to prevent double-delete with ShaderManager)
    std::shared_ptr<QOpenGLShaderProgram> m_bloomExtractShader;
    std::shared_ptr<QOpenGLShaderProgram> m_bloomBlurShader;
    std::shared_ptr<QOpenGLShaderProgram> m_bloomCombineShader;
    std::shared_ptr<QOpenGLShaderProgram> m_shadowMapShader;
    std::shared_ptr<QOpenGLShaderProgram> m_volumetricLightingShader;
    std::shared_ptr<QOpenGLShaderProgram> m_lightShaftsShader;
    std::shared_ptr<QOpenGLShaderProgram> m_finalCompositeShader;

    // Geometry (using smart pointers for better memory management)
    std::unique_ptr<QOpenGLVertexArrayObject> m_quadVAO;
    std::unique_ptr<QOpenGLBuffer> m_quadVBO;
    std::unique_ptr<QOpenGLBuffer> m_quadEBO;

    // Settings
    QSize m_size;
    bool m_bloomEnabled;
    float m_bloomThreshold;
    float m_bloomIntensity;
    float m_bloomRadius;
    bool m_shadowMappingEnabled;
    int m_shadowMapSize;
    bool m_volumetricLightingEnabled;
    float m_volumetricIntensity;
    bool m_lightShaftsEnabled;
    float m_lightShaftsIntensity;
    bool m_msaaEnabled;
    int m_msaaSamples;

    // System state
    bool m_initialized;

    // Configuration constants (reduced for memory optimization)
    static const int DEFAULT_SHADOW_MAP_SIZE = 1024;  // Reduced from 2048
    static const int DEFAULT_MSAA_SAMPLES = 2;  // Reduced from 4
    static const float DEFAULT_BLOOM_THRESHOLD;
    static const float DEFAULT_BLOOM_INTENSITY;
    static const float DEFAULT_BLOOM_RADIUS;
};

#endif // POSTPROCESSINGSYSTEM_H