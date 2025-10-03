#include "PostProcessingSystem.h"
#include "ShaderManager.h"
#include "utils/DebugConsole.h"
#include <QOpenGLFramebufferObjectFormat>

// Default constants
const float PostProcessingSystem::DEFAULT_BLOOM_THRESHOLD = 1.0f;
const float PostProcessingSystem::DEFAULT_BLOOM_INTENSITY = 0.5f;
const float PostProcessingSystem::DEFAULT_BLOOM_RADIUS = 2.0f;

PostProcessingSystem::PostProcessingSystem()
    : m_shaderManager(nullptr)
    , m_sceneFramebuffer(nullptr)
    , m_bloomFramebuffer(nullptr)
    , m_brightPixelsFramebuffer(nullptr)
    , m_blurFramebuffer1(nullptr)
    , m_blurFramebuffer2(nullptr)
    , m_shadowMapFramebuffer(nullptr)
    , m_msaaFramebuffer(nullptr)
    , m_bloomExtractShader(nullptr)
    , m_bloomBlurShader(nullptr)
    , m_bloomCombineShader(nullptr)
    , m_shadowMapShader(nullptr)
    , m_volumetricLightingShader(nullptr)
    , m_lightShaftsShader(nullptr)
    , m_finalCompositeShader(nullptr)
    , m_quadVAO(nullptr)
    , m_quadVBO(nullptr)
    , m_quadEBO(nullptr)
    , m_bloomEnabled(false)
    , m_bloomThreshold(DEFAULT_BLOOM_THRESHOLD)
    , m_bloomIntensity(DEFAULT_BLOOM_INTENSITY)
    , m_bloomRadius(DEFAULT_BLOOM_RADIUS)
    , m_shadowMappingEnabled(false)
    , m_shadowMapSize(DEFAULT_SHADOW_MAP_SIZE)
    , m_volumetricLightingEnabled(false)
    , m_volumetricIntensity(0.5f)
    , m_lightShaftsEnabled(false)
    , m_lightShaftsIntensity(0.3f)
    , m_msaaEnabled(false)
    , m_msaaSamples(DEFAULT_MSAA_SAMPLES)
    , m_initialized(false)
{
}

PostProcessingSystem::~PostProcessingSystem()
{
    cleanup();
}

void PostProcessingSystem::initialize(ShaderManager* shaderManager, const QSize& size)
{
    if (m_initialized) {
        cleanup();
    }

    if (!initializeOpenGLFunctions()) {
        DebugConsole::warning("Failed to initialize OpenGL functions", "OpenGL");
        return;
    }

    m_shaderManager = shaderManager;
    m_size = size;

    setupFramebuffers();
    setupShaders();
    setupQuadGeometry();

    m_initialized = true;
    DebugConsole::system(QString("PostProcessingSystem initialized with size: %1x%2").arg(size.width()).arg(size.height()), "OpenGL");
}

void PostProcessingSystem::cleanup()
{
    if (!m_initialized) return;

    // Smart pointers handle cleanup automatically
    m_quadVAO.reset();
    m_quadVBO.reset();
    m_quadEBO.reset();

    // Framebuffers are automatically cleaned up by unique_ptr
    m_sceneFramebuffer.reset();
    m_bloomFramebuffer.reset();
    m_brightPixelsFramebuffer.reset();
    m_blurFramebuffer1.reset();
    m_blurFramebuffer2.reset();
    m_shadowMapFramebuffer.reset();
    m_msaaFramebuffer.reset();

    // Note: Don't access m_shaderManager here as it might already be deleted
    // The shaders are owned by ShaderManager and will be cleaned up there
    // Just reset our pointers without trying to delete anything

    // Reset shader pointers
    m_bloomExtractShader = nullptr;
    m_bloomBlurShader = nullptr;
    m_bloomCombineShader = nullptr;
    m_shadowMapShader = nullptr;
    m_volumetricLightingShader = nullptr;
    m_lightShaftsShader = nullptr;
    m_finalCompositeShader = nullptr;

    // Clear the shader manager pointer to avoid dangling references
    m_shaderManager = nullptr;

    m_initialized = false;
}

void PostProcessingSystem::resize(const QSize& size)
{
    if (!m_initialized || m_size == size) return;

    m_size = size;
    setupFramebuffers();
}

void PostProcessingSystem::setupFramebuffers()
{
    // MEMORY OPTIMIZATION: Only create main framebuffer initially
    // Other framebuffers will be created on-demand when effects are enabled

    // Create main scene framebuffer (always needed)
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(GL_RGBA16F); // HDR format
    format.setSamples(m_msaaEnabled ? m_msaaSamples : 0);

    m_sceneFramebuffer = std::make_unique<QOpenGLFramebufferObject>(m_size, format);
    if (!m_sceneFramebuffer->isValid()) {
        DebugConsole::warning("Failed to create scene framebuffer", "OpenGL");
        return;
    }

    // MEMORY OPTIMIZATION: Don't create bloom framebuffers unless bloom is enabled
    if (m_bloomEnabled) {
        // Create bloom framebuffers (quarter resolution for performance)
        QSize bloomSize = m_size / 4;
        QOpenGLFramebufferObjectFormat bloomFormat;
        bloomFormat.setInternalTextureFormat(GL_RGBA16F);

        m_bloomFramebuffer = std::make_unique<QOpenGLFramebufferObject>(bloomSize, bloomFormat);
        m_brightPixelsFramebuffer = std::make_unique<QOpenGLFramebufferObject>(bloomSize, bloomFormat);
        m_blurFramebuffer1 = std::make_unique<QOpenGLFramebufferObject>(bloomSize, bloomFormat);
        m_blurFramebuffer2 = std::make_unique<QOpenGLFramebufferObject>(bloomSize, bloomFormat);

        // Validate bloom framebuffers
        if (!m_bloomFramebuffer->isValid() || !m_brightPixelsFramebuffer->isValid() ||
            !m_blurFramebuffer1->isValid() || !m_blurFramebuffer2->isValid()) {
            DebugConsole::warning("Failed to create bloom framebuffers", "OpenGL");
            // Disable bloom if framebuffers fail
            m_bloomEnabled = false;
            m_bloomFramebuffer.reset();
            m_brightPixelsFramebuffer.reset();
            m_blurFramebuffer1.reset();
            m_blurFramebuffer2.reset();
            return;
        }
    }

    // MEMORY OPTIMIZATION: Only create shadow map framebuffer when shadows are enabled
    if (m_shadowMappingEnabled) {
        QOpenGLFramebufferObjectFormat shadowFormat;
        shadowFormat.setAttachment(QOpenGLFramebufferObject::Depth);
        shadowFormat.setInternalTextureFormat(GL_DEPTH_COMPONENT24);
        QSize shadowSize(m_shadowMapSize, m_shadowMapSize);
        m_shadowMapFramebuffer = std::make_unique<QOpenGLFramebufferObject>(shadowSize, shadowFormat);
        if (!m_shadowMapFramebuffer->isValid()) {
            DebugConsole::warning("Failed to create shadow map framebuffer", "OpenGL");
            m_shadowMappingEnabled = false;
            m_shadowMapFramebuffer.reset();
        }
    }

    // MEMORY OPTIMIZATION: Only create MSAA framebuffer when MSAA is enabled
    if (m_msaaEnabled) {
        QOpenGLFramebufferObjectFormat resolveFormat;
        resolveFormat.setInternalTextureFormat(GL_RGBA16F);
        m_msaaFramebuffer = std::make_unique<QOpenGLFramebufferObject>(m_size, resolveFormat);
        if (!m_msaaFramebuffer->isValid()) {
            DebugConsole::warning("Failed to create MSAA framebuffer", "OpenGL");
            m_msaaEnabled = false;
            m_msaaFramebuffer.reset();
        }
    }
}

void PostProcessingSystem::setupShaders()
{
    if (!m_shaderManager) return;

    // Load post-processing shaders
    m_bloomExtractShader = m_shaderManager->getShaderProgram("bloom_extract");
    m_bloomBlurShader = m_shaderManager->getShaderProgram("bloom_blur");
    m_bloomCombineShader = m_shaderManager->getShaderProgram("bloom_combine");
    m_shadowMapShader = m_shaderManager->getShaderProgram("shadow_map");
    m_volumetricLightingShader = m_shaderManager->getShaderProgram("volumetric_lighting");
    m_lightShaftsShader = m_shaderManager->getShaderProgram("light_shafts");
    m_finalCompositeShader = m_shaderManager->getShaderProgram("final_composite");

    // Create basic shaders if they don't exist
    if (!m_bloomExtractShader) {
        // Create bloom extract shader inline
        QString vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        QString fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D sceneTexture;
            uniform float threshold;
            void main() {
                vec3 color = texture(sceneTexture, TexCoord).rgb;
                float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
                FragColor = brightness > threshold ? vec4(color, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
            }
        )";

        m_bloomExtractShader = std::make_unique<QOpenGLShaderProgram>();
        m_bloomExtractShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
        m_bloomExtractShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
        m_bloomExtractShader->link();
    }

    if (!m_bloomBlurShader) {
        QString vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        QString fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D image;
            uniform bool horizontal;
            uniform float radius;
            const float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
            void main() {
                vec2 tex_offset = 1.0 / textureSize(image, 0) * radius;
                vec3 result = texture(image, TexCoord).rgb * weights[0];
                if(horizontal) {
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
                        result += texture(image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
                    }
                } else {
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
                        result += texture(image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
                    }
                }
                FragColor = vec4(result, 1.0);
            }
        )";

        m_bloomBlurShader = std::make_unique<QOpenGLShaderProgram>();
        m_bloomBlurShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
        m_bloomBlurShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
        m_bloomBlurShader->link();
    }

    if (!m_bloomCombineShader) {
        QString vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        QString fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D sceneTexture;
            uniform sampler2D bloomTexture;
            uniform float intensity;
            void main() {
                vec3 scene = texture(sceneTexture, TexCoord).rgb;
                vec3 bloom = texture(bloomTexture, TexCoord).rgb;
                FragColor = vec4(scene + bloom * intensity, 1.0);
            }
        )";

        m_bloomCombineShader = std::make_unique<QOpenGLShaderProgram>();
        m_bloomCombineShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
        m_bloomCombineShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
        m_bloomCombineShader->link();
    }

    // Create basic final composite shader
    if (!m_finalCompositeShader) {
        QString vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        QString fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D sceneTexture;
            uniform bool volumetricEnabled;
            uniform bool lightShaftsEnabled;
            uniform float volumetricIntensity;
            uniform float lightShaftsIntensity;

            vec3 volumetricLighting(vec2 uv) {
                // Simple volumetric lighting approximation
                vec2 center = vec2(0.5, 0.5);
                float dist = distance(uv, center);
                float fade = 1.0 - smoothstep(0.0, 0.7, dist);
                return vec3(0.1, 0.1, 0.2) * fade * volumetricIntensity;
            }

            vec3 lightShafts(vec2 uv) {
                // Simple light shafts effect
                vec2 lightPos = vec2(0.3, 0.8);
                vec2 dir = normalize(uv - lightPos);
                float dist = distance(uv, lightPos);
                float shafts = sin(dist * 20.0) * 0.5 + 0.5;
                float fade = 1.0 - smoothstep(0.0, 0.5, dist);
                return vec3(1.0, 0.9, 0.7) * shafts * fade * lightShaftsIntensity * 0.1;
            }

            void main() {
                vec3 color = texture(sceneTexture, TexCoord).rgb;

                if (volumetricEnabled) {
                    color += volumetricLighting(TexCoord);
                }

                if (lightShaftsEnabled) {
                    color += lightShafts(TexCoord);
                }

                FragColor = vec4(color, 1.0);
            }
        )";

        m_finalCompositeShader = std::make_unique<QOpenGLShaderProgram>();
        m_finalCompositeShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
        m_finalCompositeShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
        m_finalCompositeShader->link();
    }
}

void PostProcessingSystem::setupQuadGeometry()
{
    // Create fullscreen quad
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    unsigned int quadIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    m_quadVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_quadVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    m_quadEBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer); // Fix: Make EBO a member variable

    m_quadVAO->create();
    m_quadVAO->bind();

    m_quadVBO->create();
    m_quadVBO->bind();
    m_quadVBO->allocate(quadVertices, sizeof(quadVertices));

    m_quadEBO->create();
    m_quadEBO->bind();
    m_quadEBO->allocate(quadIndices, sizeof(quadIndices));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_quadVAO->release();
}

void PostProcessingSystem::beginFrame()
{
    if (!m_initialized) return;

    // Bind scene framebuffer for main rendering
    bindFramebuffer(m_sceneFramebuffer.get());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessingSystem::endFrame()
{
    if (!m_initialized) return;

    // Apply post-processing effects
    if (m_bloomEnabled) {
        renderBloom();
    }

    if (m_shadowMappingEnabled) {
        renderShadowMap();
    }

    if (m_msaaEnabled) {
        resolveMSAA();
    }

    // Final composite
    renderToScreen();
}

void PostProcessingSystem::renderToScreen()
{
    if (!m_initialized || !m_finalCompositeShader) return;

    // Bind default framebuffer (screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_size.width(), m_size.height());
    glClear(GL_COLOR_BUFFER_BIT);

    m_finalCompositeShader->bind();

    // Set uniforms
    m_finalCompositeShader->setUniformValue("sceneTexture", 0);
    m_finalCompositeShader->setUniformValue("volumetricEnabled", m_volumetricLightingEnabled);
    m_finalCompositeShader->setUniformValue("lightShaftsEnabled", m_lightShaftsEnabled);
    m_finalCompositeShader->setUniformValue("volumetricIntensity", m_volumetricIntensity);
    m_finalCompositeShader->setUniformValue("lightShaftsIntensity", m_lightShaftsIntensity);

    // Bind scene texture
    glActiveTexture(GL_TEXTURE0);
    GLuint textureId = m_bloomEnabled && m_bloomFramebuffer ?
        m_bloomFramebuffer->texture() : m_sceneFramebuffer->texture();
    glBindTexture(GL_TEXTURE_2D, textureId);

    renderFullscreenQuad();
    m_finalCompositeShader->release();
}

void PostProcessingSystem::renderBloom()
{
    if (!m_bloomEnabled || !m_bloomExtractShader || !m_bloomBlurShader || !m_bloomCombineShader) {
        return;
    }

    extractBrightPixels();
    blurBrightPixels();
    combineBloom();
}

void PostProcessingSystem::extractBrightPixels()
{
    bindFramebuffer(m_brightPixelsFramebuffer.get());
    glClear(GL_COLOR_BUFFER_BIT);

    m_bloomExtractShader->bind();
    m_bloomExtractShader->setUniformValue("sceneTexture", 0);
    m_bloomExtractShader->setUniformValue("threshold", m_bloomThreshold);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneFramebuffer->texture());

    renderFullscreenQuad();
    m_bloomExtractShader->release();
}

void PostProcessingSystem::blurBrightPixels()
{
    bool horizontal = true;
    bool first_iteration = true;
    int amount = 5; // Performance optimization: Reduce from 10 to 5 iterations

    m_bloomBlurShader->bind();
    m_bloomBlurShader->setUniformValue("radius", m_bloomRadius);

    for (int i = 0; i < amount; i++) {
        bindFramebuffer(horizontal ? m_blurFramebuffer1.get() : m_blurFramebuffer2.get());
        m_bloomBlurShader->setUniformValue("horizontal", horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration ?
            m_brightPixelsFramebuffer->texture() :
            (horizontal ? m_blurFramebuffer2->texture() : m_blurFramebuffer1->texture()));

        renderFullscreenQuad();
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    m_bloomBlurShader->release();
}

void PostProcessingSystem::combineBloom()
{
    bindFramebuffer(m_bloomFramebuffer.get());
    glClear(GL_COLOR_BUFFER_BIT);

    m_bloomCombineShader->bind();
    m_bloomCombineShader->setUniformValue("sceneTexture", 0);
    m_bloomCombineShader->setUniformValue("bloomTexture", 1);
    m_bloomCombineShader->setUniformValue("intensity", m_bloomIntensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneFramebuffer->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blurFramebuffer1->texture());

    renderFullscreenQuad();
    m_bloomCombineShader->release();
}

void PostProcessingSystem::renderShadowMap()
{
    if (!m_shadowMappingEnabled || !m_shadowMapFramebuffer) return;

    // Shadow map rendering would require light and scene geometry information
    // This is a placeholder for shadow map generation
    bindFramebuffer(m_shadowMapFramebuffer.get());
    glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Shadow map rendering logic would go here
    // Requires scene geometry and light position/direction
}

void PostProcessingSystem::renderVolumetricLighting()
{
    // Volumetric lighting is handled in the final composite shader
}

void PostProcessingSystem::renderLightShafts()
{
    // Light shafts are handled in the final composite shader
}

void PostProcessingSystem::resolveMSAA()
{
    if (!m_msaaEnabled || !m_msaaFramebuffer) return;

    // Resolve MSAA framebuffer to regular framebuffer
    QOpenGLFramebufferObject::blitFramebuffer(
        m_msaaFramebuffer.get(), m_sceneFramebuffer.get(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
}

void PostProcessingSystem::renderFullscreenQuad()
{
    if (!m_quadVAO) return;

    m_quadVAO->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_quadVAO->release();
}

void PostProcessingSystem::bindFramebuffer(QOpenGLFramebufferObject* fbo)
{
    if (fbo) {
        fbo->bind();
        glViewport(0, 0, fbo->width(), fbo->height());
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_size.width(), m_size.height());
    }
}

// Bloom controls
void PostProcessingSystem::setBloomEnabled(bool enabled) {
    if (m_bloomEnabled == enabled) return;
    m_bloomEnabled = enabled;
    // MEMORY OPTIMIZATION: Create/destroy framebuffers on demand
    if (m_initialized) {
        if (enabled && !m_bloomFramebuffer) {
            // Create bloom framebuffers when enabling
            setupFramebuffers();
        } else if (!enabled && m_bloomFramebuffer) {
            // Free bloom framebuffers when disabling
            m_bloomFramebuffer.reset();
            m_brightPixelsFramebuffer.reset();
            m_blurFramebuffer1.reset();
            m_blurFramebuffer2.reset();
        }
    }
}
void PostProcessingSystem::setBloomThreshold(float threshold) { m_bloomThreshold = threshold; }
void PostProcessingSystem::setBloomIntensity(float intensity) { m_bloomIntensity = intensity; }
void PostProcessingSystem::setBloomRadius(float radius) { m_bloomRadius = radius; }

// Shadow mapping controls
void PostProcessingSystem::setShadowMappingEnabled(bool enabled) {
    if (m_shadowMappingEnabled == enabled) return;
    m_shadowMappingEnabled = enabled;
    // MEMORY OPTIMIZATION: Create/destroy shadow framebuffer on demand
    if (m_initialized) {
        if (enabled && !m_shadowMapFramebuffer) {
            setupFramebuffers();
        } else if (!enabled && m_shadowMapFramebuffer) {
            m_shadowMapFramebuffer.reset();
        }
    }
}
void PostProcessingSystem::setShadowMapSize(int size) {
    m_shadowMapSize = size;
    if (m_initialized && m_shadowMappingEnabled) setupFramebuffers();
}

// Volumetric lighting controls
void PostProcessingSystem::setVolumetricLightingEnabled(bool enabled) { m_volumetricLightingEnabled = enabled; }
void PostProcessingSystem::setVolumetricIntensity(float intensity) { m_volumetricIntensity = intensity; }

// Light shafts controls
void PostProcessingSystem::setLightShaftsEnabled(bool enabled) { m_lightShaftsEnabled = enabled; }
void PostProcessingSystem::setLightShaftsIntensity(float intensity) { m_lightShaftsIntensity = intensity; }

// MSAA controls
void PostProcessingSystem::setMSAAEnabled(bool enabled) {
    if (m_msaaEnabled == enabled) return;
    m_msaaEnabled = enabled;
    // MEMORY OPTIMIZATION: Create/destroy MSAA framebuffer on demand
    if (m_initialized) {
        if (enabled && !m_msaaFramebuffer) {
            setupFramebuffers();
        } else if (!enabled && m_msaaFramebuffer) {
            m_msaaFramebuffer.reset();
        }
    }
}
void PostProcessingSystem::setMSAASamples(int samples) {
    m_msaaSamples = samples;
    if (m_initialized && m_msaaEnabled) setupFramebuffers();
}