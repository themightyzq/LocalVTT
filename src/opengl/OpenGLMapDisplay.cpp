#include "OpenGLMapDisplay.h"
#include "ShaderManager.h"
#include "PostProcessingSystem.h"
#include "utils/DebugConsole.h"
#include <QTimer>
#include <QMatrix4x4>
#include <QDateTime>

// Fullscreen quad vertices (position + texture coordinates)
const float OpenGLMapDisplay::s_quadVertices[] = {
    // positions    // texture coords
    -1.0f,  1.0f,   0.0f, 1.0f,  // top left
    -1.0f, -1.0f,   0.0f, 0.0f,  // bottom left
     1.0f, -1.0f,   1.0f, 0.0f,  // bottom right
     1.0f,  1.0f,   1.0f, 1.0f   // top right
};

// Quad indices
const unsigned int OpenGLMapDisplay::s_quadIndices[] = {
    0, 1, 2,  // first triangle
    0, 2, 3   // second triangle
};


OpenGLMapDisplay::OpenGLMapDisplay(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_mapTexture(nullptr)
    , m_vao(nullptr)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_shaderManager(nullptr)
    , m_postProcessingSystem(nullptr)
    , m_basicShader(nullptr)
    , m_lightingShader(nullptr)
    , m_lightingEnabled(false)
    , m_ambientLightLevel(0.2f)
    , m_timeOfDay(1) // Day
    , m_hdrEnabled(true)
    , m_exposure(0.5f) // Default exposure for balanced lighting
    , m_initialized(false)
{
    // Set OpenGL format requirements
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // 4x MSAA
    setFormat(format);
}

OpenGLMapDisplay::~OpenGLMapDisplay()
{
    makeCurrent();

    // Smart pointers handle cleanup automatically

    doneCurrent();
}

void OpenGLMapDisplay::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        DebugConsole::error("Failed to initialize OpenGL functions", "OpenGL");
        return;
    }

    // Print OpenGL info
    DebugConsole::system(QString("OpenGL Version: %1").arg(reinterpret_cast<const char*>(glGetString(GL_VERSION))), "OpenGL");
    DebugConsole::system(QString("OpenGL Vendor: %1").arg(reinterpret_cast<const char*>(glGetString(GL_VENDOR))), "OpenGL");
    DebugConsole::system(QString("OpenGL Renderer: %1").arg(reinterpret_cast<const char*>(glGetString(GL_RENDERER))), "OpenGL");

    // Set OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Initialize shader manager
    m_shaderManager = std::make_unique<ShaderManager>();

    // Initialize post-processing system
    m_postProcessingSystem = std::make_unique<PostProcessingSystem>();
    m_postProcessingSystem->initialize(m_shaderManager.get(), size());

    // Setup vertex buffer and VAO
    setupVertexBuffer();

    // Setup shaders
    setupShaders();

    m_initialized = true;
    DebugConsole::system("OpenGLMapDisplay initialized successfully", "OpenGL");

    // If a texture upload was requested before init, upload it now in this context
    if (!m_pendingTextureImage.isNull()) {
        loadTexture(m_pendingTextureImage);
        m_pendingTextureImage = QImage();
    }
}

void OpenGLMapDisplay::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);

    // Create orthographic projection matrix
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    // Resize post-processing system
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->resize(QSize(width, height));
    }

    update();
}

void OpenGLMapDisplay::paintGL()
{
    // Begin post-processing frame if enabled
    bool usePostProcessing = m_postProcessingSystem && m_postProcessingSystem->isInitialized() &&
                           (m_postProcessingSystem->isBloomEnabled() ||
                            m_postProcessingSystem->isShadowMappingEnabled() ||
                            m_postProcessingSystem->isVolumetricLightingEnabled() ||
                            m_postProcessingSystem->isLightShaftsEnabled() ||
                            m_postProcessingSystem->isMSAAEnabled());

    if (usePostProcessing) {
        m_postProcessingSystem->beginFrame();
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // CRITICAL FIX: Validate texture and check if it's created/valid
    if (!m_mapTexture || !m_initialized) {
        // Show black screen gracefully if no texture
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (usePostProcessing) {
            m_postProcessingSystem->endFrame();
        }
        return;
    }

    // CRITICAL FIX: Additional texture validation
    if (!m_mapTexture->isCreated()) {
        DebugConsole::warning("OpenGL texture not created, attempting recovery", "OpenGL");
        // Attempt to recover by reloading the pending texture
        if (!m_pendingTextureImage.isNull()) {
            loadTexture(m_pendingTextureImage);
        }
        // Still show black screen for this frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (usePostProcessing) {
            m_postProcessingSystem->endFrame();
        }
        return;
    }

    // Choose shader program based on lighting state
    QOpenGLShaderProgram* activeShader = m_lightingEnabled ? m_lightingShader.get() : m_basicShader.get();
    if (!activeShader || !activeShader->bind()) {
        DebugConsole::warning("Failed to bind shader program", "OpenGL");
        if (usePostProcessing) {
            m_postProcessingSystem->endFrame();
        }
        return;
    }

    // Update uniforms
    updateUniforms();

    // Bind texture
    m_mapTexture->bind(0);

    // Render quad
    renderQuad();

    // Release resources
    m_mapTexture->release();
    activeShader->release();

    // End post-processing frame
    if (usePostProcessing) {
        m_postProcessingSystem->endFrame();
    }
}

bool OpenGLMapDisplay::loadTexture(const QImage& image)
{
    if (image.isNull()) {
        DebugConsole::warning("Cannot load null image", "OpenGL");
        return false;
    }

    // If GL not initialized yet, defer upload until initializeGL runs
    if (!m_initialized) {
        m_pendingTextureImage = image;
        update();
        return true;
    }

    makeCurrent();

    // Reset old texture if it exists (smart pointer handles deletion)
    if (m_mapTexture) {
        m_mapTexture.reset();
    }

    // Create texture (flip vertically for OpenGL texture coordinates)
    m_mapTexture = std::make_unique<QOpenGLTexture>(image.mirrored(false, true));
    m_mapTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_mapTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_mapTexture->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureSize = image.size();

    doneCurrent();
    update();

    return true;
}

void OpenGLMapDisplay::setupVertexBuffer()
{
    // Create VAO
    m_vao = std::make_unique<QOpenGLVertexArrayObject>();
    if (!m_vao->create()) {
        DebugConsole::error("Failed to create VAO", "OpenGL");
        return;
    }
    m_vao->bind();

    // Create vertex buffer
    m_vertexBuffer = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    if (!m_vertexBuffer->create()) {
        DebugConsole::error("Failed to create vertex buffer", "OpenGL");
        return;
    }
    m_vertexBuffer->bind();
    m_vertexBuffer->allocate(s_quadVertices, sizeof(s_quadVertices));

    // Create index buffer
    m_indexBuffer = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    if (!m_indexBuffer->create()) {
        DebugConsole::error("Failed to create index buffer", "OpenGL");
        return;
    }
    m_indexBuffer->bind();
    m_indexBuffer->allocate(s_quadIndices, sizeof(s_quadIndices));

    // Set vertex attributes
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    // Texture coordinate attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         reinterpret_cast<void*>(2 * sizeof(float)));

    m_vao->release();
}

void OpenGLMapDisplay::setupShaders()
{
    if (!m_shaderManager) {
        DebugConsole::error("ShaderManager not initialized", "OpenGL");
        return;
    }

    // Load basic shader for simple texture rendering
    m_basicShader = m_shaderManager->loadShaderProgram("basic");
    if (!m_basicShader) {
        DebugConsole::error("Failed to load basic shader", "OpenGL");
        return;
    }

    // Load lighting shader for HDR lighting
    m_lightingShader = m_shaderManager->loadShaderProgram("lighting");
    if (!m_lightingShader) {
        DebugConsole::error("Failed to load lighting shader", "OpenGL");
        return;
    }

    DebugConsole::system("Shaders loaded successfully", "OpenGL");
}

void OpenGLMapDisplay::updateUniforms()
{
    QOpenGLShaderProgram* activeShader = m_lightingEnabled ? m_lightingShader.get() : m_basicShader.get();
    if (!activeShader) return;

    // Set transformation matrices
    activeShader->setUniformValue("u_projection", m_projectionMatrix);
    activeShader->setUniformValue("u_view", m_viewMatrix);
    activeShader->setUniformValue("u_model", m_modelMatrix);

    // Set texture sampler
    activeShader->setUniformValue("u_texture", 0);

    // Set lighting uniforms if using lighting shader
    if (m_lightingEnabled && activeShader == m_lightingShader.get()) {
        activeShader->setUniformValue("u_ambientLight", m_ambientLightLevel);
        activeShader->setUniformValue("u_timeOfDay", m_timeOfDay);

        // Set HDR uniforms
        activeShader->setUniformValue("u_useHDR", m_hdrEnabled);
        activeShader->setUniformValue("u_exposure", m_exposure);

        // No point lights - simplified lighting only
        activeShader->setUniformValue("u_numPointLights", 0);
    }
}

void OpenGLMapDisplay::renderQuad()
{
    if (!m_vao) return;

    m_vao->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    m_vao->release();
}

void OpenGLMapDisplay::setProjectionMatrix(const QMatrix4x4& projection)
{
    m_projectionMatrix = projection;
    update();
}

void OpenGLMapDisplay::setViewMatrix(const QMatrix4x4& view)
{
    m_viewMatrix = view;
    update();
}

void OpenGLMapDisplay::setModelMatrix(const QMatrix4x4& model)
{
    m_modelMatrix = model;
    update();
}

void OpenGLMapDisplay::setLightingEnabled(bool enabled)
{
    m_lightingEnabled = enabled;
    update();
}

void OpenGLMapDisplay::setAmbientLightLevel(float level)
{
    m_ambientLightLevel = qBound(0.0f, level, 1.0f);
    update();
}

void OpenGLMapDisplay::setTimeOfDay(int timeOfDay)
{
    m_timeOfDay = qBound(0, timeOfDay, 3);
    update();
}

void OpenGLMapDisplay::setHDREnabled(bool enabled)
{
    m_hdrEnabled = enabled;
    update();
}

void OpenGLMapDisplay::setExposure(float exposure)
{
    m_exposure = qBound(0.01f, exposure, 10.0f);
    update();
}

QSize OpenGLMapDisplay::getTextureSize() const
{
    return m_textureSize;
}

// Post-processing effects control methods
void OpenGLMapDisplay::setBloomEnabled(bool enabled)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setBloomEnabled(enabled);
        update();
    }
}

void OpenGLMapDisplay::setBloomThreshold(float threshold)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setBloomThreshold(threshold);
        update();
    }
}

void OpenGLMapDisplay::setBloomIntensity(float intensity)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setBloomIntensity(intensity);
        update();
    }
}

void OpenGLMapDisplay::setBloomRadius(float radius)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setBloomRadius(radius);
        update();
    }
}

void OpenGLMapDisplay::setShadowMappingEnabled(bool enabled)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setShadowMappingEnabled(enabled);
        update();
    }
}

void OpenGLMapDisplay::setShadowMapSize(int size)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setShadowMapSize(size);
        update();
    }
}

void OpenGLMapDisplay::setVolumetricLightingEnabled(bool enabled)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setVolumetricLightingEnabled(enabled);
        update();
    }
}

void OpenGLMapDisplay::setVolumetricIntensity(float intensity)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setVolumetricIntensity(intensity);
        update();
    }
}

void OpenGLMapDisplay::setLightShaftsEnabled(bool enabled)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setLightShaftsEnabled(enabled);
        update();
    }
}

void OpenGLMapDisplay::setLightShaftsIntensity(float intensity)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setLightShaftsIntensity(intensity);
        update();
    }
}

void OpenGLMapDisplay::setMSAAEnabled(bool enabled)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setMSAAEnabled(enabled);
        update();
    }
}

void OpenGLMapDisplay::setMSAASamples(int samples)
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        m_postProcessingSystem->setMSAASamples(samples);
        update();
    }
}

// Post-processing getters
bool OpenGLMapDisplay::isBloomEnabled() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->isBloomEnabled();
    }
    return false;
}

bool OpenGLMapDisplay::isShadowMappingEnabled() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->isShadowMappingEnabled();
    }
    return false;
}

bool OpenGLMapDisplay::isVolumetricLightingEnabled() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->isVolumetricLightingEnabled();
    }
    return false;
}

bool OpenGLMapDisplay::isLightShaftsEnabled() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->isLightShaftsEnabled();
    }
    return false;
}

bool OpenGLMapDisplay::isMSAAEnabled() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->isMSAAEnabled();
    }
    return false;
}

// Post-processing parameter getters
float OpenGLMapDisplay::getBloomThreshold() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getBloomThreshold();
    }
    return 0.8f; // Default value
}

float OpenGLMapDisplay::getBloomIntensity() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getBloomIntensity();
    }
    return 1.0f; // Default value
}

float OpenGLMapDisplay::getBloomRadius() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getBloomRadius();
    }
    return 1.0f; // Default value
}

int OpenGLMapDisplay::getShadowMapSize() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getShadowMapSize();
    }
    return 2048; // Default value
}

float OpenGLMapDisplay::getVolumetricIntensity() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getVolumetricIntensity();
    }
    return 0.5f; // Default value
}

float OpenGLMapDisplay::getLightShaftsIntensity() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getLightShaftsIntensity();
    }
    return 0.5f; // Default value
}

int OpenGLMapDisplay::getMSAASamples() const
{
    if (m_postProcessingSystem && m_postProcessingSystem->isInitialized()) {
        return m_postProcessingSystem->getMSAASamples();
    }
    return 4; // Default value
}
