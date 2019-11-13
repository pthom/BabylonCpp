#include <babylon/engines/null_engine.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/core/logging.h>
#include <babylon/materials/effect.h>
#include <babylon/materials/textures/internal_texture.h>
#include <babylon/materials/textures/irender_target_options.h>
#include <babylon/states/_alpha_state.h>
#include <babylon/states/_depth_culling_state.h>
#include <babylon/states/_stencil_state.h>

namespace BABYLON {

bool NullEngine::isDeterministicLockStep() const
{
  return _options.deterministicLockstep;
}

bool NullEngine::getLockstepMaxSteps() const
{
  return _options.lockstepMaxSteps;
}

float NullEngine::getHardwareScalingLevel() const
{
  return 1.f;
}

NullEngine::NullEngine(const NullEngineOptions& options) : Engine{nullptr}
{
  _options = options;

  // Init caps
  // We consider we are on a webgl1 capable device

  _caps                            = EngineCapabilities();
  _caps.maxTexturesImageUnits      = 16;
  _caps.maxVertexTextureImageUnits = 16;
  _caps.maxTextureSize             = 512;
  _caps.maxCubemapTextureSize      = 512;
  _caps.maxRenderTextureSize       = 512;
  _caps.maxVertexAttribs           = 16;
  _caps.maxVaryingVectors          = 16;
  _caps.maxFragmentUniformVectors  = 16;
  _caps.maxVertexUniformVectors    = 16;

  // Extensions
  _caps.standardDerivatives = false;

  _caps.astc  = nullptr;
  _caps.s3tc  = std::nullopt;
  _caps.pvrtc = nullptr;
  _caps.etc1  = nullptr;
  _caps.etc2  = nullptr;

  _caps.textureAnisotropicFilterExtension = std::nullopt;
  _caps.maxAnisotropy                     = 0;
  _caps.uintIndices                       = false;
  _caps.fragmentDepthSupported            = false;
  _caps.highPrecisionShaderSupported      = true;

  _caps.colorBufferFloat            = false;
  _caps.textureFloat                = false;
  _caps.textureFloatLinearFiltering = false;
  _caps.textureFloatRender          = false;

  _caps.textureHalfFloat                = false;
  _caps.textureHalfFloatLinearFiltering = false;
  _caps.textureHalfFloatRender          = false;

  _caps.textureLOD           = false;
  _caps.drawBuffersExtension = false;

  _caps.depthTextureExtension = false;
  _caps.vertexArrayObject     = false;
  _caps.instancedArrays       = false;

  BABYLON_LOGF_INFO("Engine", "Babylon.js v%s - Null engine",
                    Engine::Version().c_str())
}

NullEngine::~NullEngine() = default;

Engine::GLBufferPtr
NullEngine::createVertexBuffer(const Float32Array& /*vertices*/)
{
  auto buffer        = std::make_unique<GL::IGLBuffer>(0);
  buffer->references = 1;
  return buffer;
}

Engine::GLBufferPtr
NullEngine::createIndexBuffer(const IndicesArray& /*indices*/,
                              bool /*updatable*/)
{
  auto buffer        = std::make_unique<GL::IGLBuffer>(0);
  buffer->references = 1;
  return buffer;
}

void NullEngine::clear(const Color4& /*color*/, bool /*backBuffer*/,
                       bool /*depth*/, bool /*stencil*/)
{
}

int NullEngine::getRenderWidth(bool useScreen) const
{
  if (!useScreen && _currentRenderTarget) {
    return _currentRenderTarget->width;
  }

  return _options.renderWidth;
}

int NullEngine::getRenderHeight(bool useScreen) const
{
  if (!useScreen && _currentRenderTarget) {
    return _currentRenderTarget->height;
  }

  return _options.renderHeight;
}

void NullEngine::setViewport(Viewport& viewport,
                             const std::optional<int>& /*requiredWidth*/,
                             const std::optional<int>& /*requiredHeight*/
)
{
  _cachedViewport = viewport;
}

GL::IGLProgramPtr NullEngine::createShaderProgram(
  const IPipelineContextPtr& /*pipelineContext*/,
  const std::string& /*vertexCode*/, const std::string& /*fragmentCode*/,
  const std::string& /*defines*/, GL::IGLRenderingContext* /*context*/)
{
  auto program                      = std::make_shared<GL::IGLProgram>(0);
  program->__SPECTOR_rebuildProgram = nullptr;
  return program;
}

std::unordered_map<std::string, Engine::GLUniformLocationPtr>
NullEngine::getUniforms(const IPipelineContextPtr& /*pipelineContext*/,
                        const std::vector<std::string>& /*uniformsNames*/)
{
  return {};
}

Int32Array
NullEngine::getAttributes(const IPipelineContextPtr& /*pipelineContext*/,
                          const std::vector<std::string>& /*attributesNames*/)
{
  return {};
}

void NullEngine::bindSamplers(Effect& /*effect*/)
{
  _currentEffect = nullptr;
}

void NullEngine::enableEffect(const EffectPtr& effect)
{
  _currentEffect = effect;

  if (effect->onBind) {
    effect->onBind(effect.get());
  }
  /* if (effect->_onBindObservable) */ {
    effect->_onBindObservable.notifyObservers(effect.get());
  }
}

void NullEngine::setState(bool /*culling*/, float /*zOffset*/, bool /*force*/,
                          bool /*reverseSide*/)
{
}

void NullEngine::setIntArray(GL::IGLUniformLocation* /*uniform*/,
                             const Int32Array& /*array*/)
{
}

void NullEngine::setIntArray2(GL::IGLUniformLocation* /*uniform*/,
                              const Int32Array& /*array*/)
{
}

void NullEngine::setIntArray3(GL::IGLUniformLocation* /*uniform*/,
                              const Int32Array& /*array*/)
{
}

void NullEngine::setIntArray4(GL::IGLUniformLocation* /*uniform*/,
                              const Int32Array& /*array*/)
{
}

void NullEngine::setFloatArray(GL::IGLUniformLocation* /*uniform*/,
                               const Float32Array& /*array*/)
{
}

void NullEngine::setFloatArray2(GL::IGLUniformLocation* /*uniform*/,
                                const Float32Array& /*array*/)
{
}

void NullEngine::setFloatArray3(GL::IGLUniformLocation* /*uniform*/,
                                const Float32Array& /*array*/)
{
}

void NullEngine::setFloatArray4(GL::IGLUniformLocation* /*uniform*/,
                                const Float32Array& /*array*/)
{
}

void NullEngine::setArray(GL::IGLUniformLocation* /*uniform*/,
                          const Float32Array& /*array*/)
{
}

void NullEngine::setArray2(GL::IGLUniformLocation* /*uniform*/,
                           const Float32Array& /*array*/)
{
}

void NullEngine::setArray3(GL::IGLUniformLocation* /*uniform*/,
                           const Float32Array& /*array*/)
{
}

void NullEngine::setArray4(GL::IGLUniformLocation* /*uniform*/,
                           const Float32Array& /*array*/)
{
}

void NullEngine::setMatrices(GL::IGLUniformLocation* /*uniform*/,
                             const Float32Array& /*matrices*/)
{
}

void NullEngine::setMatrix(GL::IGLUniformLocation* /*uniform*/,
                           const Matrix& /*matrix*/)
{
}

void NullEngine::setMatrix3x3(GL::IGLUniformLocation* /*uniform*/,
                              const Float32Array& /*matrix*/)
{
}

void NullEngine::setMatrix2x2(GL::IGLUniformLocation* /*uniform*/,
                              const Float32Array& /*matrix*/)
{
}

void NullEngine::setInt(GL::IGLUniformLocation* /*uniform*/, int /*value*/)
{
}

void NullEngine::setFloat(GL::IGLUniformLocation* /*uniform*/, float /*value*/)
{
}

void NullEngine::setFloat2(GL::IGLUniformLocation* /*uniform*/, float /*x*/,
                           float /*y*/)
{
}

void NullEngine::setFloat3(GL::IGLUniformLocation* /*uniform*/, float /*x*/,
                           float /*y*/, float /*z*/)
{
}

void NullEngine::setBool(GL::IGLUniformLocation* /*uniform*/, int /*value*/)
{
}

void NullEngine::setFloat4(GL::IGLUniformLocation* /*uniform*/, float /*x*/,
                           float /*y*/, float /*z*/, float /*w*/)
{
}

void NullEngine::setColor3(GL::IGLUniformLocation* /*uniform*/,
                           const Color3& /*color3*/)
{
}

void NullEngine::setColor4(GL::IGLUniformLocation* /*uniform*/,
                           const Color3& /*color3*/, float /*alpha*/)
{
}

void NullEngine::setAlphaMode(unsigned int mode, bool noDepthWriteChange)
{
  if (_alphaMode == mode) {
    return;
  }

  _alphaState->alphaBlend = (mode != Constants::ALPHA_DISABLE);

  if (!noDepthWriteChange) {
    setDepthWrite(mode == Constants::ALPHA_DISABLE);
  }
  _alphaMode = mode;
}

void NullEngine::bindBuffers(
  const std::unordered_map<std::string, VertexBufferPtr>& /*vertexBuffers*/,
  GL::IGLBuffer* /*indexBuffer*/, const EffectPtr& /*effect*/)
{
}

void NullEngine::wipeCaches(bool bruteForce)
{
  if (preventCacheWipeBetweenFrames) {
    return;
  }
  resetTextureCache();
  _currentEffect = nullptr;

  if (bruteForce) {
    _currentProgram = nullptr;

    _stencilState.reset();
    _depthCullingState.reset();
    _alphaState.reset();
  }

  _cachedVertexBuffers          = nullptr;
  _cachedIndexBuffer            = nullptr;
  _cachedEffectForVertexBuffers = nullptr;
}

void NullEngine::draw(bool /*useTriangles*/, int /*indexStart*/,
                      int /*indexCount*/, int /*instancesCount*/)
{
}

void NullEngine::drawElementsType(unsigned int /*fillMode*/, int /*indexStart*/,
                                  int /*verticesCount*/, int /*instancesCount*/)
{
}

void NullEngine::drawArraysType(unsigned int /*fillMode*/,
                                int /*verticesStart*/, int /*verticesCount*/,
                                int /*instancesCount*/)
{
}

Engine::GLTexturePtr NullEngine::_createTexture()
{
  return std::make_unique<GL::IGLTexture>(0);
}

void NullEngine::_releaseTexture(InternalTexture* /*texture*/)
{
}

InternalTexturePtr NullEngine::createTexture(
  const std::string& urlArg, bool noMipmap, bool invertY, Scene* /*scene*/,
  unsigned int samplingMode,
  const std::function<void(InternalTexture*, EventState&)>& onLoad,
  const std::function<void(const std::string& message,
                           const std::string& exception)>& /*onError*/,
  const std::optional<
    std::variant<std::string, ArrayBuffer, Image>>& /*buffer*/,
  const InternalTexturePtr& /*fallBack*/,
  const std::optional<unsigned int>& format)
{
  auto texture
    = std::make_shared<InternalTexture>(this, InternalTexture::DATASOURCE_URL);
  const auto& url = urlArg;

  texture->url             = url;
  texture->generateMipMaps = !noMipmap;
  texture->samplingMode    = samplingMode;
  texture->invertY         = invertY;
  texture->baseWidth       = _options.textureSize;
  texture->baseHeight      = _options.textureSize;
  texture->width           = _options.textureSize;
  texture->height          = _options.textureSize;
  if (format.has_value()) {
    texture->format = *format;
  }

  texture->isReady = true;

  if (onLoad) {
    EventState es{0};
    onLoad(texture.get(), es);
  }

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

InternalTexturePtr
NullEngine::createRenderTargetTexture(const std::variant<ISize, float>& size,
                                      const IRenderTargetOptions& options)
{
  RenderTargetCreationOptions fullOptions;

  {
    fullOptions.generateMipMaps = options.generateMipMaps;
    fullOptions.generateDepthBuffer
      = options.generateDepthBuffer.value_or(true);
    fullOptions.generateStencilBuffer
      = fullOptions.generateDepthBuffer.value()
        && options.generateStencilBuffer.value_or(false);
    fullOptions.type
      = options.type.value_or(Constants::TEXTURETYPE_UNSIGNED_INT);
    fullOptions.samplingMode = options.samplingMode.value_or(
      Constants::TEXTURE_TRILINEAR_SAMPLINGMODE);
  }

  auto texture = std::make_shared<InternalTexture>(
    this, InternalTexture::DATASOURCE_RENDERTARGET);

  int width = 0, height = 0;
  if (std::holds_alternative<ISize>(size)) {
    auto textureSize = std::get<ISize>(size);
    width            = textureSize.width;
    height           = textureSize.height;
  }
  else if (std::holds_alternative<float>(size)) {
    auto textureSize = std::get<float>(size);
    width            = static_cast<int>(textureSize);
    height           = static_cast<int>(textureSize);
  }

  texture->_depthStencilBuffer  = std::make_shared<GL::IGLRenderbuffer>(0);
  texture->_framebuffer         = std::make_shared<GL::IGLFramebuffer>(0);
  texture->baseWidth            = width;
  texture->baseHeight           = height;
  texture->width                = width;
  texture->height               = height;
  texture->isReady              = true;
  texture->samples              = 1;
  texture->generateMipMaps      = fullOptions.generateMipMaps.value_or(false);
  texture->samplingMode         = *fullOptions.samplingMode;
  texture->type                 = *fullOptions.type;
  texture->_generateDepthBuffer = *fullOptions.generateDepthBuffer;
  texture->_generateStencilBuffer
    = fullOptions.generateStencilBuffer.value_or(false);

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

void NullEngine::updateTextureSamplingMode(unsigned int samplingMode,
                                           const InternalTexturePtr& texture)
{
  texture->samplingMode = samplingMode;
}

void NullEngine::bindFramebuffer(const InternalTexturePtr& texture,
                                 std::optional<unsigned int> /*faceIndex*/,
                                 std::optional<int> requiredWidth,
                                 std::optional<int> requiredHeight,
                                 std::optional<bool> forceFullscreenViewport,
                                 InternalTexture* /*depthStencilTexture*/,
                                 int /*lodLevel*/)
{
  if (_currentRenderTarget) {
    unBindFramebuffer(_currentRenderTarget);
  }
  _currentRenderTarget = texture;
  _currentFramebuffer  = texture->_MSAAFramebuffer ? texture->_MSAAFramebuffer :
                                                    texture->_framebuffer;
  if (_cachedViewport && !forceFullscreenViewport) {
    setViewport(*_cachedViewport, requiredWidth, requiredHeight);
  }
}

void NullEngine::unBindFramebuffer(const InternalTexturePtr& texture,
                                   bool /*disableGenerateMipMaps*/,
                                   const std::function<void()>& onBeforeUnbind)
{
  _currentRenderTarget = nullptr;

  if (onBeforeUnbind) {
    if (texture->_MSAAFramebuffer) {
      _currentFramebuffer = texture->_framebuffer;
    }
    onBeforeUnbind();
  }
  _currentFramebuffer = nullptr;
}

Engine::GLBufferPtr
NullEngine::createDynamicVertexBuffer(const Float32Array& /*vertices*/)
{
  auto buffer        = std::make_unique<GL::IGLBuffer>(0);
  buffer->references = 1;
  buffer->capacity   = 1;
  return buffer;
}

void NullEngine::updateDynamicTexture(const InternalTexturePtr& /*texture*/,
                                      ICanvas* /*canvas*/, bool /*invertY*/,
                                      bool /*premulAlpha*/,
                                      unsigned int /*format*/)
{
}

bool NullEngine::areAllEffectsReady() const
{
  return true;
}

unsigned int NullEngine::getError() const
{
  return 0;
}

int NullEngine::_getUnpackAlignement()
{
  return 1;
}

void NullEngine::_unpackFlipY(bool /*value*/)
{
}

void NullEngine::updateDynamicIndexBuffer(const GLBufferPtr& /*indexBuffer*/,
                                          const IndicesArray& /*indices*/,
                                          int /*offset*/)
{
}

void NullEngine::updateDynamicVertexBuffer(const GLBufferPtr& /*vertexBuffer*/,
                                           const Float32Array& /*data*/,
                                           int /*byteOffset*/,
                                           int /*byteLength*/)
{
}

bool NullEngine::_bindTextureDirectly(unsigned int /*target*/,
                                      const InternalTexturePtr& texture,
                                      bool /*forTextureDataUpdate*/,
                                      bool /*force*/)
{
  if (stl_util::contains(_boundTexturesCache, _activeChannel)
      && _boundTexturesCache[_activeChannel] != texture) {
    _boundTexturesCache[_activeChannel] = texture;
    return true;
  }
  return false;
}

void NullEngine::_bindTexture(int channel, const InternalTexturePtr& texture)
{
  if (channel < 0) {
    return;
  }

  _bindTextureDirectly(0, texture);
}

bool NullEngine::_releaseBuffer(GL::IGLBuffer* buffer)
{
  --buffer->references;

  if (buffer->references == 0) {
    return true;
  }

  return false;
}

void NullEngine::releaseEffects()
{
}

void NullEngine::displayLoadingUI()
{
}

void NullEngine::hideLoadingUI()
{
}

void NullEngine::_uploadCompressedDataToTextureDirectly(
  const InternalTexturePtr& /*texture*/, unsigned int /*internalFormat*/,
  float /*width*/, float /*height*/, const Uint8Array& /*data*/,
  unsigned int /*faceIndex*/, int /*lod*/)
{
}

void NullEngine::_uploadDataToTextureDirectly(
  const InternalTexturePtr& /*texture*/, const ArrayBufferView& /*imageData*/,
  unsigned int /*faceIndex*/, int /*lod*/)
{
}

void NullEngine::_uploadArrayBufferViewToTexture(
  const InternalTexturePtr& /*texture*/, const Uint8Array& /*imageData*/,
  unsigned int /*faceIndex*/, int /*lod*/)
{
}

void NullEngine::_uploadImageToTexture(const InternalTexturePtr& /*texture*/,
                                       const Image& /*image*/,
                                       unsigned int /*faceIndex*/, int /*lod*/)
{
}

} // end of namespace BABYLON
