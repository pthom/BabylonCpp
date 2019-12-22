﻿#include <babylon/engines/engine.h>

#include <future>

#include <babylon/asio/asio.h>
#include <babylon/babylon_stl_util.h>
#include <babylon/babylon_version.h>
#include <babylon/cameras/camera.h>
#include <babylon/core/array_buffer_view.h>
#include <babylon/core/delegates/delegate.h>
#include <babylon/core/logging.h>
#include <babylon/core/string.h>
#include <babylon/core/time.h>
#include <babylon/engines/depth_texture_creation_options.h>
#include <babylon/engines/instancing_attribute_info.h>
#include <babylon/engines/scene.h>
#include <babylon/engines/webgl/webgl_pipeline_context.h>
#include <babylon/interfaces/icanvas.h>
#include <babylon/interfaces/icanvas_rendering_context2D.h>
#include <babylon/interfaces/igl_rendering_context.h>
#include <babylon/interfaces/iloading_screen.h>
#include <babylon/materials/effect.h>
#include <babylon/materials/effect_creation_options.h>
#include <babylon/materials/effect_fallbacks.h>
#include <babylon/materials/material.h>
#include <babylon/materials/textures/dummy_internal_texture_tracker.h>
#include <babylon/materials/textures/iinternal_texture_loader.h>
#include <babylon/materials/textures/iinternal_texture_tracker.h>
#include <babylon/materials/textures/imulti_render_target_options.h>
#include <babylon/materials/textures/internal_texture.h>
#include <babylon/materials/textures/irender_target_options.h>
#include <babylon/materials/textures/loaders/dds_texture_loader.h>
#include <babylon/materials/textures/loaders/env_texture_loader.h>
#include <babylon/materials/textures/loaders/ktx_texture_loader.h>
#include <babylon/materials/textures/loaders/tga_texture_loader.h>
#include <babylon/materials/textures/render_target_texture.h>
#include <babylon/materials/textures/texture.h>
#include <babylon/materials/uniform_buffer.h>
#include <babylon/maths/color3.h>
#include <babylon/maths/color4.h>
#include <babylon/maths/scalar.h>
#include <babylon/meshes/abstract_mesh.h>
#include <babylon/meshes/vertex_buffer.h>
#include <babylon/misc/dds.h>
#include <babylon/misc/file_tools.h>
#include <babylon/misc/tools.h>
#include <babylon/particles/particle_system.h>
#include <babylon/postprocesses/pass_post_process.h>
#include <babylon/postprocesses/post_process.h>
#include <babylon/postprocesses/post_process_manager.h>
#include <babylon/shaders/shadersinclude/glsl_version_3.h>
#include <babylon/states/_alpha_state.h>
#include <babylon/states/_depth_culling_state.h>
#include <babylon/states/_stencil_state.h>

namespace BABYLON {

// Register the loaders
std::vector<IInternalTextureLoaderPtr> Engine::_TextureLoaders = {
  std::make_shared<_DDSTextureLoader>(), // DDS Texture Loader
  std::make_shared<_ENVTextureLoader>(), // ENV Texture Loader
  std::make_shared<_KTXTextureLoader>(), // KTX Texture Loader
  std::make_shared<_TGATextureLoader>()  // TGA Texture Loader
};

std::string Engine::Version()
{
  return BABYLONCPP_VERSION;
}

AudioEnginePtr Engine::AudioEngine()
{
  return _audioEngine;
}

AudioEnginePtr Engine::_audioEngine   = nullptr;
float Engine::CollisionsEpsilon       = 0.001f;
std::string Engine::CodeRepository    = "src/";
std::string Engine::ShadersRepository = "src/shaders/";
std::vector<Engine*> Engine::Instances{};

Engine::Engine(ICanvas* canvas, const EngineOptions& options)
    : forcePOTTextures{false}
    , isFullscreen{false}
    , isPointerLock{false}
    , cullBackFaces{true}
    , renderEvenInBackground{true}
    , preventCacheWipeBetweenFrames{false}
    , enableOfflineSupport{false}
    , disableManifestCheck{false}
    , validateShaderPrograms{false}
    , _badOS{false}
    , _badDesktopOS{false}
    , disableTextureBindingOptimization{false}
    , _vrDisplayEnabled{false}
    , disableUniformBuffers{false}
    , _gl{nullptr}
    , disablePerformanceMonitorInBackground{false}
    , premultipliedAlpha{options.premultipliedAlpha}
    , enableUnpackFlipYCached{true}
    , _depthCullingState{std::make_unique<_DepthCullingState>()}
    , _stencilState{std::make_unique<_StencilState>()}
    , _alphaState{std::make_unique<_AlphaState>()}
    , _alphaMode{Constants::ALPHA_DISABLE}
    , _activeChannel{0}
    , _currentEffect{nullptr}
    , _currentProgram{nullptr}
    , _cachedViewport{std::nullopt}
    , _cachedVertexBuffers{nullptr}
    , _cachedIndexBuffer{nullptr}
    , _cachedEffectForVertexBuffers{nullptr}
    , _currentRenderTarget{nullptr}
    , _currentFramebuffer{nullptr}
    , _vrExclusivePointerMode{false}
    , _renderingCanvas{canvas}
    , _windowIsBackground{false}
    , _webGLVersion{1.f}
    , _colorWrite{true}
    , _loadingScreen{nullptr}
    , _videoTextureSupported{false}
    , _renderingQueueLaunched{false}
    , _deterministicLockstep{false}
    , _lockstepMaxSteps{4}
    , _contextWasLost{false}
    , _doNotHandleContextLost{options.doNotHandleContextLost}
    , _performanceMonitor{std::make_unique<PerformanceMonitor>()}
    , _fps{60.f}
    , _deltaTime{0.f}
    , _currentTextureChannel{-1}
    , _currentNonTimestampToken{std::nullopt}
    , _cachedVertexArrayObject{nullptr}
    , _uintIndicesCurrentlySet{false}
    , _firstBoundInternalTextureTracker{std::make_unique<DummyInternalTextureTracker>()}
    , _lastBoundInternalTextureTracker{std::make_unique<DummyInternalTextureTracker>()}
    , _workingCanvas{nullptr}
    , _workingContext{nullptr}
    , _rescalePostProcess{nullptr}
    , _dummyFramebuffer{nullptr}
    , _bindedRenderFunction{nullptr}
    , _vaoRecordInProgress{false}
    , _mustWipeVertexAttributes{false}
    , _emptyTexture{nullptr}
    , _emptyCubeTexture{nullptr}
    , _emptyTexture3D{nullptr}
    , _maxSimultaneousTextures{0}
    , _unpackFlipYCached{std::nullopt}
{
  Engine::Instances.emplace_back(this);

  if (!canvas) {
    return;
  }

  // GL
  if (!options.disableWebGL2Support) {
    _gl = canvas->getContext3d(options);
    if (_gl) {
      _webGLVersion = 2.f;
    }
  }

  if (!_gl) {
    if (!canvas) {
      BABYLON_LOG_ERROR("Engine", "The provided canvas is null or undefined")
      return;
    }
    _gl = canvas->getContext3d(options);
  }

  if (!_gl) {
    BABYLON_LOG_ERROR("Engine", "GL not supported")
    return;
  }

  _onCanvasFocus = [this]() { onCanvasFocusObservable.notifyObservers(this); };

  _onCanvasBlur = [this]() { onCanvasBlurObservable.notifyObservers(this); };

  _onBlur = [this]() {
    if (disablePerformanceMonitorInBackground) {
      _performanceMonitor->disable();
    }
    _windowIsBackground = true;
  };

  _onFocus = [this]() {
    if (disablePerformanceMonitorInBackground) {
      _performanceMonitor->enable();
    }
    _windowIsBackground = false;
  };

  _onCanvasPointerOut = [this]() { onCanvasPointerOutObservable.notifyObservers(this); };

  // Viewport
  _hardwareScalingLevel = options.adaptToDeviceRatio ? 1 : 1;
  resize();

  _isStencilEnable = options.stencil;
  _initGLContext();

  // if (!Engine.audioEngine) {
  //  Engine::audioEngine = new AudioEngine();
  //}

  // Prepare buffer pointers
  for (unsigned int i = 0, ul = static_cast<unsigned>(_caps.maxVertexAttribs); i < ul; ++i) {
    _currentBufferPointers[i] = BufferPointer();
  }

  _linkTrackers(_firstBoundInternalTextureTracker, _lastBoundInternalTextureTracker);

  // Load WebVR Devices
  // if (options.autoEnableWebVR) {
  //  initWebVR();
  //}

  BABYLON_LOGF_INFO("Engine", "BabylonCpp engine (v%s) launched", Engine::Version().c_str())
}

Engine::~Engine()
{
  stl_util::remove_vector_elements_equal(Engine::Instances, this);
}

Engine* Engine::LastCreatedEngine()
{
  if (Engine::Instances.empty()) {
    return nullptr;
  }

  return Engine::Instances.back();
}

Scene* Engine::LastCreatedScene()
{
  auto lastCreatedEngine = Engine::LastCreatedEngine();
  if (!lastCreatedEngine) {
    return nullptr;
  }

  if (lastCreatedEngine->scenes.empty()) {
    return nullptr;
  }

  return lastCreatedEngine->scenes.back();
}

void Engine::MarkAllMaterialsAsDirty(unsigned int flag,
                                     const std::function<bool(Material* mat)>& predicate)
{
  for (auto& engine : Engine::Instances) {
    for (auto& scene : engine->scenes) {
      scene->markAllMaterialsAsDirty(flag, predicate);
    }
  }
}

std::vector<std::string>& Engine::texturesSupported()
{
  return _texturesSupported;
}

std::string Engine::textureFormatInUse() const
{
  return _textureFormatInUse;
}

std::optional<Viewport>& Engine::currentViewport()
{
  return _cachedViewport;
}

InternalTexturePtr& Engine::emptyTexture()
{
  if (!_emptyTexture) {
    _emptyTexture = createRawTexture(Uint8Array(4), 1, 1, Constants::TEXTUREFORMAT_RGBA, false,
                                     false, Constants::TEXTURE_NEAREST_SAMPLINGMODE);
  }

  return _emptyTexture;
}

InternalTexturePtr& Engine::emptyTexture3D()
{
  if (!_emptyTexture3D) {
    Uint8Array emptyTextureBuffer(4);
    _emptyTexture3D = createRawTexture3D(
      stl_util::as_span(emptyTextureBuffer), 1, 1, 1, Constants::TEXTUREFORMAT_RGBA,
      false, false, Constants::TEXTURE_NEAREST_SAMPLINGMODE);
  }

  return _emptyTexture3D;
}

InternalTexturePtr& Engine::emptyCubeTexture()
{
  if (!_emptyCubeTexture) {
#if 0
    Uint8Array faceData(4);
    std::vector<Uint8Array> cubeData{faceData, faceData, faceData,
                                     faceData, faceData, faceData};
    _emptyCubeTexture = createRawCubeTexture(
      cubeData, 1, Constants::TEXTUREFORMAT_RGBA,
      Constants::TEXTURETYPE_UNSIGNED_INT, false, false,
      Constants::TEXTURE_NEAREST_SAMPLINGMODE);
#endif
  }

  return _emptyCubeTexture;
}

void Engine::_rebuildInternalTextures()
{
  for (auto& internalTexture : _internalTexturesCache) {
    internalTexture->_rebuild();
  }
}

void Engine::_rebuildEffects()
{
  for (auto& item : _compiledEffects) {
    item.second->_prepareEffect();
  }

  Effect::ResetCache();
}

bool Engine::areAllEffectsReady() const
{
  return true;
}

void Engine::_rebuildBuffers()
{
  // Index / Vertex
  for (auto& scene : scenes) {
    scene->resetCachedMaterial();
    scene->_rebuildGeometries();
    scene->_rebuildTextures();
  }

  // Uniforms
  for (auto& uniformBuffer : _uniformBuffers) {
    uniformBuffer->_rebuild();
  }
}

void Engine::_initGLContext()
{
  // Caps
  _caps                               = EngineCapabilities();
  _caps.maxTexturesImageUnits         = _gl->getParameteri(GL::MAX_TEXTURE_IMAGE_UNITS);
  _caps.maxCombinedTexturesImageUnits = _gl->getParameteri(GL::MAX_COMBINED_TEXTURE_IMAGE_UNITS);
  _caps.maxVertexTextureImageUnits    = _gl->getParameteri(GL::MAX_VERTEX_TEXTURE_IMAGE_UNITS);
  _caps.maxTextureSize                = _gl->getParameteri(GL::MAX_TEXTURE_SIZE);
  _caps.maxCubemapTextureSize         = _gl->getParameteri(GL::MAX_CUBE_MAP_TEXTURE_SIZE);
  _caps.maxRenderTextureSize          = _gl->getParameteri(GL::MAX_RENDERBUFFER_SIZE);
  _caps.maxVertexAttribs              = _gl->getParameteri(GL::MAX_VERTEX_ATTRIBS);

  // Those parameters cannot always be reliably queried
  // (GlGetError returns INVALID_ENUM under windows 10 (VM with parallels desktop opengl driver)
  // In case of failure, we set reasonable values
  _caps.maxVaryingVectors = _gl->getParameteri(GL::MAX_VARYING_VECTORS);
  if (_caps.maxVaryingVectors == 0) {
    BABYLON_LOGF_WARN("_initGLContext",
                      "_gl->getParameteri(GL::MAX_VARYING_VECTORS) failed "
                      "=> using %i as a default...",
                      16)
    _caps.maxVaryingVectors = 16;
  }

  _caps.maxFragmentUniformVectors = _gl->getParameteri(GL::MAX_FRAGMENT_UNIFORM_VECTORS);
  if (_caps.maxFragmentUniformVectors == 0) {
    BABYLON_LOGF_WARN("_initGLContext",
                      "_gl->getParameteri(GL::MAX_FRAGMENT_UNIFORM_VECTORS) failed => "
                      "using %i as a default...",
                      256)
    _caps.maxFragmentUniformVectors = 256;
  }

  _caps.maxVertexUniformVectors = _gl->getParameteri(GL::MAX_VERTEX_UNIFORM_VECTORS);
  if (_caps.maxVertexUniformVectors == 0) {
    BABYLON_LOGF_WARN("_initGLContext",
                      "_gl->getParameteri(GL::MAX_VERTEX_UNIFORM_VECTORS) failed => "
                      "using %i as a default...",
                      256)
    _caps.maxVertexUniformVectors = 256;
  }

  // Infos
  _glVersion  = _gl->getString(GL::VERSION);
  _glRenderer = _gl->getString(GL::RENDERER);
  _glVendor   = _gl->getString(GL::VENDOR);

  if (_glVendor.empty()) {
    _glVendor = "Unknown vendor";
  }

  if (_glRenderer.empty()) {
    _glRenderer = "Unknown renderer";
  }

  // Extensions
  auto extensionList = String::split(_gl->getString(GL::EXTENSIONS), ' ');
  std::set<std::string> extensions;
  for (auto& extension : extensionList) {
    extensions.insert(extension);
  }

  _caps.standardDerivatives = (_webGLVersion > 1.f);
  _caps.maxAnisotropy
    = static_cast<unsigned>(_gl->getParameteri(GL::MAX_TEXTURE_MAX_ANISOTROPY_EXT));
  _caps.uintIndices                  = (_webGLVersion > 1.f);
  _caps.fragmentDepthSupported       = (_webGLVersion > 1.f);
  _caps.highPrecisionShaderSupported = true;

  // Checks if some of the format renders first to allow the use of webgl
  // inspector.
  _caps.colorBufferFloat = (_webGLVersion > 1.f);

  _caps.textureFloat
    = (_webGLVersion > 1.f) || stl_util::contains(extensions, "GL_ARB_texture_float");
  _caps.textureFloatLinearFiltering = _caps.textureFloat;
  _caps.textureFloatRender          = _caps.textureFloat && _canRenderToFloatFramebuffer();

  _caps.textureHalfFloat                = (_webGLVersion > 1.f);
  _caps.textureHalfFloatLinearFiltering = (_webGLVersion > 1.f);
  if (_webGLVersion > 1) {
    _gl->HALF_FLOAT_OES = 0x140B;
  }
  _caps.textureHalfFloatRender = _caps.textureHalfFloat && _canRenderToHalfFloatFramebuffer();

  _caps.textureLOD = (_webGLVersion > 1.f);

  // Draw buffers
  if (_webGLVersion > 1.f) {
    _caps.drawBuffersExtension = true;
  }

  // Shader compiler threads
  _caps.parallelShaderCompile = std::nullopt;

  // Depth Texture
  if (_webGLVersion > 1.f) {
    _caps.depthTextureExtension = true;
  }

  // Vertex array object
  if (_webGLVersion > 1.f) {
    _caps.vertexArrayObject = true;
  }

  // Instances count
  if (_webGLVersion > 1.f) {
    _caps.instancedArrays = true;
  }

  auto highp = _gl->getShaderPrecisionFormat(GL::FRAGMENT_SHADER, GL::HIGH_FLOAT);
  if (highp) {
    _caps.highPrecisionShaderSupported = highp ? highp->precision != 0 : false;
  }

  // Depth buffer
  setDepthBuffer(true);
  setDepthFunctionToLessOrEqual();
  setDepthWrite(true);

  // Texture maps
  _maxSimultaneousTextures = static_cast<unsigned>(_caps.maxCombinedTexturesImageUnits);
  for (unsigned int slot = 0; slot < _maxSimultaneousTextures; ++slot) {
    _nextFreeTextureSlots.emplace_back(slot);
  }
}

float Engine::webGLVersion() const
{
  return _webGLVersion;
}

bool Engine::isInVRExclusivePointerMode() const
{
  return _vrExclusivePointerMode;
}

bool Engine::supportsUniformBuffers() const
{
  return webGLVersion() > 1.f && !disableUniformBuffers;
}

bool Engine::needPOTTextures() const
{
  return _webGLVersion < 2.f || forcePOTTextures;
}

bool Engine::doNotHandleContextLost() const
{
  return _doNotHandleContextLost;
}

void Engine::setDoNotHandleContextLost(bool value)
{
  _doNotHandleContextLost = value;
}

PerformanceMonitor* Engine::performanceMonitor() const
{
  return _performanceMonitor.get();
}

bool Engine::isStencilEnable() const
{
  return _isStencilEnable;
}

void Engine::_prepareWorkingCanvas()
{
  if (_workingCanvas) {
    return;
  }

  _workingCanvas = _renderingCanvas;
  auto context   = _workingCanvas->getContext2d();

  if (context) {
    _workingContext = context;
  }
}

void Engine::resetTextureCache()
{
  for (auto& boundTextureItem : _boundTexturesCache) {
    boundTextureItem.second = nullptr;
  }

  _currentTextureChannel = -1;
}

bool Engine::isDeterministicLockStep() const
{
  return _deterministicLockstep;
}

unsigned int Engine::getLockstepMaxSteps() const
{
  return _lockstepMaxSteps;
}

GL::GLInfo Engine::getGlInfo()
{
  return {
    _glVendor,   // vendor
    _glRenderer, // renderer
    _glVersion   // version
  };
}

float Engine::getAspectRatio(const Camera& camera, bool useScreen)
{
  const auto& viewport = camera.viewport;
  return static_cast<float>(getRenderWidth(useScreen) * viewport.width)
         / static_cast<float>(getRenderHeight(useScreen) * viewport.height);
}

float Engine::getScreenAspectRatio() const
{
  return static_cast<float>(getRenderWidth(true)) / static_cast<float>(getRenderHeight(true));
}

int Engine::getRenderWidth(bool useScreen) const
{
  if (!useScreen && _currentRenderTarget) {
    return _currentRenderTarget->width;
  }

  return _gl->drawingBufferWidth;
}

int Engine::getRenderHeight(bool useScreen) const
{
  if (!useScreen && _currentRenderTarget) {
    return _currentRenderTarget->height;
  }

  return _gl->drawingBufferHeight;
}

ICanvas* Engine::getRenderingCanvas()
{
  return _renderingCanvas;
}

std::optional<ClientRect> Engine::getRenderingCanvasClientRect()
{
  if (!_renderingCanvas) {
    return std::nullopt;
  }

  return _renderingCanvas->getBoundingClientRect();
}

void Engine::setHardwareScalingLevel(int level)
{
  _hardwareScalingLevel = level;
  resize();
}

int Engine::getHardwareScalingLevel() const
{
  return _hardwareScalingLevel;
}

std::vector<InternalTexturePtr>& Engine::getLoadedTexturesCache()
{
  return _internalTexturesCache;
}

EngineCapabilities& Engine::getCaps()
{
  return _caps;
}

size_t Engine::drawCalls() const
{
  BABYLON_LOG_WARN("Engine", "drawCalls is deprecated. Please use SceneInstrumentation class")
  return 0;
}

std::optional<PerfCounter> Engine::drawCallsPerfCounter()
{
  BABYLON_LOG_WARN("Engine",
                   "drawCallsPerfCounter is deprecated. Please use "
                   "SceneInstrumentation class")
  return std::nullopt;
}

// Methods
void Engine::backupGLState()
{
  _gl->backupGLState();
}

void Engine::restoreGLState()
{
  _gl->restoreGLState();
}

int Engine::getDepthFunction() const
{
  return *_depthCullingState->depthFunc();
}

void Engine::setDepthFunction(int depthFunc)
{
  _depthCullingState->depthFunc = depthFunc;
}

void Engine::setDepthFunctionToGreater()
{
  _depthCullingState->depthFunc = GL::GREATER;
}

void Engine::setDepthFunctionToGreaterOrEqual()
{
  _depthCullingState->depthFunc = GL::GEQUAL;
}

void Engine::setDepthFunctionToLess()
{
  _depthCullingState->depthFunc = GL::LESS;
}

void Engine::cacheStencilState()
{
  _cachedStencilBuffer             = getStencilBuffer();
  _cachedStencilFunction           = getStencilFunction();
  _cachedStencilMask               = getStencilMask();
  _cachedStencilOperationPass      = getStencilOperationPass();
  _cachedStencilOperationFail      = getStencilOperationFail();
  _cachedStencilOperationDepthFail = getStencilOperationDepthFail();
  _cachedStencilReference          = getStencilFunctionReference();
}

void Engine::restoreStencilState()
{
  setStencilFunction(_cachedStencilFunction);
  setStencilMask(_cachedStencilMask);
  setStencilBuffer(_cachedStencilBuffer);
  setStencilOperationPass(_cachedStencilOperationPass);
  setStencilOperationFail(_cachedStencilOperationFail);
  setStencilOperationDepthFail(_cachedStencilOperationDepthFail);
  setStencilFunctionReference(_cachedStencilReference);
}

void Engine::setDepthFunctionToLessOrEqual()
{
  _depthCullingState->depthFunc = GL::LEQUAL;
}

bool Engine::getStencilBuffer() const
{
  return _stencilState->stencilTest();
}

void Engine::setStencilBuffer(bool enable)
{
  _stencilState->stencilTest = enable;
}

unsigned int Engine::getStencilMask() const
{
  return _stencilState->stencilMask();
}

void Engine::setStencilMask(unsigned int mask)
{
  _stencilState->stencilMask = mask;
}

unsigned int Engine::getStencilFunction() const
{
  return _stencilState->stencilFunc();
}

int Engine::getStencilFunctionReference() const
{
  return _stencilState->stencilFuncRef();
}

unsigned int Engine::getStencilFunctionMask() const
{
  return _stencilState->stencilFuncMask();
}

void Engine::setStencilFunction(unsigned int stencilFunc)
{
  _stencilState->stencilFunc = stencilFunc;
}

void Engine::setStencilFunctionReference(int reference)
{
  _stencilState->stencilFuncRef = reference;
}

void Engine::setStencilFunctionMask(unsigned int mask)
{
  _stencilState->stencilFuncMask = mask;
}

unsigned int Engine::getStencilOperationFail() const
{
  return _stencilState->stencilOpStencilFail();
}

unsigned int Engine::getStencilOperationDepthFail() const
{
  return _stencilState->stencilOpDepthFail();
}

unsigned int Engine::getStencilOperationPass() const
{
  return _stencilState->stencilOpStencilDepthPass();
}

void Engine::setStencilOperationFail(unsigned int operation)
{
  _stencilState->stencilOpStencilFail = operation;
}

void Engine::setStencilOperationDepthFail(unsigned int operation)
{
  _stencilState->stencilOpDepthFail = operation;
}

void Engine::setStencilOperationPass(unsigned int operation)
{
  _stencilState->stencilOpStencilDepthPass = operation;
}

void Engine::setDitheringState(bool value)
{
  if (value) {
    _gl->enable(GL::DITHER);
  }
  else {
    _gl->disable(GL::DITHER);
  }
}

void Engine::setRasterizerState(bool value)
{
  if (value) {
    _gl->enable(GL::RASTERIZER_DISCARD);
  }
  else {
    _gl->disable(GL::RASTERIZER_DISCARD);
  }
}

void Engine::stopRenderLoop()
{
  _activeRenderLoops.clear();
}

void Engine::stopRenderLoop(const SA::delegate<void()>& renderFunction)
{
  _activeRenderLoops.erase(
    std::remove(_activeRenderLoops.begin(), _activeRenderLoops.end(), renderFunction),
    _activeRenderLoops.end());
}

void Engine::_renderLoop()
{
  if (!_contextWasLost) {
    auto shouldRender = true;
    if (!renderEvenInBackground && _windowIsBackground) {
      shouldRender = false;
    }

    if (shouldRender) {
      // Start new frame
      beginFrame();

      for (auto& renderFunction : _activeRenderLoops) {
        renderFunction();
      }

      // Present
      endFrame();
    }
  }

  if (!_activeRenderLoops.empty()) {
    // Register new frame
  }
  else {
    _renderingQueueLaunched = false;
  }
}

void Engine::runRenderLoop(const std::function<void()>& renderFunction)
{
  if (std::find(_activeRenderLoops.begin(), _activeRenderLoops.end(), renderFunction)
      != _activeRenderLoops.end()) {
    return;
  }

  _activeRenderLoops.emplace_back(renderFunction);

  if (!_renderingQueueLaunched) {
    _renderingQueueLaunched = true;
  }
}

void Engine::renderFunction(const std::function<void()>& renderFunction)
{
  bool shouldRender = true;
  if (!renderEvenInBackground && _windowIsBackground) {
    shouldRender = false;
  }

  if (shouldRender) {
    // Start new frame
    beginFrame();
    // Render
    renderFunction();
    // Present
    endFrame();
  }
}

void Engine::switchFullscreen(bool requestPointerLock)
{
  if (isFullscreen) {
    Tools::ExitFullscreen();
  }
  else {
    _pointerLockRequested = requestPointerLock;
    if (_renderingCanvas) {
      Tools::RequestFullscreen(_renderingCanvas);
    }
  }
}

void Engine::clear(bool depth, bool stencil)
{
  applyStates();

  unsigned int mode = 0;
  if (depth) {
    _gl->clearDepth(1.f);
    mode |= GL::DEPTH_BUFFER_BIT;
  }

  if (stencil) {
    _gl->clearStencil(0);
    mode |= GL::STENCIL_BUFFER_BIT;
  }

  _gl->clear(mode);
}

void Engine::clear(const Color4& color, bool backBuffer, bool depth, bool stencil)
{
  applyStates();

  unsigned int mode = 0;
  if (backBuffer) {
    _gl->clearColor(color.r, color.g, color.b, color.a);
    mode |= GL::COLOR_BUFFER_BIT;
  }

  if (depth) {
    _gl->clearDepth(1.f);
    mode |= GL::DEPTH_BUFFER_BIT;
  }

  if (stencil) {
    _gl->clearStencil(0);
    mode |= GL::STENCIL_BUFFER_BIT;
  }

  _gl->clear(mode);
}

void Engine::scissorClear(int x, int y, int width, int height, const Color4& clearColor)
{
  // Save state
  auto curScissor    = _gl->getParameteri(GL::SCISSOR_TEST);
  auto curScissorBox = _gl->getScissorBoxParameter();

  // Change state
  _gl->enable(GL::SCISSOR_TEST);
  _gl->scissor(x, y, width, height);

  // Clear
  clear(clearColor, true, true, true);

  // Restore state
  _gl->scissor(curScissorBox[0], curScissorBox[1], curScissorBox[2], curScissorBox[3]);

  if (curScissor == 1) {
    _gl->enable(GL::SCISSOR_TEST);
  }
  else {
    _gl->disable(GL::SCISSOR_TEST);
  }
}

void Engine::_viewport(float x, float y, float width, float height)
{
  if (!stl_util::almost_equal(x, _viewportCached.x) || !stl_util::almost_equal(y, _viewportCached.y)
      || !stl_util::almost_equal(width, _viewportCached.z)
      || !stl_util::almost_equal(height, _viewportCached.w)) {
    _viewportCached.x = x;
    _viewportCached.y = y;
    _viewportCached.z = width;
    _viewportCached.w = height;

    _gl->viewport(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width),
                  static_cast<int>(height));
  }
}

void Engine::setViewport(Viewport& viewport, const std::optional<int>& requiredWidth,
                         const std::optional<int>& requiredHeight)
{

  auto width  = requiredWidth.value_or(getRenderWidth());
  auto height = requiredHeight.value_or(getRenderHeight());
  auto x      = viewport.x;
  auto y      = viewport.y;

  _cachedViewport = viewport;

  _viewport(x * width, y * height, width * viewport.width, height * viewport.height);
}

Viewport Engine::setDirectViewport(int x, int y, int width, int height)
{
  auto currentViewport = _cachedViewport;
  _cachedViewport      = std::nullopt;

  _viewport(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width),
            static_cast<float>(height));

  return *currentViewport;
}

void Engine::beginFrame()
{
  onBeginFrameObservable.notifyObservers(this);
  _measureFps();
}

void Engine::endFrame()
{
  // Force a flush in case we are using a bad OS.
  if (_badOS) {
    flushFramebuffer();
  }

  // Submit frame to the vr device, if enabled
  // if (_vrDisplayEnabled && _vrDisplayEnabled.isPresenting) {
  //  _vrDisplayEnabled.submitFrame()
  //}

  onEndFrameObservable.notifyObservers(this);
}

void Engine::resize()
{
  // We're not resizing the size of the canvas while in VR mode & presenting
  if (!_vrDisplayEnabled) {
    const int width  = _renderingCanvas ? _renderingCanvas->clientWidth : 0;
    const int height = _renderingCanvas ? _renderingCanvas->clientHeight : 0;
    setSize(width / _hardwareScalingLevel, height / _hardwareScalingLevel);
  }
}

void Engine::setSize(int width, int height)
{
  if (!_renderingCanvas) {
    return;
  }

  if (_renderingCanvas->width == width && _renderingCanvas->height == height) {
    return;
  }

  _renderingCanvas->width  = width;
  _renderingCanvas->height = height;

  for (auto& scene : scenes) {
    for (auto& cam : scene->cameras) {
      cam->_currentRenderId = 0;
    }
  }

  if (onResizeObservable.hasObservers()) {
    onResizeObservable.notifyObservers(this);
  }
}

void Engine::_onVRFullScreenTriggered()
{
}

void Engine::_getVRDisplays()
{
}

void Engine::bindFramebuffer(const InternalTexturePtr& texture,
                             std::optional<unsigned int> faceIndex,
                             std::optional<int> requiredWidth, std::optional<int> requiredHeight,
                             std::optional<bool> forceFullscreenViewport,
                             InternalTexture* depthStencilTexture, int lodLevel)
{
  if (_currentRenderTarget) {
    unBindFramebuffer(_currentRenderTarget);
  }
  _currentRenderTarget = texture;
  bindUnboundFramebuffer(texture->_MSAAFramebuffer ? texture->_MSAAFramebuffer :
                                                     texture->_framebuffer);
  if (texture->isCube) {
    if (!faceIndex.has_value()) {
      faceIndex = 0u;
    }
    _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0,
                              GL::TEXTURE_CUBE_MAP_POSITIVE_X + (*faceIndex),
                              texture->_webGLTexture.get(), lodLevel);

    if (depthStencilTexture) {
      if (depthStencilTexture->_generateStencilBuffer) {
        _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_STENCIL_ATTACHMENT,
                                  GL::TEXTURE_CUBE_MAP_POSITIVE_X + *faceIndex,
                                  depthStencilTexture->_webGLTexture.get(), lodLevel);
      }
      else {
        _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_ATTACHMENT,
                                  GL::TEXTURE_CUBE_MAP_POSITIVE_X + *faceIndex,
                                  depthStencilTexture->_webGLTexture.get(), lodLevel);
      }
    }
  }

  if (_cachedViewport && !forceFullscreenViewport && requiredHeight.has_value()) {
    setViewport(*_cachedViewport, *requiredWidth, *requiredHeight);
  }
  else {
    if (!requiredWidth) {
      requiredWidth = texture->width;
      if (lodLevel) {
        requiredWidth = static_cast<int>(*requiredWidth / std::pow(2, lodLevel));
      }
    }
    if (!requiredHeight) {
      requiredHeight = texture->height;
      if (lodLevel) {
        requiredHeight = static_cast<int>(*requiredHeight / std::pow(2, lodLevel));
      }
    }

    _viewport(0.f, 0.f, static_cast<float>(*requiredWidth), static_cast<float>(*requiredHeight));
  }

  wipeCaches();
}

void Engine::bindUnboundFramebuffer(const GL::IGLFramebufferPtr& framebuffer)
{
  if (_currentFramebuffer == framebuffer)
    return;

  std::shared_ptr<GL::IGLFramebuffer> realFrameBuffer;
  if (framebuffer) {
    realFrameBuffer      = framebuffer;
    auto previousId      = _gl->getParameteri(GL::FRAMEBUFFER_BINDING);
    _previousFrameBuffer = std::make_unique<GL::IGLFramebuffer>(previousId);
  }
  else {
    if (_previousFrameBuffer) {
      realFrameBuffer = std::move(_previousFrameBuffer);
      _previousFrameBuffer.reset();
    }
  }

  _gl->bindFramebuffer(GL::FRAMEBUFFER, realFrameBuffer.get());
  _currentFramebuffer = framebuffer;
}

void Engine::unBindFramebuffer(const InternalTexturePtr& texture, bool disableGenerateMipMaps,
                               const std::function<void()>& onBeforeUnbind)
{
  // If MSAA, we need to bitblt back to main texture
  if (texture->_MSAAFramebuffer) {
    _gl->bindFramebuffer(GL::READ_FRAMEBUFFER, texture->_MSAAFramebuffer.get());
    _gl->bindFramebuffer(GL::DRAW_FRAMEBUFFER, texture->_framebuffer.get());
    _gl->blitFramebuffer(0, 0, texture->width, texture->height, 0, 0, texture->width,
                         texture->height, GL::COLOR_BUFFER_BIT, GL::NEAREST);
  }

  if (texture->generateMipMaps && !disableGenerateMipMaps && !texture->isCube) {
    _bindTextureDirectly(GL::TEXTURE_2D, texture, true);
    _gl->generateMipmap(GL::TEXTURE_2D);
    _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  }

  if (onBeforeUnbind) {
    if (texture->_MSAAFramebuffer) {
      // Bind the correct framebuffer
      bindUnboundFramebuffer(texture->_framebuffer);
    }
    onBeforeUnbind();
  }

  _currentRenderTarget = nullptr;

  bindUnboundFramebuffer(nullptr);
}

void Engine::unBindMultiColorAttachmentFramebuffer(const std::vector<InternalTexturePtr>& textures,
                                                   bool disableGenerateMipMaps,
                                                   const std::function<void()>& onBeforeUnbind)
{
  _currentRenderTarget = nullptr;

  // If MSAA, we need to bitblt back to main texture
  if (textures[0]->_MSAAFramebuffer) {
    _gl->bindFramebuffer(GL::READ_FRAMEBUFFER, textures[0]->_MSAAFramebuffer.get());
    _gl->bindFramebuffer(GL::DRAW_FRAMEBUFFER, textures[0]->_framebuffer.get());

    auto& attachments = textures[0]->_attachments;
    if (attachments.empty()) {
      attachments               = Uint32Array(textures.size());
      textures[0]->_attachments = attachments;
    }

    for (size_t i = 0; i < textures.size(); ++i) {
      const auto iStr = std::to_string(i);
      auto& texture   = textures[i];

      for (size_t j = 0; j < attachments.size(); ++j) {
        attachments[j] = GL::NONE;
      }

      attachments[i] = (*_gl)[webGLVersion() > 1.f ? "COLOR_ATTACHMENT" + iStr :
                                                 "COLOR_ATTACHMENT" + iStr + "_WEBGL"];
      _gl->readBuffer(attachments[i]);
      _gl->drawBuffers(attachments);
      _gl->blitFramebuffer(0, 0, texture->width, texture->height, 0, 0, texture->width,
                         texture->height, GL::COLOR_BUFFER_BIT, GL::NEAREST);
    }
    for (size_t i = 0; i < attachments.size(); i++) {
      const auto iStr = std::to_string(i);
      attachments[i]  = (*_gl)[webGLVersion() > 1.f ? "COLOR_ATTACHMENT" + iStr :
                                                 "COLOR_ATTACHMENT" + iStr + "_WEBGL"];
    }
    _gl->drawBuffers(attachments);
  }

  for (const auto& texture : textures) {
    if (texture->generateMipMaps && !disableGenerateMipMaps && !texture->isCube) {
      _bindTextureDirectly(GL::TEXTURE_2D, texture);
      _gl->generateMipmap(GL::TEXTURE_2D);
      _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
    }
  }

  if (onBeforeUnbind) {
    if (textures[0]->_MSAAFramebuffer) {
      // Bind the correct framebuffer
      bindUnboundFramebuffer(textures[0]->_framebuffer);
    }
    onBeforeUnbind();
  }

  bindUnboundFramebuffer(nullptr);
}

void Engine::generateMipMapsForCubemap(const InternalTexturePtr& texture)
{
  if (texture->generateMipMaps) {
    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);
    _gl->generateMipmap(GL::TEXTURE_CUBE_MAP);
    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
  }
}

void Engine::flushFramebuffer()
{
  _gl->flush();
}

void Engine::restoreDefaultFramebuffer()
{
  if (_currentRenderTarget) {
    unBindFramebuffer(_currentRenderTarget);
  }
  else {
    bindUnboundFramebuffer(nullptr);
  }

  if (_cachedViewport) {
    setViewport(*_cachedViewport);
  }

  wipeCaches();
}

// UBOs
std::unique_ptr<GL::IGLBuffer> Engine::createUniformBuffer(const Float32Array& elements)
{
  auto ubo = _gl->createBuffer();
  if (!ubo) {
    BABYLON_LOG_ERROR("Engine", "Unable to create uniform buffer")
    return nullptr;
  }

  bindUniformBuffer(ubo.get());

  _gl->bufferData(GL::UNIFORM_BUFFER, stl_util::as_span(elements), GL::STATIC_DRAW);

  bindUniformBuffer(nullptr);

  ubo->references = 1;
  return ubo;
}

std::unique_ptr<GL::IGLBuffer> Engine::createDynamicUniformBuffer(const Float32Array& elements)
{
  auto ubo = _gl->createBuffer();
  if (!ubo) {
    BABYLON_LOG_ERROR("Engine", "Unable to create dynamic uniform buffer")
    return nullptr;
  }

  bindUniformBuffer(ubo.get());

  _gl->bufferData(GL::UNIFORM_BUFFER, stl_util::as_span(elements), GL::DYNAMIC_DRAW);

  bindUniformBuffer(nullptr);

  ubo->references = 1;
  return ubo;
}

void Engine::updateUniformBuffer(GL::IGLBuffer* uniformBuffer, const Float32Array& elements,
                                 int offset, int count)
{
  bindUniformBuffer(uniformBuffer);

  if (offset == -1) {
    offset = 0;
  }

  if (count == -1) {
    _gl->bufferSubData(GL::UNIFORM_BUFFER, offset, stl_util::as_span(elements));
  }
  else {
    Float32Array subvector;
    std::copy(elements.begin() + offset, elements.begin() + offset + count,
              std::back_inserter(subvector));
    _gl->bufferSubData(GL::UNIFORM_BUFFER, 0, subvector);
  }

  bindUniformBuffer(nullptr);
}

// VBOs
void Engine::_resetVertexBufferBinding()
{
  bindArrayBuffer(nullptr);
  _cachedVertexBuffers = nullptr;
}

Engine::GLBufferPtr Engine::createVertexBuffer(const Float32Array& vertices)
{
  auto vbo = _gl->createBuffer();
  if (!vbo) {
    BABYLON_LOG_ERROR("Engine", "Unable to create vertex buffer")
    return nullptr;
  }

  bindArrayBuffer(vbo.get());

  _gl->bufferData(GL::ARRAY_BUFFER, stl_util::as_span(vertices), GL::STATIC_DRAW);

  _resetVertexBufferBinding();
  vbo->references = 1;
  return vbo;
}

Engine::GLBufferPtr Engine::createDynamicVertexBuffer(const Float32Array& vertices)
{
  auto vbo = _gl->createBuffer();
  if (!vbo) {
    BABYLON_LOG_ERROR("Engine", "Unable to create dynamic vertex buffer")
    return nullptr;
  }

  bindArrayBuffer(vbo.get());

  _gl->bufferData(GL::ARRAY_BUFFER, stl_util::as_span(vertices), GL::DYNAMIC_DRAW);

  _resetVertexBufferBinding();
  vbo->references = 1;
  return vbo;
}

void Engine::updateDynamicIndexBuffer(const GLBufferPtr& indexBuffer, const IndicesArray& indices,
                                      int /*offset*/)
{
  // Force cache update
  _currentBoundBuffer[GL::ELEMENT_ARRAY_BUFFER] = nullptr;
  bindIndexBuffer(indexBuffer.get());
  auto arrayBuffer = indices;

  _gl->bufferData(GL::ELEMENT_ARRAY_BUFFER, arrayBuffer, GL::DYNAMIC_DRAW);

  _resetIndexBufferBinding();
}

void Engine::updateDynamicVertexBuffer(const Engine::GLBufferPtr& vertexBuffer,
                                       const Float32Array& data, int byteOffset, int byteLength)
{
  bindArrayBuffer(vertexBuffer.get());

  if (byteOffset == -1) {
    byteOffset = 0;
  }

  if (byteLength == -1) {
    _gl->bufferSubData(GL::ARRAY_BUFFER, byteOffset, stl_util::as_span(data));
  }
  else {
    //auto byteArray = stl_util::to_array<uint8_t>(data, static_cast<size_t>(byteOffset),
    //                                             static_cast<size_t>(byteLength));
    //_gl->bufferSubData(GL::ARRAY_BUFFER, 0, byteArray);
    size_t floatOffset = byteOffset / sizeof(float);
    size_t floatLength = byteLength / sizeof(float);
    nonstd::span<const float> dataAsSpan = nonstd::span(data.data() + floatOffset, floatLength);
    _gl->bufferSubData(GL::ARRAY_BUFFER, 0, dataAsSpan);
  }

  _resetVertexBufferBinding();
}

void Engine::_resetIndexBufferBinding()
{
  bindIndexBuffer(nullptr);
  _cachedIndexBuffer = nullptr;
}

Engine::GLBufferPtr Engine::createIndexBuffer(const IndicesArray& indices, bool updatable)
{
  auto vbo = _gl->createBuffer();
  if (!vbo) {
    BABYLON_LOG_ERROR("Engine", "Unable to create index buffer")
    return nullptr;
  }

  bindIndexBuffer(vbo.get());

  // Check for 32 bits indices
  auto need32Bits = false;

  if (_caps.uintIndices) {
    // check 32 bit support
    using namespace std::placeholders;
    auto it = std::find_if(indices.begin(), indices.end(),
                           std::bind(std::greater<uint32_t>(), _1, 65535));
    if (it != indices.end()) {
      need32Bits = true;
    }
  }

  if (need32Bits) {
    Uint32Array arrayBuffer;
    arrayBuffer.assign(indices.begin(), indices.end());
    _gl->bufferData(GL::ELEMENT_ARRAY_BUFFER, arrayBuffer,
                    updatable ? GL::DYNAMIC_DRAW : GL::STATIC_DRAW);
  }
  else {
    Uint16Array arrayBuffer;
    for (auto v : indices)
      arrayBuffer.push_back(static_cast<uint16_t>(v));

    _gl->bufferData(GL::ELEMENT_ARRAY_BUFFER, arrayBuffer,
                    updatable ? GL::DYNAMIC_DRAW : GL::STATIC_DRAW);
  }

  _resetIndexBufferBinding();
  vbo->references = 1;
  vbo->is32Bits   = need32Bits;
  return vbo;
}

void Engine::bindArrayBuffer(GL::IGLBuffer* buffer)
{
  if (!_vaoRecordInProgress) {
    _unbindVertexArrayObject();
  }
  bindBuffer(buffer, GL::ARRAY_BUFFER);
}

void Engine::bindUniformBuffer(GL::IGLBuffer* buffer)
{
  _gl->bindBuffer(GL::UNIFORM_BUFFER, buffer);
}

void Engine::bindUniformBufferBase(GL::IGLBuffer* buffer, unsigned int location)
{
  _gl->bindBufferBase(GL::UNIFORM_BUFFER, location, buffer);
}

void Engine::bindUniformBlock(GL::IGLProgram* shaderProgram, const std::string& blockName,
                              unsigned int index)
{
  auto uniformLocation = _gl->getUniformBlockIndex(shaderProgram, blockName);
  _gl->uniformBlockBinding(shaderProgram, uniformLocation, index);
}

void Engine::bindIndexBuffer(GL::IGLBuffer* buffer)
{
  if (!_vaoRecordInProgress) {
    _unbindVertexArrayObject();
  }
  bindBuffer(buffer, GL::ELEMENT_ARRAY_BUFFER);
}

void Engine::bindBuffer(GL::IGLBuffer* buffer, int target)
{
  if (_vaoRecordInProgress || (_currentBoundBuffer.find(target) == _currentBoundBuffer.end())
      || (_currentBoundBuffer[target] != buffer)) {
    _gl->bindBuffer(static_cast<unsigned int>(target), buffer);
    _currentBoundBuffer[target] = buffer;
  }
}

void Engine::updateArrayBuffer(const Float32Array& data)
{
  _gl->bufferSubData(GL::ARRAY_BUFFER, 0, stl_util::as_span(data));
}

void Engine::_vertexAttribPointer(GL::IGLBuffer* buffer, unsigned int indx, int size,
                                  unsigned int type, bool normalized, int stride, int offset)
{
  bool changed = false;
  if (_currentBufferPointers.find(indx) == _currentBufferPointers.end()) {
    changed                      = true;
    _currentBufferPointers[indx] = BufferPointer(true,       // active
                                                 indx,       // indx
                                                 size,       // size
                                                 type,       // type
                                                 normalized, // normalized
                                                 stride,     // stride
                                                 offset,     // offset
                                                 buffer      // buffer
    );
  }
  else {
    auto& pointer = _currentBufferPointers[indx];
    if (pointer.buffer != buffer) {
      pointer.buffer = buffer;
      changed        = true;
    }
    if (pointer.size != size) {
      pointer.size = size;
      changed      = true;
    }
    if (pointer.type != type) {
      pointer.type = type;
      changed      = true;
    }
    if (pointer.normalized != normalized) {
      pointer.normalized = normalized;
      changed            = true;
    }
    if (pointer.stride != stride) {
      pointer.stride = stride;
      changed        = true;
    }
    if (pointer.offset != offset) {
      pointer.offset = offset;
      changed        = true;
    }
  }

  if (changed || _vaoRecordInProgress) {
    bindArrayBuffer(buffer);
    _gl->vertexAttribPointer(indx, size, type, normalized, stride, offset);
  }
}

void Engine::_bindIndexBufferWithCache(GL::IGLBuffer* indexBuffer)
{
  if (indexBuffer == nullptr) {
    return;
  }
  if (_cachedIndexBuffer != indexBuffer) {
    _cachedIndexBuffer = indexBuffer;
    bindIndexBuffer(indexBuffer);
    _uintIndicesCurrentlySet = indexBuffer->is32Bits;
  }
}

void Engine::_bindVertexBuffersAttributes(
  const std::unordered_map<std::string, VertexBufferPtr>& vertexBuffers, const EffectPtr& effect)
{
  auto attributes = effect->getAttributesNames();

  if (!_vaoRecordInProgress) {
    _unbindVertexArrayObject();
  }

  unbindAllAttributes();

  unsigned int _order = 0;
  for (unsigned int index = 0; index < attributes.size(); ++index) {
    auto order = effect->getAttributeLocation(index);

    if (order >= 0) {
      _order             = static_cast<unsigned int>(order);
      auto& vertexBuffer = vertexBuffers.at(attributes[index]);

      if (!vertexBuffer) {
        continue;
      }

      _gl->enableVertexAttribArray(_order);
      if (!_vaoRecordInProgress) {
        if (_order >= _vertexAttribArraysEnabled.size()) {
          _vertexAttribArraysEnabled.resize(_order + 1);
        }
        _vertexAttribArraysEnabled[_order] = true;
      }

      auto buffer = vertexBuffer->getBuffer();
      if (buffer) {
        _vertexAttribPointer(buffer, _order, static_cast<int>(vertexBuffer->getSize()), GL::FLOAT,
                             false, static_cast<int>(vertexBuffer->getStrideSize() * 4),
                             static_cast<int>(vertexBuffer->getOffset() * 4));

        if (vertexBuffer->getIsInstanced()) {
          _gl->vertexAttribDivisor(_order, vertexBuffer->getInstanceDivisor());
          if (!_vaoRecordInProgress) {
            _currentInstanceLocations.emplace_back(order);
            _currentInstanceBuffers.emplace_back(buffer);
          }
        }
      }
    }
  }
}

std::unique_ptr<GL::IGLVertexArrayObject> Engine::recordVertexArrayObject(
  const std::unordered_map<std::string, VertexBufferPtr>& vertexBuffers, GL::IGLBuffer* indexBuffer,
  const EffectPtr& effect)
{
  auto vao = _gl->createVertexArray();

  _vaoRecordInProgress = true;

  _gl->bindVertexArray(vao.get());

  _mustWipeVertexAttributes = true;
  _bindVertexBuffersAttributes(vertexBuffers, effect);

  bindIndexBuffer(indexBuffer);

  _vaoRecordInProgress = false;
  _gl->bindVertexArray(nullptr);

  return vao;
}

void Engine::bindVertexArrayObject(GL::IGLVertexArrayObject* vertexArrayObject,
                                   GL::IGLBuffer* indexBuffer)
{
  if (_cachedVertexArrayObject != vertexArrayObject) {
    _cachedVertexArrayObject = vertexArrayObject;

    _gl->bindVertexArray(vertexArrayObject);
    _cachedVertexBuffers = nullptr;
    _cachedIndexBuffer   = nullptr;

    _uintIndicesCurrentlySet  = indexBuffer != nullptr && indexBuffer->is32Bits;
    _mustWipeVertexAttributes = true;
  }
}

void Engine::bindBuffersDirectly(GL::IGLBuffer* vertexBuffer, GL::IGLBuffer* indexBuffer,
                                 const Float32Array& vertexDeclaration, int vertexStrideSize,
                                 const EffectPtr& effect)
{
  if (_cachedVertexBuffers != vertexBuffer || _cachedEffectForVertexBuffers != effect) {
    _cachedVertexBuffers          = vertexBuffer;
    _cachedEffectForVertexBuffers = effect;

    auto attributesCount = effect->getAttributesCount();

    _unbindVertexArrayObject();
    unbindAllAttributes();

    int offset = 0;
    for (unsigned int index = 0; index < attributesCount; ++index) {
      if (index < vertexDeclaration.size()) {
        auto vertexDeclarationi = static_cast<int>(vertexDeclaration[index]);
        auto order              = effect->getAttributeLocation(index);

        if (order >= 0) {
          auto _order = static_cast<unsigned int>(order);
          if (_order + 1 > _vertexAttribArraysEnabled.size()) {
            _gl->enableVertexAttribArray(_order);
            _vertexAttribArraysEnabled.resize(_order + 1);
            _vertexAttribArraysEnabled[_order] = true;
          }

          _gl->vertexAttribPointer(_order, vertexDeclarationi, GL::FLOAT, false, vertexStrideSize,
                                   offset);
        }
        offset += vertexDeclarationi * 4;
      }
    }
  }

  _bindIndexBufferWithCache(indexBuffer);
}

void Engine::_unbindVertexArrayObject()
{
  if (!_cachedVertexArrayObject) {
    return;
  }

  _cachedVertexArrayObject = nullptr;
  _gl->bindVertexArray(nullptr);
}

void Engine::bindBuffers(const std::unordered_map<std::string, VertexBufferPtr>& vertexBuffers,
                         GL::IGLBuffer* indexBuffer, const EffectPtr& effect)
{
  if (_cachedVertexBuffersMap != vertexBuffers || _cachedEffectForVertexBuffers != effect) {
    _cachedVertexBuffersMap       = vertexBuffers;
    _cachedEffectForVertexBuffers = effect;

    _bindVertexBuffersAttributes(vertexBuffers, effect);
  }

  _bindIndexBufferWithCache(indexBuffer);
}

void Engine::unbindInstanceAttributes()
{
  GL::IGLBuffer* boundBuffer = nullptr;
  for (size_t i = 0, ul = _currentInstanceLocations.size(); i < ul; ++i) {
    auto instancesBuffer = _currentInstanceBuffers[i];
    if (boundBuffer != instancesBuffer && instancesBuffer->references) {
      boundBuffer = instancesBuffer;
      bindArrayBuffer(instancesBuffer);
    }
    auto offsetLocation = static_cast<unsigned int>(_currentInstanceLocations[i]);
    _gl->vertexAttribDivisor(offsetLocation, 0);
  }
  _currentInstanceBuffers.clear();
  _currentInstanceLocations.clear();
}

void Engine::releaseVertexArrayObject(GL::IGLVertexArrayObject* vao)
{
  _gl->deleteVertexArray(vao);
}

bool Engine::_releaseBuffer(GL::IGLBuffer* buffer)
{
  --buffer->references;

  if (buffer->references == 0) {
    _gl->deleteBuffer(buffer);
    return true;
  }

  return false;
}

Engine::GLBufferPtr Engine::createInstancesBuffer(unsigned int capacity)
{
  auto buffer = _gl->createBuffer();
  if (!buffer) {
    BABYLON_LOG_ERROR("Engine", "Unable to create instance buffer")
    return nullptr;
  }

  buffer->capacity = capacity;

  bindArrayBuffer(buffer.get());
  _gl->bufferData(GL::ARRAY_BUFFER, capacity, GL::DYNAMIC_DRAW);
  return buffer;
}

void Engine::deleteInstancesBuffer(GL::IGLBuffer* buffer)
{
  _gl->deleteBuffer(buffer);
}

void Engine::updateAndBindInstancesBuffer(GL::IGLBuffer* instancesBuffer, const Float32Array& data,
                                          const Uint32Array& offsetLocations)
{
  _gl->bindBuffer(GL::ARRAY_BUFFER, instancesBuffer);
  if (!data.empty()) {
    _gl->bufferSubData(GL::ARRAY_BUFFER, 0, stl_util::as_span(data));
  }

  for (unsigned int index = 0; index < 4; ++index) {
    auto& offsetLocation = offsetLocations[index];
    _gl->enableVertexAttribArray(offsetLocation);

    if (offsetLocation > _vertexAttribArraysEnabled.size()) {
      _gl->enableVertexAttribArray(offsetLocation);
      _vertexAttribArraysEnabled.resize(offsetLocation + 1);
      _vertexAttribArraysEnabled[offsetLocation] = true;
    }

    _vertexAttribPointer(instancesBuffer, offsetLocation, 4, GL::FLOAT, false, 64,
                         static_cast<int>(index * 16));
    _gl->vertexAttribDivisor(offsetLocation, 1);
    _currentInstanceLocations.emplace_back(offsetLocation);
    _currentInstanceBuffers.emplace_back(instancesBuffer);
  }
}

void Engine::updateAndBindInstancesBuffer(
  GL::IGLBuffer* instancesBuffer, const Float32Array& data,
  const std::vector<InstancingAttributeInfo>& offsetLocations)
{
  _gl->bindBuffer(GL::ARRAY_BUFFER, instancesBuffer);
  if (!data.empty()) {
    _gl->bufferSubData(GL::ARRAY_BUFFER, 0, stl_util::as_span(data));
  }

  int stride = 0;
  for (const auto& offsetLocation : offsetLocations) {
    stride += offsetLocation.attributeSize * 4;
  }

  for (const auto& ai : offsetLocations) {
    if (ai.index > _vertexAttribArraysEnabled.size()) {
      _gl->enableVertexAttribArray(ai.index);
      _vertexAttribArraysEnabled.resize(ai.index + 1);
      _vertexAttribArraysEnabled[ai.index] = true;
    }

    _gl->enableVertexAttribArray(ai.index);
    _vertexAttribPointer(instancesBuffer, ai.index, ai.attributeSize, ai.attribyteType,
                         ai.normalized, stride, ai.offset);
    _gl->vertexAttribDivisor(ai.index, 1);
    _currentInstanceLocations.emplace_back(ai.index);
    _currentInstanceBuffers.emplace_back(instancesBuffer);
  }
}

void Engine::applyStates()
{
  _depthCullingState->apply(*_gl);
  _stencilState->apply(*_gl);
  _alphaState->apply(*_gl);
}

void Engine::draw(bool useTriangles, int indexStart, int indexCount, int instancesCount)
{
  drawElementsType(useTriangles ? Material::TriangleFillMode : Material::WireFrameFillMode,
                   indexStart, indexCount, instancesCount);
}

void Engine::drawPointClouds(int verticesStart, int verticesCount, int instancesCount)
{
  drawArraysType(Material::PointFillMode, verticesStart, verticesCount, instancesCount);
}

void Engine::drawUnIndexed(bool useTriangles, int verticesStart, int verticesCount,
                           int instancesCount)
{
  drawArraysType(useTriangles ? Material::TriangleFillMode : Material::WireFrameFillMode,
                 verticesStart, verticesCount, instancesCount);
}

void Engine::drawElementsType(unsigned int fillMode, int indexStart, int indexCount,
                              int instancesCount)
{
  // Apply states
  applyStates();

  _drawCalls.addCount(1, false);

  // Render
  const auto drawMode = _drawMode(fillMode);
  auto indexFormat    = _uintIndicesCurrentlySet ? GL::UNSIGNED_INT : GL::UNSIGNED_SHORT;
  auto mult           = _uintIndicesCurrentlySet ? 4 : 2;
  if (instancesCount) {
    _gl->drawElementsInstanced(drawMode, indexCount, indexFormat, indexStart * mult,
                               instancesCount);
  }
  else {
    _gl->drawElements(drawMode, indexCount, indexFormat, indexStart * mult);
  }
}

void Engine::drawArraysType(unsigned int fillMode, int verticesStart, int verticesCount,
                            int instancesCount)
{
  // Apply states
  applyStates();
  _drawCalls.addCount(1, false);

  // Render
  const auto drawMode = _drawMode(fillMode);
  if (instancesCount) {
    _gl->drawArraysInstanced(drawMode, verticesStart, verticesCount, instancesCount);
  }
  else {
    _gl->drawArrays(drawMode, verticesStart, verticesCount);
  }
}

unsigned int Engine::_drawMode(unsigned int fillMode) const
{
  switch (fillMode) {
    // Triangle views
    case Material::TriangleFillMode:
      return GL::TRIANGLES;
    case Material::PointFillMode:
      return GL::POINTS;
    case Material::WireFrameFillMode:
      return GL::LINES;
    // Draw modes
    case Material::PointListDrawMode:
      return GL::POINTS;
    case Material::LineListDrawMode:
      return GL::LINES;
    case Material::LineLoopDrawMode:
      return GL::LINE_LOOP;
    case Material::LineStripDrawMode:
      return GL::LINE_STRIP;
    case Material::TriangleStripDrawMode:
      return GL::TRIANGLE_STRIP;
    case Material::TriangleFanDrawMode:
      return GL::TRIANGLE_FAN;
    default:
      return GL::TRIANGLES;
  }
}

// Shaders
void Engine::_releaseEffect(Effect* effect)
{
  bool hasEffect = false;
  for (const auto& compiledEffectItem : _compiledEffects) {
    if (compiledEffectItem.second.get() == effect) {
      hasEffect = true;
      break;
    }
  }
  if (hasEffect) {
    _deleteProgram(effect->getProgram());
    _compiledEffects.erase(effect->_key);
  }
}

void Engine::_deleteProgram(GL::IGLProgram* program)
{
  if (program) {
    program->__SPECTOR_rebuildProgram = nullptr;

    if (program->transformFeedback) {
      deleteTransformFeedback(program->transformFeedback.get());
      program->transformFeedback = nullptr;
    }

    _gl->deleteProgram(program);
  }
}

EffectPtr Engine::createEffect(const std::string& baseName, EffectCreationOptions& options,
                               Engine* engine,
                               const std::function<void(const EffectPtr& effect)>& onCompiled)
{
  std::string name = baseName + "+" + baseName + "@" + options.defines;
  if (stl_util::contains(_compiledEffects, name)) {
    auto compiledEffect = _compiledEffects[name];
    if (onCompiled && compiledEffect->isReady()) {
      onCompiled(compiledEffect);
    }
    return compiledEffect;
  }

  auto effect            = Effect::New(baseName, options, engine);
  effect->_key           = name;
  _compiledEffects[name] = effect;

  return effect;
}

EffectPtr Engine::createEffect(std::unordered_map<std::string, std::string>& baseName,
                               EffectCreationOptions& options, Engine* engine)
{
  std::string vertex = stl_util::contains(baseName, "vertexElement") ?
                         baseName["vertexElement"] :
                         stl_util::contains(baseName, "vertex") ? baseName["vertex"] : "vertex";
  std::string fragment
    = stl_util::contains(baseName, "fragmentElement") ?
        baseName["fragmentElement"] :
        stl_util::contains(baseName, "fragment") ? baseName["fragment"] : "fragment";

  std::string name = vertex + "+" + fragment + "@" + options.defines;
  if (stl_util::contains(_compiledEffects, name)) {
    return _compiledEffects[name];
  }

  auto effect            = Effect::New(baseName, options, engine);
  effect->_key           = name;
  _compiledEffects[name] = effect;

  return effect;
}

EffectPtr Engine::createEffectForParticles(const std::string& fragmentName,
                                           EffectCreationOptions& options)
{
  auto attributesNamesOrOptions = ParticleSystem::_GetAttributeNamesOrOptions();
  auto effectCreationOption     = ParticleSystem::_GetEffectCreationOptions();

  auto defines = options.defines;
  if (!String::contains(defines, " BILLBOARD")) {
    defines += "\n#define BILLBOARD\n";
  }

  auto samplers = options.samplers;
  if (!stl_util::contains(samplers, "diffuseSampler")) {
    samplers.emplace_back("diffuseSampler");
  }

  EffectCreationOptions effectOptions;
  std::unordered_map<std::string, std::string> baseName{
    {"vertex", "particles"},           //
    {"fragmentElement", fragmentName}, //
  };
  effectOptions.attributes    = options.attributes;
  effectOptions.uniformsNames = stl_util::concat(effectCreationOption, options.uniformsNames);
  effectOptions.samplers      = std::move(samplers);
  effectOptions.defines       = std::move(defines);
  effectOptions.fallbacks     = std::move(options.fallbacks);
  effectOptions.onCompiled    = options.onCompiled;
  effectOptions.onError       = options.onError;

  return createEffect(baseName, effectOptions, this);
}

GL::IGLShaderPtr Engine::_compileShader(const std::string& source, const std::string& type,
                                        const std::string& defines,
                                        const std::string& shaderVersion)
{
  return _compileRawShader(shaderVersion + ((!defines.empty()) ? defines + "\n" : "") + source,
                           type);
}

GL::IGLShaderPtr Engine::_compileRawShader(const std::string& source, const std::string& type)
{
  auto shader = _gl->createShader(type == "vertex" ? GL::VERTEX_SHADER : GL::FRAGMENT_SHADER);

  if (!shader) {
    BABYLON_LOG_ERROR("Engine", "Something went wrong while compile the shader.")
    return nullptr;
  }

  _gl->shaderSource(shader.get(), source);
  _gl->compileShader(shader.get());

  if (!_gl->getShaderParameter(shader.get(), GL::COMPILE_STATUS)) {
    auto log = _gl->getShaderInfoLog(shader.get());
    if (!log.empty()) {
      BABYLON_LOG_ERROR("Engine", "CompileRawShader error : %s", log)
      BABYLON_LOG_ERROR("Engine", "With source=\n%s\n", source)
    }
    return nullptr;
  }

  return shader;
}

GL::IGLProgramPtr
Engine::createRawShaderProgram(const std::string& vertexCode, const std::string& fragmentCode,
                               GL::IGLRenderingContext* context,
                               const std::vector<std::string>& transformFeedbackVaryings)
{
  context = context ? context : _gl;

  auto vertexShader   = _compileRawShader(vertexCode, "vertex");
  if (!vertexShader) {
    BABYLON_LOG_ERROR("Engine", "Engine::createRawShaderProgram failed on vertexShader");
    return nullptr;
  }
  auto fragmentShader = _compileRawShader(fragmentCode, "fragment");
  if (!fragmentShader) {
    BABYLON_LOG_ERROR("Engine", "Engine::createRawShaderProgram failed on fragmentShader");
    return nullptr;
  }

  return _createShaderProgram(vertexShader, fragmentShader, context, transformFeedbackVaryings);
}

GL::IGLProgramPtr
Engine::createShaderProgram(const std::string& vertexCode, const std::string& fragmentCode,
                            const std::string& defines, GL::IGLRenderingContext* context,
                            const std::vector<std::string>& transformFeedbackVaryings)
{
  context = context ? context : _gl;

  onBeforeShaderCompilationObservable.notifyObservers(this);

  const std::string shaderVersion
    = (_webGLVersion > 1.f) ? BABYLONCPP_GLSL_VERSION_3 "#define WEBGL2 \n" : "";
  auto vertexShader   = _compileShader(vertexCode, "vertex", defines, shaderVersion);
  if (!vertexShader) {
    BABYLON_LOG_ERROR("Engine", "Engine::createShaderProgram failed on vertexShader");
    return nullptr;
  }
  auto fragmentShader = _compileShader(fragmentCode, "fragment", defines, shaderVersion);
  if (!fragmentShader) {
    BABYLON_LOG_ERROR("Engine", "Engine::createShaderProgram failed on fragmentShader");
    return nullptr;
  }

  auto program
    = _createShaderProgram(vertexShader, fragmentShader, context, transformFeedbackVaryings);

  onAfterShaderCompilationObservable.notifyObservers(this);

  return program;
}

GL::IGLProgramPtr Engine::createShaderProgram(const IPipelineContextPtr& /*pipelineContext*/,
                                              const std::string& /*vertexCode*/,
                                              const std::string& /*fragmentCode*/,
                                              const std::string& /*defines*/,
                                              GL::IGLRenderingContext* /*context*/)
{
  return nullptr;
}

GL::IGLProgramPtr Engine::_createShaderProgram(
  const GL::IGLShaderPtr& vertexShader, const GL::IGLShaderPtr& fragmentShader,
  GL::IGLRenderingContext* context, const std::vector<std::string>& transformFeedbackVaryings)
{
  auto shaderProgram = context->createProgram();
  if (!shaderProgram) {
    BABYLON_LOG_ERROR("Engine", "Unable to create program")
    return nullptr;
  }

  context->attachShader(shaderProgram.get(), vertexShader.get());
  context->attachShader(shaderProgram.get(), fragmentShader.get());

  if (webGLVersion() > 1.f && !transformFeedbackVaryings.empty()) {
    auto transformFeedback = createTransformFeedback();

    bindTransformFeedback(transformFeedback.get());
    setTranformFeedbackVaryings(shaderProgram.get(), transformFeedbackVaryings);
    shaderProgram->transformFeedback = transformFeedback;
  }

  bool linked = context->linkProgram(shaderProgram.get());

  if (webGLVersion() > 1.f && !transformFeedbackVaryings.empty()) {
    bindTransformFeedback(nullptr);
  }

  if (!_caps.parallelShaderCompile) {
    _finalizeProgram(shaderProgram, vertexShader, fragmentShader, context, linked);
  }
  else {
    shaderProgram->isParallelCompiled = true;
  }

  return shaderProgram;
}

void Engine::_finalizeProgram(const GL::IGLProgramPtr& shaderProgram,
                              const GL::IGLShaderPtr& vertexShader,
                              const GL::IGLShaderPtr& fragmentShader,
                              GL::IGLRenderingContext* context, bool linked)
{
  if (!linked) {
    const auto error = context->getProgramInfoLog(shaderProgram.get());
    if (!error.empty()) {
      BABYLON_LOG_ERROR("Engine", error)
    }
  }

  if (validateShaderPrograms) {
    context->validateProgram(shaderProgram.get());
    auto validated = context->getProgramParameter(shaderProgram.get(), GL::VALIDATE_STATUS);
    if (!validated) {
      const auto error = context->getProgramInfoLog(shaderProgram.get());
      if (!error.empty()) {
        BABYLON_LOG_ERROR("Engine", error)
      }
    }
  }

  context->deleteShader(vertexShader.get());
  context->deleteShader(fragmentShader.get());
}

bool Engine::_isProgramCompiled(GL::IGLProgram* /*shaderProgram*/)
{
  return false;
}

bool Engine::_isRenderingStateCompiled(IPipelineContext const* /*pipelineContext*/)
{
  return false;
}

std::unordered_map<std::string, std::unique_ptr<GL::IGLUniformLocation>>
Engine::getUniforms(GL::IGLProgram* shaderProgram, const std::vector<std::string>& uniformsNames)
{
  std::unordered_map<std::string, std::unique_ptr<GL::IGLUniformLocation>> results;

  for (auto& name : uniformsNames) {
    auto uniform = _gl->getUniformLocation(shaderProgram, name);
    if (uniform) {
      results[name] = std::move(uniform);
    }
  }

  return results;
}

std::unordered_map<std::string, Engine::GLUniformLocationPtr>
Engine::getUniforms(const IPipelineContextPtr& /*pipelineContext*/,
                    const std::vector<std::string>& /*uniformsNames*/)
{
  return {};
}

Int32Array Engine::getAttributes(GL::IGLProgram* shaderProgram,
                                 const std::vector<std::string>& attributesNames)
{
  Int32Array results;

  for (auto& attributesName : attributesNames) {
    results.emplace_back(_gl->getAttribLocation(shaderProgram, attributesName));
  }

  return results;
}

Int32Array Engine::getAttributes(const IPipelineContextPtr& /*pipelineContext*/,
                                 const std::vector<std::string>& /*attributesNames*/)
{
  return {};
}

void Engine::enableEffect(const EffectPtr& effect)
{
  if (!effect || effect == _currentEffect) {
    return;
  }

  // Use program
  bindSamplers(*effect);

  _currentEffect = effect;

  if (effect->onBind) {
    effect->onBind(effect.get());
  }
  effect->onBindObservable().notifyObservers(effect.get());
}

void Engine::setIntArray(GL::IGLUniformLocation* uniform, const Int32Array& array)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1iv(uniform, stl_util::as_span(array));
}

void Engine::setIntArray2(GL::IGLUniformLocation* uniform, const Int32Array& array)
{
  if (!uniform || array.size() % 2 != 0) {
    return;
  }

  _gl->uniform2iv(uniform, stl_util::as_span(array));
}

void Engine::setIntArray3(GL::IGLUniformLocation* uniform, const Int32Array& array)
{
  if (!uniform || array.size() % 3 != 0) {
    return;
  }

  _gl->uniform3iv(uniform, stl_util::as_span(array));
}

void Engine::setIntArray4(GL::IGLUniformLocation* uniform, const Int32Array& array)
{
  if (!uniform || array.size() % 4 != 0) {
    return;
  }

  _gl->uniform4iv(uniform, stl_util::as_span(array));
}

void Engine::setFloatArray(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1fv(uniform, stl_util::as_span(array));
}

void Engine::setFloatArray2(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 2 != 0) {
    return;
  }

  _gl->uniform2fv(uniform, stl_util::as_span(array));
}

void Engine::setFloatArray3(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 3 != 0) {
    return;
  }

  _gl->uniform3fv(uniform, stl_util::as_span(array));
}

void Engine::setFloatArray4(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 4 != 0) {
    return;
  }

  _gl->uniform4fv(uniform, stl_util::as_span(array));
}

void Engine::setArray(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1fv(uniform, stl_util::as_span(array));
}

void Engine::setArray2(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 2 != 0) {
    return;
  }

  _gl->uniform2fv(uniform, stl_util::as_span(array));
}

void Engine::setArray3(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 3 != 0) {
    return;
  }

  _gl->uniform3fv(uniform, stl_util::as_span(array));
}

void Engine::setArray4(GL::IGLUniformLocation* uniform, const Float32Array& array)
{
  if (!uniform || array.size() % 4 != 0) {
    return;
  }

  _gl->uniform4fv(uniform, stl_util::as_span(array));
}

void Engine::setMatrices(GL::IGLUniformLocation* uniform, const Float32Array& matrices)
{
  if (!uniform) {
    return;
  }

  _gl->uniformMatrix4fv(uniform, false, stl_util::as_span(matrices));
}

void Engine::setMatrix(GL::IGLUniformLocation* uniform, const Matrix& matrix)
{
  if (!uniform) {
    return;
  }

  auto arr = matrix.toArray();
  _gl->uniformMatrix4fv(uniform, false, stl_util::as_span(arr));
}

void Engine::setMatrix3x3(GL::IGLUniformLocation* uniform, const Float32Array& matrix)
{
  if (!uniform) {
    return;
  }

  _gl->uniformMatrix3fv(uniform, false, stl_util::as_span(matrix));
}

void Engine::setMatrix2x2(GL::IGLUniformLocation* uniform, const Float32Array& matrix)
{
  if (!uniform) {
    return;
  }

  _gl->uniformMatrix2fv(uniform, false, stl_util::as_span(matrix));
}

void Engine::setInt(GL::IGLUniformLocation* uniform, int value)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1i(uniform, value);
}

void Engine::setFloat(GL::IGLUniformLocation* uniform, float value)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1f(uniform, value);
}

void Engine::setFloat2(GL::IGLUniformLocation* uniform, float x, float y)
{
  if (!uniform) {
    return;
  }

  _gl->uniform2f(uniform, x, y);
}

void Engine::setFloat3(GL::IGLUniformLocation* uniform, float x, float y, float z)
{
  if (!uniform) {
    return;
  }

  _gl->uniform3f(uniform, x, y, z);
}

void Engine::setBool(GL::IGLUniformLocation* uniform, int value)
{
  if (!uniform) {
    return;
  }

  _gl->uniform1i(uniform, value);
}

void Engine::setFloat4(GL::IGLUniformLocation* uniform, float x, float y, float z, float w)
{
  if (!uniform) {
    return;
  }

  _gl->uniform4f(uniform, x, y, z, w);
}

void Engine::setColor3(GL::IGLUniformLocation* uniform, const Color3& color3)
{
  if (!uniform) {
    return;
  }

  _gl->uniform3f(uniform, color3.r, color3.g, color3.b);
}

void Engine::setColor4(GL::IGLUniformLocation* uniform, const Color3& color3, float alpha)
{
  if (!uniform) {
    return;
  }

  _gl->uniform4f(uniform, color3.r, color3.g, color3.b, alpha);
}

void Engine::setDirectColor4(GL::IGLUniformLocation* uniform, const Color4& color4)
{
  if (!uniform) {
    return;
  }

  _gl->uniform4f(uniform, color4.r, color4.g, color4.b, color4.a);
}

/** States **/
void Engine::setState(bool culling, float zOffset, bool force, bool reverseSide)
{
  // Culling
  if (_depthCullingState->cull() != culling || force) {
    _depthCullingState->cull = culling;
  }

  // Cull face
  const auto cullFace = cullBackFaces ? GL::BACK : GL::FRONT;
  if ((_depthCullingState->cullFace().has_value()
       && *_depthCullingState->cullFace() != static_cast<int>(cullFace))
      || force) {
    _depthCullingState->cullFace = static_cast<int>(cullFace);
  }

  // Z offset
  setZOffset(zOffset);

  // Front face
  const auto frontFace = reverseSide ? GL::CW : GL::CCW;
  if (_depthCullingState->frontFace() != frontFace || force) {
    _depthCullingState->frontFace = frontFace;
  }
}

void Engine::setZOffset(float value)
{
  _depthCullingState->zOffset = value;
}

float Engine::getZOffset() const
{
  return _depthCullingState->zOffset();
}

void Engine::setDepthBuffer(bool enable)
{
  _depthCullingState->depthTest = enable;
}

bool Engine::getDepthWrite() const
{
  return _depthCullingState->depthMask();
}

void Engine::setDepthWrite(bool enable)
{
  _depthCullingState->depthMask = enable;
}

void Engine::setColorWrite(bool enable)
{
  _gl->colorMask(enable, enable, enable, enable);
  _colorWrite = enable;
}

bool Engine::getColorWrite() const
{
  return _colorWrite;
}

void Engine::setAlphaConstants(float r, float g, float b, float a)
{
  _alphaState->setAlphaBlendConstants(r, g, b, a);
}

void Engine::setAlphaMode(unsigned int mode, bool noDepthWriteChange)
{
  if (_alphaMode == mode) {
    return;
  }

  switch (mode) {
    case Constants::ALPHA_DISABLE:
      _alphaState->alphaBlend = false;
      break;
    case Constants::ALPHA_PREMULTIPLIED:
      _alphaState->setAlphaBlendFunctionParameters(GL::ONE, GL::ONE_MINUS_SRC_ALPHA, GL::ONE,
                                                   GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_PREMULTIPLIED_PORTERDUFF:
      _alphaState->setAlphaBlendFunctionParameters(GL::ONE, GL::ONE_MINUS_SRC_ALPHA, GL::ONE,
                                                   GL::ONE_MINUS_SRC_ALPHA);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_COMBINE:
      _alphaState->setAlphaBlendFunctionParameters(GL::SRC_ALPHA, GL::ONE_MINUS_SRC_ALPHA, GL::ONE,
                                                   GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_ONEONE:
      _alphaState->setAlphaBlendFunctionParameters(GL::ONE, GL::ONE, GL::ZERO, GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_ADD:
      _alphaState->setAlphaBlendFunctionParameters(GL::SRC_ALPHA, GL::ONE, GL::ZERO, GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_SUBTRACT:
      _alphaState->setAlphaBlendFunctionParameters(GL::ZERO, GL::ONE_MINUS_SRC_COLOR, GL::ONE,
                                                   GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_MULTIPLY:
      _alphaState->setAlphaBlendFunctionParameters(GL::DST_COLOR, GL::ZERO, GL::ONE, GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_MAXIMIZED:
      _alphaState->setAlphaBlendFunctionParameters(GL::SRC_ALPHA, GL::ONE_MINUS_SRC_COLOR, GL::ONE,
                                                   GL::ONE);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_INTERPOLATE:
      _alphaState->setAlphaBlendFunctionParameters(GL::CONSTANT_COLOR, GL::ONE_MINUS_CONSTANT_COLOR,
                                                   GL::CONSTANT_ALPHA,
                                                   GL::ONE_MINUS_CONSTANT_ALPHA);
      _alphaState->alphaBlend = true;
      break;
    case Constants::ALPHA_SCREENMODE:
      _alphaState->setAlphaBlendFunctionParameters(GL::ONE, GL::ONE_MINUS_SRC_COLOR, GL::ONE,
                                                   GL::ONE_MINUS_SRC_ALPHA);
      _alphaState->alphaBlend = true;
      break;
    default:
      break;
  }
  if (!noDepthWriteChange) {
    setDepthWrite(mode == Constants::ALPHA_DISABLE);
  }
  _alphaMode = mode;
}

unsigned int Engine::getAlphaMode() const
{
  return _alphaMode;
}

_StencilState* Engine::stencilState()
{
  return _stencilState.get();
}

// Textures

void Engine::clearInternalTexturesCache()
{
  _internalTexturesCache.clear();
}

void Engine::wipeCaches(bool bruteForce)
{
  if (preventCacheWipeBetweenFrames && !bruteForce) {
    return;
  }
  _currentEffect = nullptr;

  _viewportCached.x = 0.f;
  _viewportCached.y = 0.f;
  _viewportCached.z = 0.f;
  _viewportCached.w = 0.f;

  if (bruteForce) {
    resetTextureCache();
    _currentProgram = nullptr;

    _stencilState->reset();
    _depthCullingState->reset();
    setDepthFunctionToLessOrEqual();
    _alphaState->reset();

    _unpackFlipYCached = std::nullopt;
  }

  _resetVertexBufferBinding();
  _cachedIndexBuffer            = nullptr;
  _cachedEffectForVertexBuffers = nullptr;
  _unbindVertexArrayObject();
  bindIndexBuffer(nullptr);
}

std::string& Engine::setTextureFormatToUse(const std::vector<std::string>& formatsAvailable)
{
  for (const auto& textureSupported1 : _texturesSupported) {
    for (const auto& formatAvailable : formatsAvailable) {
      if (textureSupported1 == String::toLowerCase(formatAvailable)) {
        _textureFormatInUse = textureSupported1;
        return _textureFormatInUse;
      }
    }
  }
  // actively set format to nothing, to allow this to be called more than once and possibly fail the
  // 2nd time
  _textureFormatInUse.clear();
  return _textureFormatInUse;
}

SamplingParameters Engine::_getSamplingParameters(unsigned int samplingMode, bool generateMipMaps)
{
  GL::GLenum magFilter = GL::NEAREST;
  GL::GLenum minFilter = GL::NEAREST;

  switch (samplingMode) {
    case TextureConstants::BILINEAR_SAMPLINGMODE:
      magFilter = GL::LINEAR;
      if (generateMipMaps) {
        minFilter = GL::LINEAR_MIPMAP_NEAREST;
      }
      else {
        minFilter = GL::LINEAR;
      }
      break;
    case TextureConstants::TRILINEAR_SAMPLINGMODE:
      magFilter = GL::LINEAR;
      if (generateMipMaps) {
        minFilter = GL::LINEAR_MIPMAP_LINEAR;
      }
      else {
        minFilter = GL::LINEAR;
      }
      break;
    case TextureConstants::NEAREST_SAMPLINGMODE:
      magFilter = GL::NEAREST;
      if (generateMipMaps) {
        minFilter = GL::NEAREST_MIPMAP_LINEAR;
      }
      else {
        minFilter = GL::NEAREST;
      }
      break;
    case TextureConstants::NEAREST_NEAREST_MIPNEAREST:
      magFilter = GL::NEAREST;
      if (generateMipMaps) {
        minFilter = GL::NEAREST_MIPMAP_NEAREST;
      }
      else {
        minFilter = GL::NEAREST;
      }
      break;
    case TextureConstants::NEAREST_LINEAR_MIPNEAREST:
      magFilter = GL::NEAREST;
      if (generateMipMaps) {
        minFilter = GL::LINEAR_MIPMAP_NEAREST;
      }
      else {
        minFilter = GL::LINEAR;
      }
      break;
    case TextureConstants::NEAREST_LINEAR_MIPLINEAR:
      magFilter = GL::NEAREST;
      if (generateMipMaps) {
        minFilter = GL::LINEAR_MIPMAP_LINEAR;
      }
      else {
        minFilter = GL::LINEAR;
      }
      break;
    case TextureConstants::NEAREST_LINEAR:
      magFilter = GL::NEAREST;
      minFilter = GL::LINEAR;
      break;
    case TextureConstants::NEAREST_NEAREST:
      magFilter = GL::NEAREST;
      minFilter = GL::NEAREST;
      break;
    case TextureConstants::LINEAR_NEAREST_MIPNEAREST:
      magFilter = GL::LINEAR;
      if (generateMipMaps) {
        minFilter = GL::NEAREST_MIPMAP_NEAREST;
      }
      else {
        minFilter = GL::NEAREST;
      }
      break;
    case TextureConstants::LINEAR_NEAREST_MIPLINEAR:
      magFilter = GL::LINEAR;
      if (generateMipMaps) {
        minFilter = GL::NEAREST_MIPMAP_LINEAR;
      }
      else {
        minFilter = GL::NEAREST;
      }
      break;
    case TextureConstants::LINEAR_LINEAR:
      magFilter = GL::LINEAR;
      minFilter = GL::LINEAR;
      break;
    case TextureConstants::LINEAR_NEAREST:
      magFilter = GL::LINEAR;
      minFilter = GL::NEAREST;
      break;
  }

  return {static_cast<int>(minFilter), static_cast<int>(magFilter)};
}

void Engine::_cascadeLoadImgs(
  const std::string& /*rootUrl*/, Scene* /*scene*/,
  const std::function<void(const std::vector<Image>& images)>& onfinish,
  const std::vector<std::string>& files,
  const std::function<void(const std::string& message, const std::string& exception)>& onError)
{
  std::vector<Image> images;
  for (size_t index = 0; index < 6; ++index) {
    FileTools::LoadImageFromUrl(
      files[index],
      [&images](const Image& img) { images.emplace_back(img); },
      onError, false);
  }

  // Force sync
  // because the code before seems deprecated, and is difficult enough to make it async
  BABYLON::asio::Service_WaitAll_Sync();

  if (images.size() == 6) {
    onfinish(images);
  }
  else {
    if (onError) {
      onError("Unable to cascade load images!", "");
    }
  }
}

std::unique_ptr<GL::IGLTexture> Engine::_createTexture()
{
  auto texture = _gl->createTexture();

  if (!texture) {
    BABYLON_LOG_ERROR("Engine", "Unable to create texture")
  }

  return texture;
}

InternalTexturePtr Engine::createTexture(
  const std::vector<std::string>& list, bool noMipmap, bool invertY, Scene* scene,
  unsigned int samplingMode, const std::function<void(InternalTexture*, EventState&)>& onLoad,
  const std::function<void(const std::string& message, const std::string& exception)>& onError,
  const std::variant<std::string, ArrayBuffer, ArrayBufferView, Image>& buffer)
{
  if (list.empty()) {
    return nullptr;
  }

  return createTexture(list[0], noMipmap, invertY, scene, samplingMode, onLoad, onError, buffer);
}

InternalTexturePtr Engine::createTexture(
  const std::string& urlArg, bool noMipmap, bool invertY, Scene* scene, unsigned int samplingMode,
  const std::function<void(InternalTexture*, EventState&)>& onLoad,
  const std::function<void(const std::string& message, const std::string& exception)>& onError,
  const std::optional<std::variant<std::string, ArrayBuffer, ArrayBufferView, Image>>& buffer,
  const InternalTexturePtr& fallback, const std::optional<unsigned int>& format)
{
  // assign a new string, so that the original is still available in case of fallback
  auto url      = urlArg;
  auto fromData = url.substr(0, 5) == "data:";
  auto fromBlob = url.substr(0, 5) == "blob:";
  auto isBase64 = fromData && String::contains(url, "base64");

  auto texture = fallback ? fallback : InternalTexture::New(this, InternalTexture::DATASOURCE_URL);

  // establish the file extension, if possible
  auto lastDotTmp = String::lastIndexOf(url, ".");
  auto lastDot    = lastDotTmp >= 0 ? static_cast<size_t>(lastDotTmp) : 0ull;
  auto extension  = (lastDot > 0) ? String::toLowerCase(url.substr(lastDot, url.size())) : "";

  IInternalTextureLoaderPtr loader = nullptr;
  for (const auto& availableLoader : Engine::_TextureLoaders) {
    if (availableLoader->canLoad(extension, _textureFormatInUse, fallback, false, false)) {
      loader = availableLoader;
      break;
    }
  }

  if (loader) {
    url = loader->transformUrl(url, _textureFormatInUse);
  }

  if (scene) {
    scene->_addPendingData(texture);
  }
  texture->url             = url;
  texture->generateMipMaps = !noMipmap;
  texture->samplingMode    = samplingMode;
  texture->invertY         = invertY;

  if (!_doNotHandleContextLost && buffer.has_value()) {
    // Keep a link to the buffer only if we plan to handle context lost
    texture->_buffer = *buffer;
  }

  Observer<InternalTexture>::Ptr onLoadObserver = nullptr;
  if (onLoad && !fallback) {
    onLoadObserver = texture->onLoadedObservable.add(onLoad);
  }

  if (!fallback) {
    _internalTexturesCache.emplace_back(texture);
  }

  const auto onInternalError = [=](const std::string& message, const std::string& exception) {
    if (scene) {
      scene->_removePendingData(texture);
    }

    bool customFallback = false;
    if (loader) {
      auto fallbackUrl = loader->getFallbackTextureUrl(url, _textureFormatInUse);
      if (!fallbackUrl.empty()) {
        // Add Back
        customFallback = true;
        createTexture(urlArg, noMipmap, invertY, scene, samplingMode, nullptr, onError, buffer,
                      texture);
      }
    }

    if (!customFallback) {
      if (onLoadObserver) {
        texture->onLoadedObservable.remove(onLoadObserver);
      }
      if (Tools::UseFallbackTexture()) {
        createTexture(Tools::FallbackTexture(), noMipmap, invertY, scene, samplingMode, nullptr,
                      onError, buffer, texture);
      }
    }

    if (onError) {
      onError(message.empty() ? "Unknown error" : message, exception);
    }
  };

  // processing for non-image formats
  if (loader) {
    const auto callback = [=](const std::variant<std::string, ArrayBuffer>& data,
                              const std::string & /*responseURL*/) -> void {
      loader->loadData(std::get<ArrayBuffer>(data), texture,
                       [=](int width, int height, bool loadMipmap, bool isCompressed,
                           const std::function<void()>& done, bool /*loadFailed*/) -> void {
                         _prepareWebGLTexture(
                           texture, scene, width, height, invertY, !loadMipmap, isCompressed,
                           [=](int /*width*/, int /*height*/,
                               const std::function<void()> & /*continuationCallback*/) -> bool {
                             done();
                             return false;
                           },
                           samplingMode);
                       });
    };

    if (!buffer.has_value()) {
      _loadFile(url, callback, nullptr, true, onInternalError);
    }
    else {
      callback(std::get<ArrayBuffer>(*buffer), "");
    }
  }
  else {
    auto onload = [=](const Image& img) {
      if (fromBlob && !_doNotHandleContextLost) {
        // We need to store the image if we need to rebuild the texture in case of a webgl context
        // lost
        texture->_buffer = img;
      }

      _prepareWebGLTexture(
        texture, scene, img.width, img.height, invertY, noMipmap, false,
        [=](int potWidth, int potHeight, const std::function<void()>& continuationCallback) {
          auto isPot = (img.width == potWidth && img.height == potHeight);
          auto internalFormat
            = (format ? _getInternalFormat(*format) : ((extension == ".jpg") ? GL::RGB : GL::RGBA));

          if (isPot) {
            _gl->texImage2D(GL::TEXTURE_2D, 0, static_cast<int>(internalFormat), img.width,
                            img.height, 0, GL::RGBA, GL::UNSIGNED_BYTE, stl_util::as_span(img.data));
            return false;
          }

          auto maxTextureSize = _caps.maxTextureSize;

          if (img.width > maxTextureSize || img.height > maxTextureSize) {
            _prepareWorkingCanvas();
            if (!_workingCanvas || !_workingContext) {
              return false;
            }

            _workingCanvas->width  = potWidth;
            _workingCanvas->height = potHeight;

            _workingContext->drawImage(img, 0, 0, img.width, img.height, 0, 0, potWidth, potHeight);
#if 0
            _gl->texImage2D(GL::TEXTURE_2D, 0, static_cast<int>(internalFormat),
                            internalFormat, GL::UNSIGNED_BYTE, _workingCanvas);
#endif
            texture->width  = potWidth;
            texture->height = potHeight;

            return false;
          }
          else {
            // Using shaders when possible to rescale because canvas.drawImage is lossy
            auto source = InternalTexture::New(this, InternalTexture::DATASOURCE_TEMP);
            _bindTextureDirectly(GL::TEXTURE_2D, source);
            _gl->texImage2D(GL::TEXTURE_2D, 0, static_cast<int>(internalFormat), img.width,
                            img.height, 0, GL::RGBA, GL::UNSIGNED_BYTE, stl_util::as_span(img.data));

            _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
            _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
            _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
            _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

            _rescaleTexture(source, texture, scene, internalFormat, [=]() {
              _releaseTexture(source.get());
              _bindTextureDirectly(GL::TEXTURE_2D, texture);

              continuationCallback();
            });
          }

          return true;
        },
        samplingMode);
    };

    if (!fromData || isBase64) {
      if (url.empty()) {
        // onload(buffer);
      }
      else {
        FileTools::LoadImageFromUrl(url, onload, onInternalError, invertY);
      }
    }
    else if (buffer.has_value()
             && (std::holds_alternative<std::string>(*buffer)
                 || std::holds_alternative<ArrayBuffer>(*buffer)
                 || std::holds_alternative<Image>(*buffer))) {
      FileTools::LoadImageFromBuffer(*buffer, invertY, onload, onInternalError);
    }
  }

  return texture;
}

void Engine::_rescaleTexture(const InternalTexturePtr& source,
                             const InternalTexturePtr& destination, Scene* scene,
                             unsigned int internalFormat, const std::function<void()>& onComplete)
{
  IRenderTargetOptions options;
  options.generateMipMaps       = false;
  options.type                  = Constants::TEXTURETYPE_UNSIGNED_INT;
  options.samplingMode          = Constants::TEXTURE_BILINEAR_SAMPLINGMODE;
  options.generateDepthBuffer   = false;
  options.generateStencilBuffer = false;

  auto rtt = createRenderTargetTexture(ISize(destination->width, destination->height), options);

  if (!_rescalePostProcess) {
    _rescalePostProcess
      = PassPostProcess::New("rescale", 1.f, nullptr, Constants::TEXTURE_BILINEAR_SAMPLINGMODE,
                             this, false, Constants::TEXTURETYPE_UNSIGNED_INT);
  }

  _rescalePostProcess->getEffect()->executeWhenCompiled([=](Effect* /*effect*/) {
    _rescalePostProcess->onApply
      = [=](Effect* effect, EventState&) { effect->_bindTexture("textureSampler", source); };

    auto hostingScene = scene;

    if (!hostingScene) {
      hostingScene = scenes.back();
    }
    hostingScene->postProcessManager->directRender({_rescalePostProcess}, rtt, true);

    _bindTextureDirectly(GL::TEXTURE_2D, destination, true);
    _gl->copyTexImage2D(GL::TEXTURE_2D, 0, internalFormat, 0, 0, destination->width,
                        destination->height, 0);

    unBindFramebuffer(rtt);
    _releaseTexture(rtt.get());

    if (onComplete) {
      onComplete();
    }
  });
}

void Engine::updateRawTexture(const InternalTexturePtr& texture, const Uint8Span_ro& data,
                              unsigned int format, bool invertY, const std::string& compression,
                              unsigned int type)
{
  if (!texture) {
    return;
  }

  // babylon's internalSizedFomat but gl's texImage2D internalFormat
  auto internalSizedFomat = _getRGBABufferInternalSizedFormat(type, format);

  // babylon's internalFormat but gl's texImage2D format
  auto internalFormat = _getInternalFormat(format);
  auto textureType    = _getWebGLTextureType(type);
  _bindTextureDirectly(GL::TEXTURE_2D, texture, true);
  _unpackFlipY(invertY);

  if (!_doNotHandleContextLost) {
    std::copy(data.begin(), data.end(), std::back_inserter(texture->_bufferView));
    //texture->_bufferView  = data;
    texture->format       = format;
    texture->type         = type;
    texture->invertY      = invertY;
    texture->_compression = compression;
  }

  if (texture->width % 4 != 0) {
    _gl->pixelStorei(GL::UNPACK_ALIGNMENT, 1);
  }

  if (!compression.empty() && !data.empty()) {
    //_gl->compressedTexImage2D(GL::TEXTURE_2D, 0, getCaps().s3tc[compression],
    //                          texture->_width, texture->_height, 0, stl_util::as_span(data));
  }
  else {
    auto _internalSizedFomat = static_cast<GL::GLint>(internalSizedFomat);
    _gl->texImage2D(GL::TEXTURE_2D, 0, _internalSizedFomat, texture->width, texture->height, 0,
                    internalFormat, textureType, data);
  }

  if (texture->generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_2D);
  }
  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  // resetTextureCache();
  texture->isReady = true;
}

InternalTexturePtr Engine::createRawTexture(const Uint8Span_ro& data, int width, int height,
                                            unsigned int format, bool generateMipMaps, bool invertY,
                                            unsigned int samplingMode,
                                            const std::string& compression, unsigned int type)
{
  auto texture             = InternalTexture::New(this, InternalTexture::DATASOURCE_RAW);
  texture->baseWidth       = width;
  texture->baseHeight      = height;
  texture->width           = width;
  texture->height          = height;
  texture->format          = format;
  texture->generateMipMaps = generateMipMaps;
  texture->samplingMode    = samplingMode;
  texture->invertY         = invertY;
  texture->_compression    = compression;
  texture->type            = type;

  if (!_doNotHandleContextLost) {
    std::copy(data.begin(), data.end(), std::back_inserter(texture->_bufferView));
    //texture->_bufferView = data;
  }

  updateRawTexture(texture, data, format, invertY, compression, type);
  _bindTextureDirectly(GL::TEXTURE_2D, texture, true);

  // Filters
  auto filters = _getSamplingParameters(samplingMode, generateMipMaps);

  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, filters.min);

  if (generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_2D);
  }

  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);

  texture->samplingMode = samplingMode;

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

int Engine::_getUnpackAlignement() const
{
  return _gl->getParameteri(GL::UNPACK_ALIGNMENT);
}

void Engine::_unpackFlipY(bool value)
{
  if (!_unpackFlipYCached.has_value() || *_unpackFlipYCached != value) {
    _gl->pixelStorei(GL::UNPACK_FLIP_Y_WEBGL, value ? 1 : 0);
    if (enableUnpackFlipYCached) {
      _unpackFlipYCached = value;
    }
  }
}

int Engine::_getUnpackAlignement()
{
  return _gl->getParameteri(GL::UNPACK_ALIGNMENT);
}

InternalTexturePtr Engine::createDynamicTexture(int width, int height, bool generateMipMaps,
                                                unsigned int samplingMode)
{
  auto texture        = InternalTexture::New(this, InternalTexture::DATASOURCE_DYNAMIC);
  texture->baseWidth  = width;
  texture->baseHeight = height;

  if (generateMipMaps) {
    width  = needPOTTextures() ? Engine::GetExponentOfTwo(width, _caps.maxTextureSize) : width;
    height = needPOTTextures() ? Engine::GetExponentOfTwo(height, _caps.maxTextureSize) : height;
  }

  // resetTextureCache();
  texture->width           = width;
  texture->height          = height;
  texture->isReady         = false;
  texture->generateMipMaps = generateMipMaps;
  texture->samplingMode    = samplingMode;

  updateTextureSamplingMode(samplingMode, texture);

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

void Engine::updateTextureSamplingMode(unsigned int samplingMode, const InternalTexturePtr& texture)
{
  auto filters = _getSamplingParameters(samplingMode, texture->generateMipMaps);

  if (texture->isCube) {
    _setTextureParameterInteger(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MAG_FILTER, filters.mag, texture);
    _setTextureParameterInteger(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MIN_FILTER, filters.min);
    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
  }
  else if (texture->is3D) {
    _setTextureParameterInteger(GL::TEXTURE_3D, GL::TEXTURE_MAG_FILTER, filters.mag, texture);
    _setTextureParameterInteger(GL::TEXTURE_3D, GL::TEXTURE_MIN_FILTER, filters.min);
    _bindTextureDirectly(GL::TEXTURE_3D, nullptr);
  }
  else {
    _setTextureParameterInteger(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, filters.mag, texture);
    _setTextureParameterInteger(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, filters.min);
    _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  }

  texture->samplingMode = samplingMode;
}

void Engine::updateDynamicTexture(const InternalTexturePtr& texture, ICanvas* /*canvas*/,
                                  bool invertY, bool premulAlpha, unsigned int /*format*/)
{
  if (!texture) {
    return;
  }

  _bindTextureDirectly(GL::TEXTURE_2D, texture, true);

  _unpackFlipY(invertY);
  if (premulAlpha) {
    _gl->pixelStorei(GL::UNPACK_PREMULTIPLY_ALPHA_WEBGL, 1);
  }
#if 0
  auto internalFormat = format ? _getInternalFormat(format) : GL::RGBA;
  _gl->texImage2D(GL::TEXTURE_2D, 0, static_cast<int>(internalFormat),
                  internalFormat, GL::UNSIGNED_BYTE, canvas);
#endif
  if (texture->generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_2D);
  }
  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  if (premulAlpha) {
    _gl->pixelStorei(GL::UNPACK_PREMULTIPLY_ALPHA_WEBGL, 0);
  }
  resetTextureCache();
  texture->isReady = true;
}

void Engine::updateTextureComparisonFunction(const InternalTexturePtr& texture,
                                             int comparisonFunction)
{
  if (webGLVersion() == 1.f) {
    BABYLON_LOG_ERROR("Engine", "WebGL 1 does not support texture comparison.")
    return;
  }

  if (texture->isCube) {
    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);

    if (comparisonFunction == 0) {
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_COMPARE_FUNC, Constants::LEQUAL);
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_COMPARE_MODE, GL::NONE);
    }
    else {
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_COMPARE_FUNC, comparisonFunction);
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_COMPARE_MODE,
                         GL::COMPARE_REF_TO_TEXTURE);
    }

    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
  }
  else {
    _bindTextureDirectly(GL::TEXTURE_2D, texture, true);

    if (comparisonFunction == 0) {
      _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_COMPARE_FUNC, Constants::LEQUAL);
      _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_COMPARE_MODE, GL::NONE);
    }
    else {
      _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_COMPARE_FUNC, comparisonFunction);
      _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_COMPARE_MODE, GL::COMPARE_REF_TO_TEXTURE);
    }

    _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  }

  texture->_comparisonFunction = comparisonFunction;
}

void Engine::_setupDepthStencilTexture(InternalTexture* internalTexture,
                                       const std::variant<int, ISize>& size, bool generateStencil,
                                       bool bilinearFiltering, int comparisonFunction)
{
  auto width
    = std::holds_alternative<int>(size) ? std::get<int>(size) : std::get<ISize>(size).width;
  auto height
    = std::holds_alternative<int>(size) ? std::get<int>(size) : std::get<ISize>(size).height;
  internalTexture->baseWidth              = width;
  internalTexture->baseHeight             = height;
  internalTexture->width                  = width;
  internalTexture->height                 = height;
  internalTexture->isReady                = true;
  internalTexture->samples                = 1;
  internalTexture->generateMipMaps        = false;
  internalTexture->_generateDepthBuffer   = true;
  internalTexture->_generateStencilBuffer = generateStencil;
  internalTexture->samplingMode = bilinearFiltering ? Constants::TEXTURE_BILINEAR_SAMPLINGMODE :
                                                      Constants::TEXTURE_NEAREST_SAMPLINGMODE;
  internalTexture->type                = Constants::TEXTURETYPE_UNSIGNED_INT;
  internalTexture->_comparisonFunction = comparisonFunction;

  auto target             = internalTexture->isCube ? GL::TEXTURE_CUBE_MAP : GL::TEXTURE_2D;
  auto samplingParameters = _getSamplingParameters(internalTexture->samplingMode, false);
  _gl->texParameteri(target, GL::TEXTURE_MAG_FILTER, samplingParameters.mag);
  _gl->texParameteri(target, GL::TEXTURE_MIN_FILTER, samplingParameters.min);
  _gl->texParameteri(target, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
  _gl->texParameteri(target, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

  if (comparisonFunction == 0) {
    _gl->texParameteri(target, GL::TEXTURE_COMPARE_FUNC, Constants::LEQUAL);
    _gl->texParameteri(target, GL::TEXTURE_COMPARE_MODE, GL::NONE);
  }
  else {
    _gl->texParameteri(target, GL::TEXTURE_COMPARE_FUNC, comparisonFunction);
    _gl->texParameteri(target, GL::TEXTURE_COMPARE_MODE, GL::COMPARE_REF_TO_TEXTURE);
  }
}

InternalTexturePtr Engine::createDepthStencilTexture(const std::variant<int, ISize>& size,
                                                     const DepthTextureCreationOptions& options)
{
  if (options.isCube.has_value() && *options.isCube) {
    auto width
      = std::holds_alternative<int>(size) ? std::get<int>(size) : std::get<ISize>(size).width;
    return _createDepthStencilCubeTexture(width, options);
  }
  else {
    return _createDepthStencilTexture(size, options);
  }
}

InternalTexturePtr Engine::_createDepthStencilTexture(const std::variant<int, ISize>& size,
                                                      const DepthTextureCreationOptions& options)
{
  auto internalTexture = InternalTexture::New(this, InternalTexture::DATASOURCE_DEPTHTEXTURE);

  if (!_caps.depthTextureExtension) {
    BABYLON_LOG_ERROR("Engine", "Depth texture is not supported by your browser or hardware.")
    return internalTexture;
  }

  auto internalOptions = options;
  internalOptions.bilinearFiltering
    = options.bilinearFiltering.has_value() ? *options.bilinearFiltering : false;
  internalOptions.comparisonFunction
    = options.comparisonFunction.has_value() ? *options.comparisonFunction : 0;
  internalOptions.generateStencil
    = options.generateStencil.has_value() ? *options.generateStencil : false;

  _bindTextureDirectly(GL::TEXTURE_2D, internalTexture, true);

  _setupDepthStencilTexture(internalTexture.get(), size, *internalOptions.generateStencil,
                            *internalOptions.bilinearFiltering,
                            *internalOptions.comparisonFunction);

  if (webGLVersion() > 1.f) {
    if (internalOptions.generateStencil.has_value() && *internalOptions.generateStencil) {
      _gl->texImage2D(GL::TEXTURE_2D, 0, GL::DEPTH24_STENCIL8, internalTexture->width,
                      internalTexture->height, 0, GL::DEPTH_STENCIL, GL::UNSIGNED_INT_24_8,
                      std::nullopt);
    }
    else {
      _gl->texImage2D(GL::TEXTURE_2D, 0, GL::DEPTH_COMPONENT24, internalTexture->width,
                      internalTexture->height, 0, GL::DEPTH_COMPONENT, GL::UNSIGNED_INT, std::nullopt);
    }
  }
  else {
    if (internalOptions.generateStencil.has_value() && *internalOptions.generateStencil) {
      _gl->texImage2D(GL::TEXTURE_2D, 0, GL::DEPTH_STENCIL, internalTexture->width,
                      internalTexture->height, 0, GL::DEPTH_STENCIL, GL::UNSIGNED_INT_24_8,
                      std::nullopt);
    }
    else {
      _gl->texImage2D(GL::TEXTURE_2D, 0, GL::DEPTH_COMPONENT, internalTexture->width,
                      internalTexture->height, 0, GL::DEPTH_COMPONENT, GL::UNSIGNED_INT, std::nullopt);
    }
  }

  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);

  return internalTexture;
}

InternalTexturePtr
Engine::_createDepthStencilCubeTexture(int size, const DepthTextureCreationOptions& options)
{
  auto internalTexture    = InternalTexture::New(this, InternalTexture::DATASOURCE_UNKNOWN);
  internalTexture->isCube = true;

  if (webGLVersion() == 1.f) {
    BABYLON_LOG_ERROR("Engine", "Depth cube texture is not supported by WebGL 1.")
    return internalTexture;
  }

  auto internalOptions = options;
  internalOptions.bilinearFiltering
    = options.bilinearFiltering.has_value() ? *options.bilinearFiltering : false;
  internalOptions.comparisonFunction
    = options.comparisonFunction.has_value() ? *options.comparisonFunction : 0;
  internalOptions.generateStencil
    = options.generateStencil.has_value() ? *options.generateStencil : false;

  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, internalTexture, true);

  _setupDepthStencilTexture(internalTexture.get(), size, *internalOptions.generateStencil,
                            *internalOptions.bilinearFiltering,
                            *internalOptions.comparisonFunction);

  // Create the depth/stencil buffer
  for (unsigned int face = 0; face < 6; ++face) {
    if (internalOptions.generateStencil.has_value() && *internalOptions.generateStencil) {
      _gl->texImage2D(GL::TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL::DEPTH24_STENCIL8, size, size, 0,
                    GL::DEPTH_STENCIL, GL::UNSIGNED_INT_24_8, std::nullopt);
    }
    else {
      _gl->texImage2D(GL::TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL::DEPTH_COMPONENT24, size, size, 0,
                    GL::DEPTH_COMPONENT, GL::UNSIGNED_INT, std::nullopt);
    }
  }

  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);

  return internalTexture;
}

void Engine::setFrameBufferDepthStencilTexture(RenderTargetTexture* renderTarget)
{
  // Create the framebuffer
  auto internalTexture = renderTarget->getInternalTexture();
  if (!internalTexture || !internalTexture->_framebuffer || !renderTarget->depthStencilTexture) {
    return;
  }

  auto& depthStencilTexture = renderTarget->depthStencilTexture;

  bindUnboundFramebuffer(internalTexture->_framebuffer);
  if (depthStencilTexture->isCube) {
    if (depthStencilTexture->_generateStencilBuffer) {
      _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_STENCIL_ATTACHMENT,
                                GL::TEXTURE_CUBE_MAP_POSITIVE_X,
                                depthStencilTexture->_webGLTexture.get(), 0);
    }
    else {
      _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_ATTACHMENT,
                                GL::TEXTURE_CUBE_MAP_POSITIVE_X,
                                depthStencilTexture->_webGLTexture.get(), 0);
    }
  }
  else {
    if (depthStencilTexture->_generateStencilBuffer) {
      _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_STENCIL_ATTACHMENT, GL::TEXTURE_2D,
                                depthStencilTexture->_webGLTexture.get(), 0);
    }
    else {
      _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::DEPTH_ATTACHMENT, GL::TEXTURE_2D,
                                depthStencilTexture->_webGLTexture.get(), 0);
    }
  }
  bindUnboundFramebuffer(nullptr);
}

InternalTexturePtr Engine::createRenderTargetTexture(const std::variant<ISize, float>& size,
                                                     const IRenderTargetOptions& options)
{
  RenderTargetCreationOptions fullOptions;
  fullOptions.generateMipMaps = options.generateMipMaps;
  fullOptions.generateDepthBuffer
    = !options.generateDepthBuffer.has_value() ? true : options.generateDepthBuffer;
  fullOptions.generateStencilBuffer
    = fullOptions.generateDepthBuffer.value() && options.generateStencilBuffer.value_or(false);
  fullOptions.type = options.type.value_or(Constants::TEXTURETYPE_UNSIGNED_INT);
  fullOptions.samplingMode
    = options.samplingMode.value_or(Constants::TEXTURE_TRILINEAR_SAMPLINGMODE);
  fullOptions.format = options.format.value_or(Constants::TEXTUREFORMAT_RGBA);

  if (fullOptions.type.value() == Constants::TEXTURETYPE_FLOAT
      && !_caps.textureFloatLinearFiltering) {
    // if floating point linear (_gl->FLOAT) then force to NEAREST_SAMPLINGMODE
    fullOptions.samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
  }
  else if (fullOptions.type.value() == Constants::TEXTURETYPE_HALF_FLOAT
           && !_caps.textureHalfFloatLinearFiltering) {
    // if floating point linear (HALF_FLOAT) then force to NEAREST_SAMPLINGMODE
    fullOptions.samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
  }

  auto texture = InternalTexture::New(this, InternalTexture::DATASOURCE_RENDERTARGET);
  _bindTextureDirectly(GL::TEXTURE_2D, texture, true);

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

  auto filters
    = _getSamplingParameters(fullOptions.samplingMode.value(), fullOptions.generateMipMaps.value());

  if (fullOptions.type.value() == Constants::TEXTURETYPE_FLOAT && !_caps.textureFloat) {
    fullOptions.type = Constants::TEXTURETYPE_UNSIGNED_INT;
    BABYLON_LOG_WARN("Engine",
                     "Float textures are not supported. Render target forced to "
                     "TEXTURETYPE_UNSIGNED_BYTE type")
  }

  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, filters.min);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

  _gl->texImage2D(GL::TEXTURE_2D, 0,
                  static_cast<GL::GLint>(_getRGBABufferInternalSizedFormat(
                    fullOptions.type.value(), fullOptions.format.value())),
                  width, height, 0, _getInternalFormat(fullOptions.format.value()),
                  _getWebGLTextureType(fullOptions.type.value()), std::nullopt);

  // Create the framebuffer
  auto currentFrameBuffer = _currentFramebuffer;
  auto framebuffer        = _gl->createFramebuffer();
  bindUnboundFramebuffer(framebuffer);
  _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0, GL::TEXTURE_2D,
                            texture->_webGLTexture.get(), 0);

  texture->_depthStencilBuffer
    = _setupFramebufferDepthAttachments(fullOptions.generateStencilBuffer.value(),
                                        fullOptions.generateDepthBuffer.value(), width, height);

  if (fullOptions.generateMipMaps.value()) {
    _gl->generateMipmap(GL::TEXTURE_2D);
  }

  // Unbind
  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
  _gl->bindRenderbuffer(GL::RENDERBUFFER, nullptr);
  bindUnboundFramebuffer(currentFrameBuffer);

  texture->_framebuffer           = std::move(framebuffer);
  texture->baseWidth              = width;
  texture->baseHeight             = height;
  texture->width                  = width;
  texture->height                 = height;
  texture->isReady                = true;
  texture->samples                = 1;
  texture->generateMipMaps        = fullOptions.generateMipMaps.value();
  texture->samplingMode           = fullOptions.samplingMode.value();
  texture->type                   = fullOptions.type.value();
  texture->format                 = fullOptions.format.value();
  texture->_generateDepthBuffer   = fullOptions.generateDepthBuffer.value();
  texture->_generateStencilBuffer = fullOptions.generateStencilBuffer.value();

  // resetTextureCache();

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

std::vector<InternalTexturePtr>
Engine::createMultipleRenderTarget(ISize size, const IMultiRenderTargetOptions& options)
{
  auto generateMipMaps       = options.generateMipMaps.value_or(false);
  auto generateDepthBuffer   = options.generateDepthBuffer.value_or(true);
  auto generateStencilBuffer = options.generateStencilBuffer.value_or(false);
  auto generateDepthTexture  = options.generateDepthTexture.value_or(false);
  auto textureCount          = options.textureCount.value_or(1);

  auto defaultType         = Constants::TEXTURETYPE_UNSIGNED_INT;
  auto defaultSamplingMode = Constants::TEXTURE_TRILINEAR_SAMPLINGMODE;

  const auto& types         = options.types;
  const auto& samplingModes = options.samplingModes;

  auto width  = size.width;
  auto height = size.height;

  // Create the framebuffer
  auto framebuffer = _gl->createFramebuffer();
  bindUnboundFramebuffer(framebuffer);

  std::vector<InternalTexturePtr> textures;
  std::vector<GL::GLenum> attachments;

  auto depthStencilBuffer
    = _setupFramebufferDepthAttachments(generateStencilBuffer, generateDepthBuffer, width, height);

  for (unsigned int i = 0; i < textureCount; ++i) {
    const auto iStr   = std::to_string(i);
    auto samplingMode = (i < samplingModes.size()) ? samplingModes[i] : defaultSamplingMode;
    auto type         = (i < types.size()) ? types[i] : defaultType;

    if (type == Constants::TEXTURETYPE_FLOAT && !_caps.textureFloatLinearFiltering) {
      // if floating point linear (_gl->FLOAT) then force to NEAREST_SAMPLINGMODE
      samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
    }
    else if (type == Constants::TEXTURETYPE_HALF_FLOAT && !_caps.textureHalfFloatLinearFiltering) {
      // if floating point linear (HALF_FLOAT) then force to NEAREST_SAMPLINGMODE
      samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
    }

    auto filters = _getSamplingParameters(samplingMode, generateMipMaps);
    if (type == Constants::TEXTURETYPE_FLOAT && !_caps.textureFloat) {
      type = Constants::TEXTURETYPE_UNSIGNED_INT;
      BABYLON_LOG_WARN("Engine",
                       "Float textures are not supported. Render target forced "
                       "to TEXTURETYPE_UNSIGNED_BYTE type")
    }

    auto texture = InternalTexture::New(this, InternalTexture::DATASOURCE_MULTIRENDERTARGET);
    auto attachment
      = (*_gl)[webGLVersion() > 1 ? "COLOR_ATTACHMENT" + iStr : "COLOR_ATTACHMENT" + iStr + "_WEBGL"];
    textures.emplace_back(texture);
    attachments.emplace_back(attachment);

    _gl->activeTexture((*_gl)["TEXTURE0"] + i);
    _gl->bindTexture(GL::TEXTURE_2D, texture->_webGLTexture.get());

    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, filters.mag);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, filters.min);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

    _gl->texImage2D(GL::TEXTURE_2D, 0, static_cast<int>(_getRGBABufferInternalSizedFormat(type)),
                  width, height, 0, GL::RGBA, _getWebGLTextureType(type), std::nullopt);

    _gl->framebufferTexture2D(GL::DRAW_FRAMEBUFFER, attachment, GL::TEXTURE_2D,
                            texture->_webGLTexture.get(), 0);

    if (generateMipMaps) {
      _gl->generateMipmap(GL::TEXTURE_2D);
    }

    // Unbind
    _bindTextureDirectly(GL::TEXTURE_2D, nullptr);

    texture->_framebuffer           = std::move(framebuffer);
    texture->_depthStencilBuffer    = std::move(depthStencilBuffer);
    texture->baseWidth              = width;
    texture->baseHeight             = height;
    texture->width                  = width;
    texture->height                 = height;
    texture->isReady                = true;
    texture->samples                = 1;
    texture->generateMipMaps        = generateMipMaps;
    texture->samplingMode           = samplingMode;
    texture->type                   = type;
    texture->_generateDepthBuffer   = generateDepthBuffer;
    texture->_generateStencilBuffer = generateStencilBuffer;
    texture->_attachments           = attachments;

    _internalTexturesCache.emplace_back(texture);
  }

  if (generateDepthTexture && _caps.depthTextureExtension) {
    // Depth texture
    auto depthTexture = InternalTexture::New(this, InternalTexture::DATASOURCE_MULTIRENDERTARGET);

    _gl->activeTexture(GL::TEXTURE0);
    _gl->bindTexture(GL::TEXTURE_2D, depthTexture->_webGLTexture.get());
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
    _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
    _gl->texImage2D(GL::TEXTURE_2D,                                                     //
                  0,                                                                  //
                  webGLVersion() < 2.f ? GL::DEPTH_COMPONENT : GL::DEPTH_COMPONENT16, //
                  width, height,                                                      //
                  0,                                                                  //
                  GL::DEPTH_COMPONENT,                                                //
                  GL::UNSIGNED_SHORT,                                                 //
                  std::nullopt                                                             //
    );

    _gl->framebufferTexture2D(GL::FRAMEBUFFER,                   //
                            GL::DEPTH_ATTACHMENT,              //
                            GL::TEXTURE_2D,                    //
                            depthTexture->_webGLTexture.get(), //
                            0                                  //
    );

    depthTexture->_framebuffer           = std::move(framebuffer);
    depthTexture->baseWidth              = width;
    depthTexture->baseHeight             = height;
    depthTexture->width                  = width;
    depthTexture->height                 = height;
    depthTexture->isReady                = true;
    depthTexture->samples                = 1;
    depthTexture->generateMipMaps        = generateMipMaps;
    depthTexture->samplingMode           = GL::NEAREST;
    depthTexture->_generateDepthBuffer   = generateDepthBuffer;
    depthTexture->_generateStencilBuffer = generateStencilBuffer;

    textures.emplace_back(depthTexture);
    _internalTexturesCache.emplace_back(depthTexture);
  }

  _gl->drawBuffers(attachments);
  _gl->bindRenderbuffer(GL::RENDERBUFFER, nullptr);
  bindUnboundFramebuffer(nullptr);

  resetTextureCache();

  return textures;
}

std::unique_ptr<GL::IGLRenderbuffer>
Engine::_setupFramebufferDepthAttachments(bool generateStencilBuffer, bool generateDepthBuffer,
                                          int width, int height, int samples)
{
  GLRenderBufferPtr depthStencilBuffer = nullptr;

  // Create the depth/stencil buffer
  if (generateStencilBuffer) {
    depthStencilBuffer = _gl->createRenderbuffer();
    _gl->bindRenderbuffer(GL::RENDERBUFFER, depthStencilBuffer);

    if (samples > 1) {
      _gl->renderbufferStorageMultisample(GL::RENDERBUFFER, samples, GL::DEPTH24_STENCIL8, width,
                                          height);
    }
    else {
      _gl->renderbufferStorage(GL::RENDERBUFFER, GL::DEPTH_STENCIL, width, height);
    }

    _gl->framebufferRenderbuffer(GL::FRAMEBUFFER, GL::DEPTH_STENCIL_ATTACHMENT, GL::RENDERBUFFER,
                                 depthStencilBuffer.get());
  }
  else if (generateDepthBuffer) {
    depthStencilBuffer = _gl->createRenderbuffer();
    _gl->bindRenderbuffer(GL::RENDERBUFFER, depthStencilBuffer);

    if (samples > 1) {
      _gl->renderbufferStorageMultisample(GL::RENDERBUFFER, samples, GL::DEPTH_COMPONENT16, width,
                                          height);
    }
    else {
      _gl->renderbufferStorage(GL::RENDERBUFFER, GL::DEPTH_COMPONENT16, width, height);
    }

    _gl->framebufferRenderbuffer(GL::FRAMEBUFFER, GL::DEPTH_ATTACHMENT, GL::RENDERBUFFER,
                                 depthStencilBuffer.get());
  }

  return depthStencilBuffer;
}

unsigned int Engine::updateRenderTargetTextureSampleCount(const InternalTexturePtr& texture,
                                                          unsigned int samples)
{
  if (webGLVersion() < 2.f || !texture) {
    return 1;
  }

  if (texture->samples == samples) {
    return samples;
  }

  samples = std::min(samples, static_cast<unsigned int>(_gl->getParameteri(GL::MAX_SAMPLES)));

  // Dispose previous render buffers
  if (texture->_depthStencilBuffer) {
    _gl->deleteRenderbuffer(texture->_depthStencilBuffer.get());
    texture->_depthStencilBuffer = nullptr;
  }

  if (texture->_MSAAFramebuffer) {
    _gl->deleteFramebuffer(texture->_MSAAFramebuffer.get());
    texture->_MSAAFramebuffer = nullptr;
  }

  if (texture->_MSAARenderBuffer) {
    _gl->deleteRenderbuffer(texture->_MSAARenderBuffer.get());
    texture->_MSAARenderBuffer = nullptr;
  }

  if (samples > 1) {
    auto framebuffer = _gl->createFramebuffer();

    if (!framebuffer) {
      BABYLON_LOG_ERROR("Engine", "Unable to create multi sampled framebuffer")
      return 0;
    }

    texture->_MSAAFramebuffer = std::move(framebuffer);
    bindUnboundFramebuffer(texture->_MSAAFramebuffer);

    auto colorRenderbuffer = _gl->createRenderbuffer();

    if (!colorRenderbuffer) {
      BABYLON_LOG_ERROR("Engine", "Unable to create multi sampled framebuffer")
      return 0;
    }

    _gl->bindRenderbuffer(GL::RENDERBUFFER, colorRenderbuffer);
    _gl->renderbufferStorageMultisample(GL::RENDERBUFFER, static_cast<int>(samples),
                                        _getRGBAMultiSampleBufferFormat(texture->type),
                                        texture->width, texture->height);

    _gl->framebufferRenderbuffer(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0, GL::RENDERBUFFER,
                                 colorRenderbuffer.get());

    texture->_MSAARenderBuffer = std::move(colorRenderbuffer);
  }
  else {
    bindUnboundFramebuffer(texture->_framebuffer);
  }

  texture->samples             = samples;
  texture->_depthStencilBuffer = _setupFramebufferDepthAttachments(
    texture->_generateStencilBuffer, texture->_generateDepthBuffer, texture->width, texture->height,
    static_cast<int>(samples));

  _gl->bindRenderbuffer(GL::RENDERBUFFER, nullptr);
  bindUnboundFramebuffer(nullptr);

  return samples;
}

unsigned int Engine::updateMultipleRenderTargetTextureSampleCount(
  const std::vector<InternalTexturePtr>& textures, unsigned int samples)
{
  if (webGLVersion() < 2.f || textures.empty()) {
    return 1;
  }

  if (textures[0]->samples == samples) {
    return samples;
  }

  samples = std::min(samples, getCaps().maxMSAASamples);

  // Dispose previous render buffers
  if (textures[0]->_depthStencilBuffer) {
    _gl->deleteRenderbuffer(textures[0]->_depthStencilBuffer.get());
    textures[0]->_depthStencilBuffer = nullptr;
  }

  if (textures[0]->_MSAAFramebuffer) {
    _gl->deleteFramebuffer(textures[0]->_MSAAFramebuffer.get());
    textures[0]->_MSAAFramebuffer = nullptr;
  }

  for (const auto& texture : textures) {
    if (texture->_MSAARenderBuffer) {
      _gl->deleteRenderbuffer(texture->_MSAARenderBuffer.get());
      texture->_MSAARenderBuffer = nullptr;
    }
  }

  if (samples > 1) {
    auto framebuffer = _gl->createFramebuffer();

    if (!framebuffer) {
      BABYLON_LOG_ERROR("Engine", "Unable to create multi sampled framebuffer")
      return 0;
    }

    bindUnboundFramebuffer(framebuffer);

    auto depthStencilBuffer = _setupFramebufferDepthAttachments(
      textures[0]->_generateStencilBuffer, textures[0]->_generateDepthBuffer, textures[0]->width,
      textures[0]->height, static_cast<int>(samples));

    Uint32Array attachments;

    for (size_t i = 0; i < textures.size(); ++i) {
      auto iStr       = std::to_string(i);
      auto& texture   = textures[i];
      auto attachment = (*_gl)[webGLVersion() > 1.f ? "COLOR_ATTACHMENT" + iStr :
                                                  "COLOR_ATTACHMENT" + iStr + "_WEBGL"];

      auto colorRenderbuffer = _gl->createRenderbuffer();

      if (!colorRenderbuffer) {
        BABYLON_LOG_ERROR("Engine", "Unable to create multi sampled framebuffer")
        return 0;
      }

      _gl->bindRenderbuffer(GL::RENDERBUFFER, colorRenderbuffer);
      _gl->renderbufferStorageMultisample(GL::RENDERBUFFER, static_cast<int>(samples),
                                        _getRGBAMultiSampleBufferFormat(texture->type),
                                        texture->width, texture->height);

      _gl->framebufferRenderbuffer(GL::FRAMEBUFFER, attachment, GL::RENDERBUFFER,
                                 colorRenderbuffer.get());

      texture->_MSAAFramebuffer    = framebuffer;
      texture->_MSAARenderBuffer   = std::move(colorRenderbuffer);
      texture->samples             = samples;
      texture->_depthStencilBuffer = std::move(depthStencilBuffer);
      _gl->bindRenderbuffer(GL::RENDERBUFFER, nullptr);
      attachments.emplace_back(attachment);
    }
    _gl->drawBuffers(attachments);
  }
  else {
    bindUnboundFramebuffer(textures[0]->_framebuffer);
  }

  bindUnboundFramebuffer(nullptr);

  return samples;
}

void Engine::_uploadCompressedDataToTextureDirectly(const InternalTexturePtr& /*texture*/,
                                                    unsigned int /*internalFormat*/,
                                                    float /*width*/, float /*height*/,
                                                    const Uint8Span_ro& /*data*/,
                                                    unsigned int /*faceIndex*/, int /*lod*/)
{
}

void Engine::_uploadDataToTextureDirectly(const InternalTexturePtr& texture,
                                          const Uint8Span_ro& imageData, unsigned int faceIndex,
                                          int lod)
{
  auto textureType    = _getWebGLTextureType(texture->type);
  auto format         = _getInternalFormat(texture->format);
  auto internalFormat = static_cast<int>(_getRGBABufferInternalSizedFormat(texture->type, format));

  _unpackFlipY(texture->invertY);

  GL::GLenum target = GL::TEXTURE_2D;
  if (texture->isCube) {
    target = GL::TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
  }

  const auto lodMaxWidth  = static_cast<int>(std::round(Scalar::Log2(texture->width)));
  const auto lodMaxHeight = static_cast<int>(std::round(Scalar::Log2(texture->height)));
  const auto width        = static_cast<int>(std::pow(2, std::max(lodMaxWidth - lod, 0)));
  const auto height       = static_cast<int>(std::pow(2, std::max(lodMaxHeight - lod, 0)));

  _gl->texImage2D(target, lod, internalFormat, width, height, 0, format, textureType,
                  imageData);
}

void Engine::_uploadArrayBufferViewToTexture(const InternalTexturePtr& texture,
                                             const Uint8Span_ro& imageData, unsigned int faceIndex,
                                             int lod)
{
  auto bindTarget = texture->isCube ? GL::TEXTURE_CUBE_MAP : GL::TEXTURE_2D;

  _bindTextureDirectly(bindTarget, texture, true);

  _uploadDataToTextureDirectly(texture, imageData, faceIndex, lod);

  _bindTextureDirectly(bindTarget, nullptr, true);
}

void Engine::_uploadImageToTexture(const InternalTexturePtr& texture, const Image& image,
                                   unsigned int faceIndex, int lod)
{
  auto textureType    = _getWebGLTextureType(texture->type);
  auto format         = _getInternalFormat(texture->format);
  auto internalFormat = _getRGBABufferInternalSizedFormat(texture->type, format);

  auto bindTarget = texture->isCube ? GL::TEXTURE_CUBE_MAP : GL::TEXTURE_2D;

  _bindTextureDirectly(bindTarget, texture, true);
  _unpackFlipY(texture->invertY);

  GL::GLenum target = GL::TEXTURE_2D;
  if (texture->isCube) {
    target = static_cast<GL::GLenum>(GL::TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex);
  }

  _gl->texImage2D(target, lod, static_cast<int>(internalFormat), image.width, image.height, 0,
                  format, textureType, stl_util::as_span(image.data));
  _bindTextureDirectly(bindTarget, nullptr, true);
}

InternalTexturePtr Engine::createRenderTargetCubeTexture(const ISize& size,
                                                         const RenderTargetCreationOptions& options)
{
  RenderTargetCreationOptions fullOptions;
  fullOptions.generateMipMaps
    = options.generateMipMaps.has_value() ? options.generateMipMaps.value() : true;
  fullOptions.generateDepthBuffer
    = options.generateDepthBuffer.has_value() ? options.generateDepthBuffer.value() : true;
  fullOptions.generateStencilBuffer
    = options.generateStencilBuffer.has_value() ? options.generateStencilBuffer.value() : false;
  fullOptions.type
    = options.type.has_value() ? options.type.value() : Constants::TEXTURETYPE_UNSIGNED_INT;
  fullOptions.samplingMode = options.samplingMode.has_value() ?
                               options.samplingMode.value() :
                               Constants::TEXTURE_TRILINEAR_SAMPLINGMODE;
  fullOptions.format
    = options.format.has_value() ? options.format.value() : Constants::TEXTUREFORMAT_RGBA;
  fullOptions.generateStencilBuffer
    = fullOptions.generateDepthBuffer.value() && fullOptions.generateStencilBuffer.value();

  if (fullOptions.type.value() == Constants::TEXTURETYPE_FLOAT
      && !_caps.textureFloatLinearFiltering) {
    // if floating point linear (_gl->FLOAT) then force to NEAREST_SAMPLINGMODE
    fullOptions.samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
  }
  else if (fullOptions.type.value() == Constants::TEXTURETYPE_HALF_FLOAT
           && !_caps.textureHalfFloatLinearFiltering) {
    // if floating point linear (HALF_FLOAT) then force to NEAREST_SAMPLINGMODE
    fullOptions.samplingMode = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
  }

  auto texture = InternalTexture::New(this, InternalTexture::DATASOURCE_RENDERTARGET);
  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);

  auto filters = Engine::_getSamplingParameters(fullOptions.samplingMode.value(),
                                                fullOptions.generateMipMaps.value());

  if (fullOptions.type.value() == Constants::TEXTURETYPE_FLOAT && !_caps.textureFloat) {
    fullOptions.type = Constants::TEXTURETYPE_UNSIGNED_INT;
    BABYLON_LOG_WARN("Engine",
                     "Float textures are not supported. Cube render target "
                     "forced to TEXTURETYPE_UNESIGNED_BYTE type")
  }

  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MIN_FILTER, filters.min);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

  for (unsigned int face = 0; face < 6; ++face) {
    _gl->texImage2D((GL::TEXTURE_CUBE_MAP_POSITIVE_X + face), 0,
                  static_cast<GL::GLint>(_getRGBABufferInternalSizedFormat(
                    fullOptions.type.value(), fullOptions.format.value())),
                  size.width, size.height, 0, _getInternalFormat(fullOptions.format.value()),
                  _getWebGLTextureType(fullOptions.type.value()), std::nullopt);
  }

  // Create the framebuffer
  auto framebuffer = _gl->createFramebuffer();
  bindUnboundFramebuffer(framebuffer);

  texture->_depthStencilBuffer = _setupFramebufferDepthAttachments(
    fullOptions.generateStencilBuffer.value(), fullOptions.generateDepthBuffer.value(), size.width,
    size.height);

  // Mipmaps
  if (texture->generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_CUBE_MAP);
  }

  // Unbind
  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
  _gl->bindRenderbuffer(GL::RENDERBUFFER, nullptr);
  bindUnboundFramebuffer(nullptr);

  texture->_framebuffer           = std::move(framebuffer);
  texture->width                  = size.width;
  texture->height                 = size.height;
  texture->isReady                = true;
  texture->isCube                 = true;
  texture->samples                = 1;
  texture->generateMipMaps        = fullOptions.generateMipMaps.value();
  texture->samplingMode           = fullOptions.samplingMode.value();
  texture->type                   = fullOptions.type.value();
  texture->format                 = fullOptions.format.value();
  texture->_generateDepthBuffer   = fullOptions.generateDepthBuffer.value();
  texture->_generateStencilBuffer = fullOptions.generateStencilBuffer.value();

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

InternalTexturePtr Engine::createPrefilteredCubeTexture(
  const std::string& rootUrl, Scene* scene, float lodScale, float lodOffset,
  const std::function<void(const std::optional<CubeTextureData>& data)>& onLoad,
  const std::function<void(const std::string& message, const std::string& exception)>& onError,
  unsigned int format, const std::string& forcedExtension, bool createPolynomials)
{
  const auto callback = [=](const std::optional<CubeTextureData>& loadData) {
    if (!loadData.has_value()) {
      if (onLoad) {
        onLoad(std::nullopt);
      }
      return;
    }

    auto texture = loadData->texture;
    if (!createPolynomials) {
      texture->_sphericalPolynomial = std::make_shared<SphericalPolynomial>();
    }
    else if (loadData->info.sphericalPolynomial) {
      texture->_sphericalPolynomial = loadData->info.sphericalPolynomial;
    }
    texture->_dataSource = InternalTexture::DATASOURCE_CUBEPREFILTERED;

    if (_caps.textureLOD) {
      // Do not add extra process if texture lod is supported.
      if (onLoad) {
        onLoad(loadData);
      }
      return;
    }

    unsigned int mipSlices = 3;

    auto width = loadData->width;
    if (!width) {
      return;
    }

    std::vector<BaseTexturePtr> textures;
    for (unsigned int i = 0; i < mipSlices; ++i) {
      // compute LOD from even spacing in smoothness (matching shader
      // calculation)
      float smoothness = static_cast<float>(i) / static_cast<float>(mipSlices - 1);
      float roughness  = 1.f - smoothness;

      float minLODIndex = lodOffset;                                  // roughness = 0
      float maxLODIndex = Scalar::Log2(width) * lodScale + lodOffset; // roughness = 1

      float lodIndex = minLODIndex + (maxLODIndex - minLODIndex) * roughness;
      int mipmapIndex
        = static_cast<int>(std::round(std::min(std::max(lodIndex, 0.f), maxLODIndex)));

      auto glTextureFromLod    = InternalTexture::New(this, InternalTexture::DATASOURCE_TEMP);
      glTextureFromLod->type   = texture->type;
      glTextureFromLod->format = texture->format;
      glTextureFromLod->width  = static_cast<int>(
        std::pow(2.f, std::max(static_cast<float>(Scalar::Log2(width) - mipmapIndex), 0.f)));
      glTextureFromLod->height = glTextureFromLod->width;
      glTextureFromLod->isCube = true;
      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, glTextureFromLod, true);

      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
      _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

      if (loadData->isDDS) {
#if 0
        auto info  = loadData->info;
        auto& data = loadData->data;
        _unpackFlipY(info.isCompressed);

        DDSTools::UploadDDSLevels(this, glTextureFromLod, data, info, true, 6u,
                                  mipmapIndex);
#endif
      }
      else {
        BABYLON_LOG_WARN("Engine", "DDS is the only prefiltered cube map supported so far.")
      }

      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);

      // Wrap in a base texture for easy binding.
      auto lodTexture      = BaseTexture::New(scene);
      lodTexture->isCube   = true;
      lodTexture->_texture = glTextureFromLod;

      glTextureFromLod->isReady = true;
      textures.emplace_back(lodTexture);
    }

    texture->_lodTextureHigh = textures[2];
    texture->_lodTextureMid  = textures[1];
    texture->_lodTextureLow  = textures[0];

    if (onLoad) {
      onLoad(loadData);
    }
  };

  return createCubeTexture(rootUrl, scene, {}, false, callback, onError, format, forcedExtension,
                           createPolynomials, lodScale, lodOffset);
}

InternalTexturePtr Engine::createCubeTexture(
  std::string rootUrl, Scene* scene, const std::vector<std::string>& files, bool noMipmap,
  const std::function<void(const std::optional<CubeTextureData>& data)>& onLoad,
  const std::function<void(const std::string& message, const std::string& exception)>& onError,
  unsigned int format, const std::string& forcedExtension, bool createPolynomials, float lodScale,
  float lodOffset, const InternalTexturePtr& fallback)
{
  auto texture = fallback ? fallback : InternalTexture::New(this, InternalTexture::DATASOURCE_CUBE);
  texture->isCube               = true;
  texture->url                  = rootUrl;
  texture->generateMipMaps      = !noMipmap;
  texture->_lodGenerationScale  = lodScale;
  texture->_lodGenerationOffset = lodOffset;

  if (!_doNotHandleContextLost) {
    texture->_extension = forcedExtension;
    texture->_files     = files;
  }

  auto lastDot = String::lastIndexOf(rootUrl, ".");
  auto extension
    = !forcedExtension.empty() ?
        forcedExtension :
        (lastDot > -1 ? String::toLowerCase(rootUrl.substr(static_cast<unsigned long>(lastDot))) :
                        "");

  IInternalTextureLoaderPtr loader = nullptr;
  for (const auto& availableLoader : Engine::_TextureLoaders) {
    if (availableLoader->canLoad(extension, _textureFormatInUse, fallback, false, false)) {
      loader = availableLoader;
      break;
    }
  }

  const auto onInternalError
    = [](const std::string& /*message*/, const std::string & /*exception*/) -> void {

  };

  if (loader) {
    rootUrl = loader->transformUrl(rootUrl, _textureFormatInUse);

    const auto onloaddata = [=](const std::variant<std::string, ArrayBuffer>& data,
                                const std::string & /*responseURL*/) -> void {
      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);
      loader->loadCubeData(data, texture, createPolynomials, onLoad, onError);
    };
    if (!files.empty() && files.size() == 6) {
      if (loader->supportCascades()) {
        _cascadeLoadFiles(scene, onloaddata, files, onError);
      }
      else if (onError) {
        onError("Textures type does not support cascades.", "");
      }
    }
    else {
      _loadFile(rootUrl, onloaddata, nullptr, true, onInternalError);
    }
  }
  else {
    if (files.empty()) {
      BABYLON_LOG_ERROR("Engine", "Cannot load cubemap because files were not defined")
    }

    _cascadeLoadImgs(
      rootUrl, scene,
      [=](const std::vector<Image>& imgs) {
        auto width = needPOTTextures() ?
                       Engine::GetExponentOfTwo(imgs[0].width, _caps.maxCubemapTextureSize) :
                       imgs[0].width;
        auto height = width;

        _prepareWorkingCanvas();

        if (!_workingCanvas /*|| !_workingContext*/) {
          return;
        }
        _workingCanvas->width  = width;
        _workingCanvas->height = height;

        std::vector<GL::GLenum> faces{
          GL::TEXTURE_CUBE_MAP_POSITIVE_X, GL::TEXTURE_CUBE_MAP_POSITIVE_Y,
          GL::TEXTURE_CUBE_MAP_POSITIVE_Z, GL::TEXTURE_CUBE_MAP_NEGATIVE_X,
          GL::TEXTURE_CUBE_MAP_NEGATIVE_Y, GL::TEXTURE_CUBE_MAP_NEGATIVE_Z};

        _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);
        _unpackFlipY(false);

        auto internalFormat = format ? _getInternalFormat(format) : GL::RGBA;
        for (size_t index = 0; index < faces.size(); index++) {
#if 0
          _workingContext->drawImage(imgs[index], 0, 0, imgs[index].width,
                                     imgs[index].height, 0, 0, width, height);
          _gl->texImage2D(faces[index], 0, internalFormat, internalFormat,
                        GL::UNSIGNED_BYTE, _workingCanvas);
#else
          _gl->texImage2D(faces[index], 0, static_cast<int>(internalFormat), imgs[index].width,
                        imgs[index].height, 0, GL::RGBA, GL::UNSIGNED_BYTE,
                        stl_util::as_span(imgs[index].data));
#endif
        }

        if (!noMipmap) {
          _gl->generateMipmap(GL::TEXTURE_CUBE_MAP);
        }

        _setCubeMapTextureParams(!noMipmap);

        texture->width   = width;
        texture->height  = height;
        texture->isReady = true;
        if (format) {
          texture->format = format;
        }

        texture->onLoadedObservable.notifyObservers(texture.get());
        texture->onLoadedObservable.clear();

        if (onLoad) {
          onLoad(std::nullopt);
        }
      },
      files, onError);
  }

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

void Engine::_setCubeMapTextureParams(bool loadMipmap)
{
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MIN_FILTER,
                     loadMipmap ? GL::LINEAR_MIPMAP_LINEAR : GL::LINEAR);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);

  // resetTextureCache();
}

void Engine::updateRawCubeTexture(const InternalTexturePtr& texture,
                                  const std::vector<ArrayBufferView>& data, unsigned int format,
                                  unsigned int type, bool invertY, const std::string& compression,
                                  unsigned int level)
{
  texture->_bufferViewArray = data;
  texture->format           = format;
  texture->type             = type;
  texture->invertY          = invertY;
  texture->_compression     = compression;

  auto textureType        = _getWebGLTextureType(type);
  auto internalFormat     = _getInternalFormat(format);
  auto internalSizedFomat = _getRGBABufferInternalSizedFormat(type);

  auto needConversion = false;
  if (internalFormat == GL::RGB) {
    internalFormat = GL::RGBA;
    needConversion = true;
  }

  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);
  _unpackFlipY(invertY);

  if (texture->width % 4 != 0) {
    _gl->pixelStorei(GL::UNPACK_ALIGNMENT, 1);
  }

  const std::vector<unsigned int> facesIndex
    = {GL::TEXTURE_CUBE_MAP_POSITIVE_X, GL::TEXTURE_CUBE_MAP_NEGATIVE_X,
       GL::TEXTURE_CUBE_MAP_POSITIVE_Y, GL::TEXTURE_CUBE_MAP_NEGATIVE_Y,
       GL::TEXTURE_CUBE_MAP_POSITIVE_Z, GL::TEXTURE_CUBE_MAP_NEGATIVE_Z};

  // Data are known to be in +X +Y +Z -X -Y -Z
  for (std::size_t index = 0; index < facesIndex.size(); ++index) {
    const auto& faceData = data[index];

    if (!compression.empty()) {
#if 0
      _gl->compressedTexImage2D(facesIndex[index], level,
                                getCaps().s3tc[compression], texture->width,
                                texture->height, 0, faceData.uint8Array);
#endif
    }
    else {
      if (needConversion) {
        auto convertedFaceData
          = _convertRGBtoRGBATextureData(faceData, texture->width, texture->height, type);
        _gl->texImage2D(facesIndex[index], static_cast<int>(level),
                      static_cast<int>(internalSizedFomat), texture->width, texture->height, 0,
                      internalFormat, textureType, convertedFaceData.uint8Span());
      }
      else {
        _gl->texImage2D(facesIndex[index], static_cast<int>(level),
                      static_cast<int>(internalSizedFomat), texture->width, texture->height, 0,
                      internalFormat, textureType, faceData.uint8Span());
      }
    }
  }

  auto isPot = !needPOTTextures()
               || (Tools::IsExponentOfTwo(static_cast<size_t>(texture->width))
                   && Tools::IsExponentOfTwo(static_cast<size_t>(texture->height)));
  if (isPot && texture->generateMipMaps && level == 0) {
    _gl->generateMipmap(GL::TEXTURE_CUBE_MAP);
  }
  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);

  // resetTextureCache();
  texture->isReady = true;
}

InternalTexturePtr Engine::createRawCubeTexture(const std::vector<ArrayBufferView>& data, int size,
                                                unsigned int format, unsigned int type,
                                                bool generateMipMaps, bool invertY,
                                                unsigned int samplingMode,
                                                const std::string& compression)
{
  auto texture    = InternalTexture::New(this, InternalTexture::DATASOURCE_CUBERAW);
  texture->isCube = true;
  texture->format = format;
  texture->type   = type;
  if (!_doNotHandleContextLost) {
    texture->_bufferViewArray = data;
  }

  auto textureType = _getWebGLTextureType(type);

  /*
  // code from src/Engines/Extensions/engine.rawTexture.ts
  // however those variables are never used
  auto internalFormat = _getInternalFormat(format);

  if (internalFormat == GL::RGB) {
    internalFormat = GL::RGBA;
  }
  */

  // Mipmap generation needs a sized internal format that is both
  // color-renderable and texture-filterable
  if (textureType == GL::FLOAT && !_caps.textureFloatLinearFiltering) {
    generateMipMaps = false;
    samplingMode    = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
    BABYLON_LOG_WARN("Engine",
                     "Float texture filtering is not supported. Mipmap "
                     "generation and sampling mode are forced to false and "
                     "TEXTURE_NEAREST_SAMPLINGMODE, respectively.")
  }
  else if (textureType == GL::HALF_FLOAT_OES && !_caps.textureHalfFloatLinearFiltering) {
    generateMipMaps = false;
    samplingMode    = Constants::TEXTURE_NEAREST_SAMPLINGMODE;
    BABYLON_LOG_WARN("Engine",
                     "Half float texture filtering is not supported. Mipmap "
                     "generation and sampling mode are forced to false and "
                     "TEXTURE_NEAREST_SAMPLINGMODE, respectively.")
  }
  else if (textureType == GL::FLOAT && !_caps.textureFloatRender) {
    generateMipMaps = false;
    BABYLON_LOG_WARN("Engine",
                     "Render to float textures is not supported. Mipmap "
                     "generation forced to false.")
  }
  else if (textureType == GL::HALF_FLOAT && !_caps.colorBufferFloat) {
    generateMipMaps = false;
    BABYLON_LOG_WARN("Engine",
                     "Render to half float textures is not supported. Mipmap "
                     "generation forced to false.")
  }

  auto width  = size;
  auto height = width;

  texture->width  = width;
  texture->height = height;

  // Double check on POT to generate Mips.
  auto isPot = !needPOTTextures()
               || (Tools::IsExponentOfTwo(static_cast<size_t>(texture->width))
                   && Tools::IsExponentOfTwo(static_cast<size_t>(texture->height)));
  if (!isPot) {
    generateMipMaps = false;
  }

  // Upload data if needed. The texture won't be ready until then.
  if (!data.empty()) {
    updateRawCubeTexture(texture, data, format, type, invertY, compression);
  }

  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);

  // Filters
  if (!data.empty() && generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_CUBE_MAP);
  }

  auto filters = _getSamplingParameters(samplingMode, generateMipMaps);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_MIN_FILTER, filters.min);

  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
  _gl->texParameteri(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
  _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);

  texture->generateMipMaps = generateMipMaps;

  return texture;
}

InternalTexturePtr Engine::createRawCubeTextureFromUrl(
  const std::string& url, Scene* scene, int size, unsigned int format, unsigned int type,
  bool noMipmap,
  const std::function<ArrayBufferViewArray(const ArrayBuffer& arrayBuffer)>& callback,
  const std::function<std::vector<ArrayBufferViewArray>(const ArrayBufferViewArray& faces)>&
    mipmapGenerator,
  const std::function<void()>& onLoad,
  const std::function<void(const std::string& message, const std::string& exception)>& onError,
  unsigned int samplingMode, bool invertY)
{
  auto texture = createRawCubeTexture({}, size, format, type, !noMipmap, invertY, samplingMode);
  scene->_addPendingData(texture);
  texture->url = url;
  _internalTexturesCache.emplace_back(texture);

  const auto onerror = [=](const std::string& message, const std::string& exception) -> void {
    scene->_removePendingData(texture);
    if (onError) {
      onError(message, exception);
    }
  };

  const auto internalCallback = [=](const std::variant<std::string, ArrayBuffer>& data,
                                    const std::string & /*responseURL*/) -> void {
    auto width = texture->width;

    if (!std::holds_alternative<ArrayBuffer>(data)) {
      return;
    }

    auto faceDataArrays = callback(std::get<ArrayBuffer>(data));

    if (faceDataArrays.empty()) {
      return;
    }

    if (mipmapGenerator) {
      auto textureType        = _getWebGLTextureType(type);
      auto internalFormat     = _getInternalFormat(format);
      auto internalSizedFomat = _getRGBABufferInternalSizedFormat(type);

      auto needConversion = false;
      if (internalFormat == GL::RGB) {
        internalFormat = GL::RGBA;
        needConversion = true;
      }

      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, texture, true);
      _unpackFlipY(false);

      const auto& mipData = mipmapGenerator(faceDataArrays);
      for (size_t level = 0; level < mipData.size(); level++) {
        auto mipSize = width >> level;

        for (unsigned int faceIndex = 0; faceIndex < 6; ++faceIndex) {
          auto mipFaceData = mipData[level][faceIndex];
          if (needConversion) {
            mipFaceData = _convertRGBtoRGBATextureData(mipFaceData, mipSize, mipSize, type);
          }
          _gl->texImage2D(faceIndex, static_cast<int>(level), static_cast<int>(internalSizedFomat),
                        mipSize, mipSize, 0, internalFormat, textureType,
                        mipFaceData.uint8Span());
        }
      }

      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
    }
    else {
      updateRawCubeTexture(texture, faceDataArrays, format, type, invertY);
    }

    texture->isReady = true;
    // resetTextureCache();
    scene->_removePendingData(texture);

    if (onLoad) {
      onLoad();
    }
  };

  _loadFile(
    url,
    [=](const std::variant<std::string, ArrayBuffer>& data,
        const std::string& responseURL) -> void { internalCallback(data, responseURL); },
    nullptr, true, onerror);

  return texture;
}

void Engine::updateRawTexture3D(
  const InternalTexturePtr& texture,
  const Uint8Span_ro& data,
  unsigned int format, bool invertY, const std::string& compression,
  unsigned int textureType)
{

  auto internalType       = _getWebGLTextureType(textureType);
  auto internalFormat     = _getInternalFormat(format);
  auto internalSizedFomat = _getRGBABufferInternalSizedFormat(textureType, format);

  _bindTextureDirectly(GL::TEXTURE_3D, texture, true);

  _unpackFlipY(invertY);

  if (!_doNotHandleContextLost) {
    std::copy(data.begin(), data.end(), std::back_inserter(texture->_bufferView));
    texture->format       = format;
    texture->invertY      = invertY;
    texture->_compression = compression;
  }

  if (texture->width % 4 != 0) {
    _gl->pixelStorei(GL::UNPACK_ALIGNMENT, 1);
  }

  if (!compression.empty() && !data.empty()) {
    // __gl->compressedTexImage3D(GL::TEXTURE_3D, 0,
    // (<any>getCaps().s3tc)[compression], texture->width, texture->height,
    // texture->depth, 0, data);
  }
  else {
    _gl->texImage3D(GL::TEXTURE_3D, 0, static_cast<int>(internalSizedFomat), texture->width,
                    texture->height, texture->depth, 0, internalFormat, internalType,
                    data);
  }

  if (texture->generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_3D);
  }
  _bindTextureDirectly(GL::TEXTURE_3D, nullptr);
  // resetTextureCache();
  texture->isReady = true;
}

InternalTexturePtr Engine::createRawTexture3D(const Uint8Span_ro& data, int width, int height,
                                              int depth, unsigned int format, bool generateMipMaps,
                                              bool invertY, unsigned int samplingMode,
                                              const std::string& compression,
                                              unsigned int textureType)
{
  auto texture             = InternalTexture::New(this, InternalTexture::DATASOURCE_RAW3D);
  texture->baseWidth       = width;
  texture->baseHeight      = height;
  texture->baseDepth       = depth;
  texture->width           = width;
  texture->height          = height;
  texture->depth           = depth;
  texture->format          = format;
  texture->type            = textureType;
  texture->generateMipMaps = generateMipMaps;
  texture->samplingMode    = samplingMode;
  texture->is3D            = true;

  if (!_doNotHandleContextLost) {
    std::copy(data.begin(), data.end(), std::back_inserter(texture->_bufferView));
  }

  updateRawTexture3D(texture, data, format, invertY, compression, textureType);
  _bindTextureDirectly(GL::TEXTURE_3D, texture, true);

  // Filters
  auto filters = _getSamplingParameters(samplingMode, generateMipMaps);

  _gl->texParameteri(GL::TEXTURE_3D, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_3D, GL::TEXTURE_MIN_FILTER, filters.min);

  if (generateMipMaps) {
    _gl->generateMipmap(GL::TEXTURE_3D);
  }

  _bindTextureDirectly(GL::TEXTURE_3D, nullptr);

  _internalTexturesCache.emplace_back(texture);

  return texture;
}

InternalTexturePtr Engine::createMultiviewRenderTargetTexture(int width, int height)
{
  if (!getCaps().multiview) {
    BABYLON_LOG_ERROR("Engine", "Multiview is not supported")
    return nullptr;
  }

  auto internalTexture    = InternalTexture::New(this, InternalTexture::DATASOURCE_UNKNOWN, true);
  internalTexture->width  = width;
  internalTexture->height = height;
  internalTexture->_framebuffer = _gl->createFramebuffer();

  internalTexture->_colorTextureArray = _gl->createTexture();
  _gl->bindTexture(GL::TEXTURE_2D_ARRAY, internalTexture->_colorTextureArray.get());
  _gl->texStorage3D(GL::TEXTURE_2D_ARRAY, 1, GL::RGBA8, width, height, 2);

  internalTexture->_depthStencilTextureArray = _gl->createTexture();
  _gl->bindTexture(GL::TEXTURE_2D_ARRAY, internalTexture->_depthStencilTextureArray.get());
  _gl->texStorage3D(GL::TEXTURE_2D_ARRAY, 1, GL::DEPTH32F_STENCIL8, width, height, 2);
  internalTexture->isReady = true;
  return internalTexture;
}

void Engine::bindMultiviewFramebuffer(const InternalTexturePtr& multiviewTexture)
{
  bindFramebuffer(multiviewTexture, std::nullopt, std::nullopt, std::nullopt, true);
  _gl->bindFramebuffer(GL::DRAW_FRAMEBUFFER, multiviewTexture->_framebuffer.get());
  if (multiviewTexture->_colorTextureArray && multiviewTexture->_depthStencilTextureArray) {
    _gl->framebufferTextureMultiviewOVR(GL::DRAW_FRAMEBUFFER, GL::COLOR_ATTACHMENT0,
                                      multiviewTexture->_colorTextureArray.get(), 0, 0, 2);
    _gl->framebufferTextureMultiviewOVR(GL::DRAW_FRAMEBUFFER, GL::DEPTH_STENCIL_ATTACHMENT,
                                      multiviewTexture->_depthStencilTextureArray.get(), 0, 0, 2);
  }
  else {
    BABYLON_LOG_ERROR("Engine", "Invalid multiview frame buffer")
  }
}

void Engine::_prepareWebGLTextureContinuation(const InternalTexturePtr& texture, Scene* scene,
                                              bool noMipmap, bool isCompressed,
                                              unsigned int samplingMode)
{
  if (!_gl) {
    return;
  }

  auto filters = _getSamplingParameters(samplingMode, !noMipmap);

  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, filters.mag);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, filters.min);

  if (!noMipmap && !isCompressed) {
    _gl->generateMipmap(GL::TEXTURE_2D);
  }

  _bindTextureDirectly(GL::TEXTURE_2D, nullptr);

  // resetTextureCache();
  if (scene) {
    scene->_removePendingData(texture);
  }

  texture->onLoadedObservable.notifyObservers(texture.get());
  texture->onLoadedObservable.clear();
}

void Engine::_prepareWebGLTexture(
  const InternalTexturePtr& texture, Scene* scene, int width, int height,
  std::optional<bool> invertY, bool noMipmap, bool isCompressed,
  const std::function<bool(int width, int height,
                           const std::function<void()>& continuationCallback)>& processFunction,
  unsigned int samplingMode)
{
  auto maxTextureSize = getCaps().maxTextureSize;
  auto potWidth       = std::min(
    maxTextureSize, needPOTTextures() ? Engine::GetExponentOfTwo(width, maxTextureSize) : width);
  auto potHeight = std::min(
    maxTextureSize, needPOTTextures() ? Engine::GetExponentOfTwo(height, maxTextureSize) : height);

  if (!_gl) {
    return;
  }

  if (!texture->_webGLTexture) {
    // resetTextureCache();
    if (scene) {
      scene->_removePendingData(texture);
    }

    return;
  }

  _bindTextureDirectly(GL::TEXTURE_2D, texture, true);

  _unpackFlipY(static_cast<bool>(invertY));

  texture->baseWidth  = width;
  texture->baseHeight = height;
  texture->width      = potWidth;
  texture->height     = potHeight;
  texture->isReady    = true;

  if (processFunction(potWidth, potHeight, [=]() {
        _prepareWebGLTextureContinuation(texture, scene, noMipmap, isCompressed, samplingMode);
      })) {
    // Returning as texture needs extra async steps
    return;
  }

  _prepareWebGLTextureContinuation(texture, scene, noMipmap, isCompressed, samplingMode);
}

ArrayBufferView Engine::_convertRGBtoRGBATextureData(const ArrayBufferView& rgbData, int width,
                                                     int height, unsigned int textureType)
{
  // Create new RGBA data container.
  if (textureType == Constants::TEXTURETYPE_FLOAT) {
    Float32Array rgbaData(static_cast<size_t>(width * height * 4));
    // Convert each pixel.
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        auto index    = static_cast<size_t>((y * width + x) * 3);
        auto newIndex = static_cast<size_t>((y * width + x) * 4);

        // Map Old Value to new value.
        rgbaData[newIndex + 0] = rgbData.float32Span()[index + 0];
        rgbaData[newIndex + 1] = rgbData.float32Span()[index + 1];
        rgbaData[newIndex + 2] = rgbData.float32Span()[index + 2];

        // Add fully opaque alpha channel.
        rgbaData[newIndex + 3] = 1.f;
      }
    }
    return ArrayBufferView(rgbaData);
  }
  else {
    Uint32Array rgbaData(static_cast<size_t>(width * height * 4));
    // Convert each pixel.
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        auto index    = static_cast<size_t>((y * width + x) * 3);
        auto newIndex = static_cast<size_t>((y * width + x) * 4);

        // Map Old Value to new value.
        rgbaData[newIndex + 0] = rgbData.uint32Span()[index + 0];
        rgbaData[newIndex + 1] = rgbData.uint32Span()[index + 1];
        rgbaData[newIndex + 2] = rgbData.uint32Span()[index + 2];

        // Add fully opaque alpha channel.
        rgbaData[newIndex + 3] = 1;
      }
    }
    return ArrayBufferView(rgbaData);
  }
}

void Engine::_releaseFramebufferObjects(InternalTexture* texture)
{
  if (texture->_framebuffer) {
    _gl->deleteFramebuffer(texture->_framebuffer.get());
    texture->_framebuffer = nullptr;
  }

  if (texture->_depthStencilBuffer) {
    _gl->deleteRenderbuffer(texture->_depthStencilBuffer.get());
    texture->_depthStencilBuffer = nullptr;
  }

  if (texture->_MSAAFramebuffer) {
    _gl->deleteFramebuffer(texture->_MSAAFramebuffer.get());
    texture->_MSAAFramebuffer = nullptr;
  }

  if (texture->_MSAARenderBuffer) {
    _gl->deleteRenderbuffer(texture->_MSAARenderBuffer.get());
    texture->_MSAARenderBuffer.reset(nullptr);
  }
}

void Engine::_releaseTexture(InternalTexture* texture)
{
  _releaseFramebufferObjects(texture);

  _gl->deleteTexture(texture->_webGLTexture.get());

  // Unbind channels
  unbindAllTextures();

  stl_util::remove_vector_elements_equal_sharedptr(_internalTexturesCache, texture);

  // Integrated fixed lod samplers.
  if (texture->_lodTextureHigh) {
    texture->_lodTextureHigh->dispose();
  }
  if (texture->_lodTextureMid) {
    texture->_lodTextureMid->dispose();
  }
  if (texture->_lodTextureLow) {
    texture->_lodTextureLow->dispose();
  }

  // Set output texture of post process to null if the texture has been
  // released/disposed
  for (auto& scene : scenes) {
    for (auto& postProcess : scene->postProcesses) {
      if (postProcess->_outputTexture.get() == texture) {
        postProcess->_outputTexture = nullptr;
      }
    }
    for (auto& camera : scene->cameras) {
      for (auto& postProcess : camera->_postProcesses) {
        if (postProcess) {
          if (postProcess->_outputTexture.get() == texture) {
            postProcess->_outputTexture = nullptr;
          }
        }
      }
    }
  }
}

void Engine::setProgram(GL::IGLProgram* program)
{
  if (_currentProgram != program) {
    _gl->useProgram(program);
    _currentProgram = program;
  }
}

void Engine::bindSamplers(Effect& effect)
{
  setProgram(effect.getProgram());

  const auto& samplers = effect.getSamplers();
  for (size_t index = 0; index < samplers.size(); ++index) {
    auto uniform = effect.getUniform(samplers[index]);

    if (uniform) {
      _boundUniforms[static_cast<int>(index)] = uniform;
    }
  }
  _currentEffect = nullptr;
}

void Engine::_moveBoundTextureOnTop(const InternalTexturePtr& internalTexture)
{
  if (disableTextureBindingOptimization
      || _lastBoundInternalTextureTracker->previous == internalTexture) {
    return;
  }

  // Remove
  _linkTrackers(internalTexture->previous, internalTexture->next);

  // Bind last to it
  _linkTrackers(_lastBoundInternalTextureTracker->previous, internalTexture);

  // Bind to dummy
  _linkTrackers(internalTexture, _lastBoundInternalTextureTracker);
}

void Engine::_linkTrackers(const IInternalTextureTrackerPtr& previous,
                           const IInternalTextureTrackerPtr& next)
{
  if (previous) {
    previous->next = next;
  }
  if (next) {
    next->previous = previous;
  }
}

void Engine::_activateCurrentTexture()
{
  if (_currentTextureChannel != _activeChannel) {
    _gl->activeTexture((*_gl)["TEXTURE0"] + static_cast<unsigned int>(_activeChannel));
    _currentTextureChannel = _activeChannel;
  }
}

bool Engine::_bindTextureDirectly(unsigned int target, const InternalTexturePtr& texture,
                                  bool forTextureDataUpdate, bool force)
{
  auto wasPreviouslyBound    = false;
  auto isTextureForRendering = texture && texture->_associatedChannel > -1;
  if (forTextureDataUpdate && isTextureForRendering) {
    _activeChannel = texture->_associatedChannel;
  }

  InternalTexturePtr currentTextureBound = nullptr;
  if (stl_util::contains(_boundTexturesCache, _activeChannel)) {
    currentTextureBound = _boundTexturesCache[_activeChannel];
  }

  if (currentTextureBound != texture || force) {
    _activateCurrentTexture();

    if (texture && texture->isMultiview) {
      _gl->bindTexture(target, texture ? texture->_colorTextureArray.get() : nullptr);
    }
    else {
      _gl->bindTexture(target, texture ? texture->_webGLTexture.get() : nullptr);
    }

    _boundTexturesCache[_activeChannel] = texture;

    if (texture) {
      texture->_associatedChannel = _activeChannel;
    }
  }
  else if (forTextureDataUpdate) {
    wasPreviouslyBound = true;
    _activateCurrentTexture();
  }

  if (isTextureForRendering && !forTextureDataUpdate) {
    _bindSamplerUniformToChannel(texture->_associatedChannel, _activeChannel);
  }

  return wasPreviouslyBound;
}

void Engine::_bindTexture(int channel, const InternalTexturePtr& texture)
{
  if (channel < 0) {
    return;
  }

  if (texture) {
    texture->_associatedChannel = channel;
  }

  _activeChannel = channel;
  _bindTextureDirectly(GL::TEXTURE_2D, texture);
}

void Engine::setTextureFromPostProcess(int channel, PostProcess* postProcess)
{
  const auto _ind = static_cast<size_t>(postProcess->_currentRenderTextureInd);
  _bindTexture(channel, postProcess ? postProcess->_textures[_ind] : nullptr);
}

void Engine::setTextureFromPostProcessOutput(int channel, PostProcess* postProcess)
{
  _bindTexture(channel, postProcess ? postProcess->_outputTexture : nullptr);
}

void Engine::unbindAllTextures()
{
  for (unsigned int channel = 0; channel < _maxSimultaneousTextures; ++channel) {
    _activeChannel = static_cast<int>(channel);
    _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
    _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
    if (webGLVersion() > 1.f) {
      _bindTextureDirectly(GL::TEXTURE_3D, nullptr);
    }
  }
}

void Engine::setTexture(int channel, GL::IGLUniformLocation* uniform, const BaseTexturePtr& texture)
{
  if (channel < 0) {
    return;
  }

  if (uniform) {
    _boundUniforms[channel] = uniform;
  }

  _setTexture(channel, texture);
}

void Engine::setDepthStencilTexture(int channel, GL::IGLUniformLocation* uniform,
                                    const RenderTargetTexturePtr& texture)
{
  if (channel < 0) {
    return;
  }

  if (uniform) {
    _boundUniforms[channel] = uniform;
  }

  if (!texture || !texture->depthStencilTexture) {
    _setTexture(channel, nullptr);
  }
  else {
    _setTexture(channel, texture, false, true);
  }
}

void Engine::_bindSamplerUniformToChannel(int sourceSlot, int destination)
{
  auto& uniform = _boundUniforms[sourceSlot];
  if (uniform->_currentState == destination) {
    return;
  }
  _gl->uniform1i(uniform, destination);
  uniform->_currentState = destination;
}

unsigned int Engine::_getTextureWrapMode(unsigned int mode) const
{
  switch (mode) {
    case Constants::TEXTURE_WRAP_ADDRESSMODE:
      return GL::REPEAT;
    case Constants::TEXTURE_CLAMP_ADDRESSMODE:
      return GL::CLAMP_TO_EDGE;
    case Constants::TEXTURE_MIRROR_ADDRESSMODE:
      return GL::MIRRORED_REPEAT;
  }
  return GL::REPEAT;
}

bool Engine::_setTexture(int channel, const BaseTexturePtr& texture, bool isPartOfTextureArray,
                         bool depthStencilTexture)
{
  // Not ready?
  if (!texture) {
    if ((_boundTexturesCache.find(channel) != _boundTexturesCache.end())
        && (_boundTexturesCache[channel] != nullptr)) {
      _activeChannel = channel;
      _bindTextureDirectly(GL::TEXTURE_2D, nullptr);
      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, nullptr);
      if (webGLVersion() > 1.f) {
        _bindTextureDirectly(GL::TEXTURE_3D, nullptr);
      }
    }
    return false;
  }

  // Video (not supported)
  if (texture->delayLoadState == Constants::DELAYLOADSTATE_NOTLOADED) {
    // Delay loading
    texture->delayLoad();
    return false;
  }

  InternalTexturePtr internalTexture = nullptr;
  if (depthStencilTexture) {
    internalTexture = std::static_pointer_cast<RenderTargetTexture>(texture)->depthStencilTexture;
  }
  else if (texture->isReady()) {
    internalTexture = texture->getInternalTexture();
  }
  else if (texture->isCube) {
    internalTexture = emptyCubeTexture();
  }
  else if (texture->is3D) {
    internalTexture = emptyTexture3D();
  }
  else {
    internalTexture = emptyTexture();
  }

  if (!isPartOfTextureArray && internalTexture) {
    internalTexture->_associatedChannel = channel;
  }

  bool needToBind = true;
  if ((_boundTexturesCache.find(channel) != _boundTexturesCache.end())
      && (_boundTexturesCache[channel] == internalTexture)) {
    if (!isPartOfTextureArray) {
      _bindSamplerUniformToChannel(internalTexture->_associatedChannel, channel);
    }
    needToBind = false;
  }

  _activeChannel = channel;

  if (internalTexture && internalTexture->is3D) {
    if (needToBind) {
      _bindTextureDirectly(GL::TEXTURE_3D, internalTexture, isPartOfTextureArray);
    }

    if (internalTexture && internalTexture->_cachedWrapU != static_cast<int>(texture->wrapU)) {
      internalTexture->_cachedWrapU = static_cast<int>(texture->wrapU);
      _setTextureParameterInteger(GL::TEXTURE_3D, GL::TEXTURE_WRAP_S,
                                  static_cast<int>(_getTextureWrapMode(texture->wrapU)),
                                  internalTexture);
    }

    if (internalTexture && internalTexture->_cachedWrapV != static_cast<int>(texture->wrapV)) {
      internalTexture->_cachedWrapV = static_cast<int>(texture->wrapV);
      _setTextureParameterInteger(GL::TEXTURE_3D, GL::TEXTURE_WRAP_T,
                                  static_cast<int>(_getTextureWrapMode(texture->wrapV)),
                                  internalTexture);
    }

    if (internalTexture && internalTexture->_cachedWrapR != static_cast<int>(texture->wrapR)) {
      internalTexture->_cachedWrapR = static_cast<int>(texture->wrapR);
      _setTextureParameterInteger(GL::TEXTURE_3D, GL::TEXTURE_WRAP_R,
                                  static_cast<int>(_getTextureWrapMode(texture->wrapR)),
                                  internalTexture);
    }

    _setAnisotropicLevel(GL::TEXTURE_3D, texture);
  }
  else if (internalTexture && internalTexture->isCube) {
    if (needToBind) {
      _bindTextureDirectly(GL::TEXTURE_CUBE_MAP, internalTexture, isPartOfTextureArray);
    }

    if (internalTexture->_cachedCoordinatesMode != static_cast<int>(texture->coordinatesMode())) {
      internalTexture->_cachedCoordinatesMode = static_cast<int>(texture->coordinatesMode());
      // CUBIC_MODE and SKYBOX_MODE both require CLAMP_TO_EDGE.  All other modes use REPEAT.
      auto textureWrapMode = (texture->coordinatesMode() != Constants::TEXTURE_CUBIC_MODE
                              && texture->coordinatesMode() != Constants::TEXTURE_SKYBOX_MODE) ?
                               GL::REPEAT :
                               GL::CLAMP_TO_EDGE;
      _setTextureParameterInteger(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_S,
                                  static_cast<int>(textureWrapMode), internalTexture);
      _setTextureParameterInteger(GL::TEXTURE_CUBE_MAP, GL::TEXTURE_WRAP_T,
                                  static_cast<int>(textureWrapMode));
    }

    _setAnisotropicLevel(GL::TEXTURE_CUBE_MAP, texture);
  }
  else {
    if (needToBind) {
      _bindTextureDirectly(GL::TEXTURE_2D, internalTexture, isPartOfTextureArray);
    }

    if (internalTexture && internalTexture->_cachedWrapU != static_cast<int>(texture->wrapU)) {
      internalTexture->_cachedWrapU = static_cast<int>(texture->wrapU);
      _setTextureParameterInteger(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S,
                                  static_cast<int>(_getTextureWrapMode(texture->wrapU)),
                                  internalTexture);
    }

    if (internalTexture && internalTexture->_cachedWrapV != static_cast<int>(texture->wrapV)) {
      internalTexture->_cachedWrapV = static_cast<int>(texture->wrapV);
      _setTextureParameterInteger(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T,
                                  static_cast<int>(_getTextureWrapMode(texture->wrapV)),
                                  internalTexture);
    }

    _setAnisotropicLevel(GL::TEXTURE_2D, texture);
  }

  return true;
}

void Engine::setTextureArray(int channel, GL::IGLUniformLocation* uniform,
                             const std::vector<BaseTexturePtr>& textures)
{
  if (channel < 0 || !uniform) {
    return;
  }

  if (_textureUnits.empty() || _textureUnits.size() != textures.size()) {
    _textureUnits.clear();
    _textureUnits.resize(textures.size());
  }
  auto _channel = static_cast<size_t>(channel);
  for (unsigned int i = 0; i < textures.size(); ++i) {
    auto texture = textures[i]->getInternalTexture();

    if (texture) {
      _textureUnits[_channel + i] = static_cast<int>(_channel + i);
      texture->_associatedChannel = static_cast<int>(_channel + i);
    }
    else {
      _textureUnits[_channel + i] = -1;
    }
  }
  _gl->uniform1iv(uniform, _textureUnits);

  unsigned int index = 0;
  for (auto& texture : textures) {
    _setTexture(_textureUnits[index], texture, true);
    ++index;
  }
}

void Engine::_setAnisotropicLevel(unsigned int target, const BaseTexturePtr& texture)
{
  auto internalTexture = texture->getInternalTexture();

  if (!internalTexture) {
    return;
  }

  auto anisotropicFilterExtension = _caps.textureAnisotropicFilterExtension;
  auto value                      = texture->anisotropicFilteringLevel;

  if (internalTexture->samplingMode != Constants::TEXTURE_LINEAR_LINEAR_MIPNEAREST
      && internalTexture->samplingMode != Constants::TEXTURE_LINEAR_LINEAR_MIPLINEAR
      && internalTexture->samplingMode != Constants::TEXTURE_LINEAR_LINEAR) {
    value = 1; // Forcing the anisotropic to 1 because else webgl will force filters to linear
  }

  if (anisotropicFilterExtension
      && internalTexture->_cachedAnisotropicFilteringLevel != static_cast<int>(value)) {
    _setTextureParameterFloat(target, AnisotropicFilterExtension::TEXTURE_MAX_ANISOTROPY_EXT,
                              static_cast<float>(std::min(value, _caps.maxAnisotropy)),
                              internalTexture);
    internalTexture->_cachedAnisotropicFilteringLevel = static_cast<int>(value);
  }
}
void Engine::_setTextureParameterFloat(unsigned int target, unsigned int parameter, float value,
                                       const InternalTexturePtr& texture)
{
  _bindTextureDirectly(target, texture, true, true);
  _gl->texParameterf(target, parameter, value);
}

void Engine::_setTextureParameterInteger(unsigned int target, unsigned int parameter, int value,
                                         const InternalTexturePtr& texture)
{
  if (texture) {
    _bindTextureDirectly(target, texture, true, true);
  }
  _gl->texParameteri(target, parameter, value);
}

Uint8Array Engine::readPixels(int x, int y, int width, int height)
{
  Uint8Array data(static_cast<size_t>(height * width * 4));
  _gl->readPixels(x, y, width, height, GL::RGBA, GL::UNSIGNED_BYTE, stl_util::as_span_rw(data));
  return data;
}

void Engine::unbindAllAttributes()
{
  if (_mustWipeVertexAttributes) {
    _mustWipeVertexAttributes = false;

    const auto ul = static_cast<unsigned>(_caps.maxVertexAttribs);
    if (_vertexAttribArraysEnabled.size() < ul) {
      _vertexAttribArraysEnabled.resize(ul);
    }

    for (unsigned int i = 0; i < ul; ++i) {
      _gl->disableVertexAttribArray(i);
      _vertexAttribArraysEnabled[i]    = false;
      _currentBufferPointers[i].active = false;
    }
    _currentBufferPointers.clear();
    return;
  }

  for (unsigned int i = 0; i < _vertexAttribArraysEnabled.size(); i++) {
    if (i >= static_cast<unsigned>(_caps.maxVertexAttribs) || !_vertexAttribArraysEnabled[i]) {
      continue;
    }
    _gl->disableVertexAttribArray(i);
    _vertexAttribArraysEnabled[i]    = false;
    _currentBufferPointers[i].active = false;
  }
  _currentBufferPointers.clear();
}

void Engine::releaseEffects()
{
  for (auto& item : _compiledEffects) {
    _deleteProgram(item.second->getProgram());
  }

  _compiledEffects.clear();
}

// Dispose
void Engine::dispose()
{
  hideLoadingUI();
  stopRenderLoop();

  // Release postProcesses
  for (auto& postProcess : postProcesses) {
    postProcess->dispose();
  }

  // Empty texture
  if (_emptyTexture) {
    _releaseTexture(_emptyTexture.get());
    _emptyTexture = nullptr;
  }

  if (_emptyCubeTexture) {
    _releaseTexture(_emptyCubeTexture.get());
    _emptyCubeTexture = nullptr;
  }

  // Rescale PP
  if (_rescalePostProcess) {
    _rescalePostProcess->dispose();
  }

  // Release scenes
  for (auto& scene : scenes) {
    scene->dispose();
  }
  scenes.clear();

  // Release audio engine
  // if (Engine::audioEngine) {
  //  Engine::audioEngine->dispose();
  // }

  // Release effects
  releaseEffects();

  // Unbind
  unbindAllAttributes();
  _boundUniforms.clear();

  if (_dummyFramebuffer) {
    _gl->deleteFramebuffer(_dummyFramebuffer.get());
  }

  // WebVR
  // disableVR();

  // Remove from Instances
  Engine::Instances.erase(std::remove(Engine::Instances.begin(), Engine::Instances.end(), this),
                          Engine::Instances.end());

  _workingCanvas  = nullptr;
  _workingContext = nullptr;
  _currentBufferPointers.clear();
  _renderingCanvas      = nullptr;
  _currentProgram       = nullptr;
  _bindedRenderFunction = nullptr;

  onResizeObservable.clear();
  onCanvasBlurObservable.clear();
  onCanvasFocusObservable.clear();
  onCanvasPointerOutObservable.clear();
  onBeginFrameObservable.clear();
  onEndFrameObservable.clear();

  Effect::ResetCache();
}

// Loading screen
void Engine::displayLoadingUI()
{
  if (_loadingScreen) {
    _loadingScreen->displayLoadingUI();
  }
}

void Engine::hideLoadingUI()
{
  if (_loadingScreen) {
    _loadingScreen->hideLoadingUI();
  }
}

ILoadingScreen* Engine::loadingScreen()
{
  return _loadingScreen;
}

void Engine::setLoadingScreen(ILoadingScreen* loadingScreen_)
{
  _loadingScreen = loadingScreen_;
}

void Engine::setLoadingUIText(const std::string& text)
{
  _loadingScreen->loadingUIText = text;
}

void Engine::loadingUIBackgroundColor(const std::string& color)
{
  _loadingScreen->loadingUIBackgroundColor = color;
}

std::string Engine::getVertexShaderSource(GL::IGLProgram* program)
{
  auto shaders = _gl->getAttachedShaders(program);
  if (shaders.empty()) {
    BABYLON_LOGF_ERROR("Engine", "Unable to get vertex shader source for program %d",
                       program->value)
    return "";
  }

  return _gl->getShaderSource(shaders[0]);
}

std::string Engine::getFragmentShaderSource(GL::IGLProgram* program)
{
  auto shaders = _gl->getAttachedShaders(program);
  if (shaders.size() < 2) {
    BABYLON_LOGF_ERROR("Engine", "Unable to get fragment shader source for program %d",
                       program->value)
    return "";
  }

  return _gl->getShaderSource(shaders[1]);
}

unsigned int Engine::getError() const
{
  return _gl->getError();
}

// FPS
float Engine::getFps() const
{
  return _fps;
}

float Engine::getDeltaTime() const
{
  return _deltaTime;
}

void Engine::_measureFps()
{
  _performanceMonitor->sampleFrame();
  _fps       = _performanceMonitor->averageFPS();
  _deltaTime = _performanceMonitor->instantaneousFrameTime();
}

ArrayBufferView Engine::_readTexturePixels(const InternalTexturePtr& texture, int width, int height,
                                           int faceIndex, int level,
                                           std::optional<ArrayBufferView> buffer)
{
  if (!_dummyFramebuffer) {
    auto dummy = _gl->createFramebuffer();

    if (!dummy) {
      BABYLON_LOG_ERROR("Engine", "Unable to create dummy framebuffer")
      return ArrayBufferView();
    }

    _dummyFramebuffer = dummy;
  }
  _gl->bindFramebuffer(GL::FRAMEBUFFER, _dummyFramebuffer.get());

  if (faceIndex > -1) {
    auto _faceIndex = static_cast<unsigned>(faceIndex);
    _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0,
                              GL::TEXTURE_CUBE_MAP_POSITIVE_X + _faceIndex,
                              texture->_webGLTexture.get(), level);
  }
  else {
    _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0, GL::TEXTURE_2D,
                              texture->_webGLTexture.get(), level);
  }

  auto readType = _getWebGLTextureType(texture->type);

  switch (readType) {
    case GL::UNSIGNED_BYTE: {
      if (!buffer.has_value()) {
        buffer = ArrayBufferView(Uint8Array(static_cast<std::size_t>(4 * width * height)));
      }
      readType = GL::UNSIGNED_BYTE;
      _gl->readPixels(0, 0, width, height, GL::RGBA, readType,
                      buffer->uint8Span_rw());
      _gl->bindFramebuffer(GL::FRAMEBUFFER, _currentFramebuffer.get());

    } break;
    default: {
      if (!buffer.has_value()) {
        buffer = ArrayBufferView(Float32Array(static_cast<std::size_t>(4 * width * height)));
      }
      readType = GL::FLOAT;
      _gl->readPixels(0, 0, width, height, GL::RGBA, readType,
                      buffer->float32Span_rw());
      _gl->bindFramebuffer(GL::FRAMEBUFFER, _currentFramebuffer.get());
    } break;
  }

  return *buffer;
}

bool Engine::_canRenderToFloatFramebuffer()
{
  if (_webGLVersion > 1.f) {
    return _caps.colorBufferFloat;
  }
  return _canRenderToFramebuffer(Constants::TEXTURETYPE_FLOAT);
}

bool Engine::_canRenderToHalfFloatFramebuffer()
{
  if (_webGLVersion > 1.f) {
    return _caps.colorBufferFloat;
  }

  return _canRenderToFramebuffer(Constants::TEXTURETYPE_HALF_FLOAT);
}

// Thank you :
// http://stackoverflow.com/questions/28827511/webgl-ios-render-to-floating-point-texture
bool Engine::_canRenderToFramebuffer(unsigned int /*type*/)
{
#if 0
  // clear existing errors
  while (_gl->getError() != GL::NO_ERROR) {
  }

  bool successful = true;

  auto texture = _gl->createTexture();
  _gl->bindTexture(GL::TEXTURE_2D, texture.get());
  _gl->texImage2D(GL::TEXTURE_2D, 0, _getRGBABufferInternalSizedFormat(type), 1,
                  1, 0, GL::RGBA, _getWebGLTextureType(type), nullptr);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
  _gl->texParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);

  auto fb = _gl->createFramebuffer();
  _gl->bindFramebuffer(GL::FRAMEBUFFER, fb.get());
  _gl->framebufferTexture2D(GL::FRAMEBUFFER, GL::COLOR_ATTACHMENT0,
                            GL::TEXTURE_2D, texture.get(), 0);
  auto status = _gl->checkFramebufferStatus(GL::FRAMEBUFFER);

  successful = successful && (status == GL::FRAMEBUFFER_COMPLETE);
  successful = successful && (_gl->getError() == GL::NO_ERROR);

  // try render by clearing frame buffer's color buffer
  if (successful) {
    _gl->clear(GL::COLOR_BUFFER_BIT);
    successful = successful && (_gl->getError() == GL::NO_ERROR);
  }

  // try reading from frame to ensure render occurs (just creating the FBO is
  // not sufficient to determine if rendering is supported)
  if (successful) {
    // in practice it's sufficient to just read from the backbuffer rather than
    // handle potentially issues reading from the texture
    _gl->bindFramebuffer(GL::FRAMEBUFFER, nullptr);
    auto readFormat = GL::RGBA;
    auto readType   = GL::UNSIGNED_BYTE;
    Uint8Array buffer(4);
    _gl->readPixels(0, 0, 1, 1, readFormat, readType, buffer);
    successful = successful && (_gl->getError() == GL::NO_ERROR);
  }

  // clean up
  _gl->deleteTexture(texture.get());
  _gl->deleteFramebuffer(fb.get());
  _gl->bindFramebuffer(GL::FRAMEBUFFER, nullptr);

  // clear accumulated errors
  while (!successful && (_gl->getError() != GL::NO_ERROR)) {
  }

  return successful;
#endif
  return true;
}

GL::GLenum Engine::_getWebGLTextureType(unsigned int type) const
{
  if (_webGLVersion == 1.f) {
    switch (type) {
      case Constants::TEXTURETYPE_FLOAT:
        return GL::FLOAT;
      case Constants::TEXTURETYPE_HALF_FLOAT:
        return GL::HALF_FLOAT_OES;
      case Constants::TEXTURETYPE_UNSIGNED_BYTE:
        return GL::UNSIGNED_BYTE;
    }
    return GL::UNSIGNED_BYTE;
  }

  switch (type) {
    case Constants::TEXTURETYPE_BYTE:
      return GL::BYTE;
    case Constants::TEXTURETYPE_UNSIGNED_BYTE:
      return GL::UNSIGNED_BYTE;
    case Constants::TEXTURETYPE_SHORT:
      return GL::SHORT;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT:
      return GL::UNSIGNED_SHORT;
    case Constants::TEXTURETYPE_INT:
      return GL::INT;
    case Constants::TEXTURETYPE_UNSIGNED_INTEGER:
      return GL::UNSIGNED_INT; // Refers to UNSIGNED_INT
    case Constants::TEXTURETYPE_FLOAT:
      return GL::FLOAT;
    case Constants::TEXTURETYPE_HALF_FLOAT:
      return GL::HALF_FLOAT;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_4_4_4_4:
      return GL::UNSIGNED_SHORT_4_4_4_4;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_5_5_5_1:
      return GL::UNSIGNED_SHORT_5_5_5_1;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_5_6_5:
      return GL::UNSIGNED_SHORT_5_6_5;
    case Constants::TEXTURETYPE_UNSIGNED_INT_2_10_10_10_REV:
      return GL::UNSIGNED_INT_2_10_10_10_REV;
    case Constants::TEXTURETYPE_UNSIGNED_INT_24_8:
      return GL::UNSIGNED_INT_24_8;
    case Constants::TEXTURETYPE_UNSIGNED_INT_10F_11F_11F_REV:
      return GL::UNSIGNED_INT_10F_11F_11F_REV;
    case Constants::TEXTURETYPE_UNSIGNED_INT_5_9_9_9_REV:
      return GL::UNSIGNED_INT_5_9_9_9_REV;
    case Constants::TEXTURETYPE_FLOAT_32_UNSIGNED_INT_24_8_REV:
      return GL::FLOAT_32_UNSIGNED_INT_24_8_REV;
  }

  return GL::UNSIGNED_BYTE;
}

GL::GLenum Engine::_getInternalFormat(unsigned int format) const
{
  GL::GLenum internalFormat = GL::RGBA;

  switch (format) {
    case Constants::TEXTUREFORMAT_ALPHA:
      internalFormat = GL::ALPHA;
      break;
    case Constants::TEXTUREFORMAT_LUMINANCE:
      internalFormat = GL::LUMINANCE;
      break;
    case Constants::TEXTUREFORMAT_LUMINANCE_ALPHA:
      internalFormat = GL::LUMINANCE_ALPHA;
      break;
    case Constants::TEXTUREFORMAT_RED:
      internalFormat = GL::RED;
      break;
    case Constants::TEXTUREFORMAT_RG:
      internalFormat = GL::RG;
      break;
    case Constants::TEXTUREFORMAT_RGB:
      internalFormat = GL::RGB;
      break;
    case Constants::TEXTUREFORMAT_RGBA:
      internalFormat = GL::RGBA;
      break;
  }

  if (_webGLVersion > 1.f) {
    switch (format) {
      case Constants::TEXTUREFORMAT_RED_INTEGER:
        internalFormat = GL::RED_INTEGER;
        break;
      case Constants::TEXTUREFORMAT_RG_INTEGER:
        internalFormat = GL::RG_INTEGER;
        break;
      case Constants::TEXTUREFORMAT_RGB_INTEGER:
        internalFormat = GL::RGB_INTEGER;
        break;
      case Constants::TEXTUREFORMAT_RGBA_INTEGER:
        internalFormat = GL::RGBA_INTEGER;
        break;
    }
  }

  return internalFormat;
}

GL::GLenum
Engine::_getRGBABufferInternalSizedFormat(unsigned int type,
                                          const std::optional<unsigned int>& format) const
{
  const auto _format = format.value_or(0);

  if (_webGLVersion == 1.f) {
    if (format) {
      switch (_format) {
        case Constants::TEXTUREFORMAT_ALPHA:
          return GL::ALPHA;
        case Constants::TEXTUREFORMAT_LUMINANCE:
          return GL::LUMINANCE;
        case Constants::TEXTUREFORMAT_LUMINANCE_ALPHA:
          return GL::LUMINANCE_ALPHA;
      }
    }
    return GL::RGBA;
  }

  switch (type) {
    case Constants::TEXTURETYPE_BYTE:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED:
          return GL::R8_SNORM;
        case Constants::TEXTUREFORMAT_RG:
          return GL::RG8_SNORM;
        case Constants::TEXTUREFORMAT_RGB:
          return GL::RGB8_SNORM;
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R8I;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG8I;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB8I;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA8I;
        default:
          return GL::RGBA8_SNORM;
      }
    case Constants::TEXTURETYPE_UNSIGNED_BYTE:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED:
          return GL::R8;
        case Constants::TEXTUREFORMAT_RG:
          return GL::RG8;
        case Constants::TEXTUREFORMAT_RGB:
          return GL::RGB8; // By default. Other possibilities are RGB565, SRGB8.
        case Constants::TEXTUREFORMAT_RGBA:
          return GL::RGBA8; // By default. Other possibilities are RGB5_A1,
                            // RGBA4, SRGB8_ALPHA8.
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R8UI;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG8UI;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB8UI;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA8UI;
        default:
          return GL::RGBA8;
      }
    case Constants::TEXTURETYPE_SHORT:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R16I;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG16I;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB16I;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA16I;
        default:
          return GL::RGBA16I;
      }
    case Constants::TEXTURETYPE_UNSIGNED_SHORT:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R16UI;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG16UI;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB16UI;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA16UI;
        default:
          return GL::RGBA16UI;
      }
    case Constants::TEXTURETYPE_INT:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R32I;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG32I;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB32I;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA32I;
        default:
          return GL::RGBA32I;
      }
    case Constants::TEXTURETYPE_UNSIGNED_INTEGER: // Refers to
                                                  // UNSIGNED_INT
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED_INTEGER:
          return GL::R32UI;
        case Constants::TEXTUREFORMAT_RG_INTEGER:
          return GL::RG32UI;
        case Constants::TEXTUREFORMAT_RGB_INTEGER:
          return GL::RGB32UI;
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGBA32UI;
        default:
          return GL::RGBA32UI;
      }
    case Constants::TEXTURETYPE_FLOAT:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED:
          return GL::R32F; // By default. Other possibility is R16F.
        case Constants::TEXTUREFORMAT_RG:
          return GL::RG32F; // By default. Other possibility is RG16F.
        case Constants::TEXTUREFORMAT_RGB:
          return GL::RGB32F; // By default. Other possibilities are RGB16F,
                             // R11F_G11F_B10F, RGB9_E5.
        case Constants::TEXTUREFORMAT_RGBA:
          return GL::RGBA32F; // By default. Other possibility is RGBA16F.
        default:
          return GL::RGBA32F;
      }
    case Constants::TEXTURETYPE_HALF_FLOAT:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RED:
          return GL::R16F;
        case Constants::TEXTUREFORMAT_RG:
          return GL::RG16F;
        case Constants::TEXTUREFORMAT_RGB:
          return GL::RGB16F; // By default. Other possibilities are
                             // R11F_G11F_B10F, RGB9_E5.
        case Constants::TEXTUREFORMAT_RGBA:
          return GL::RGBA16F;
        default:
          return GL::RGBA16F;
      }
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_5_6_5:
      return GL::RGB565;
    case Constants::TEXTURETYPE_UNSIGNED_INT_10F_11F_11F_REV:
      return GL::R11F_G11F_B10F;
    case Constants::TEXTURETYPE_UNSIGNED_INT_5_9_9_9_REV:
      return GL::RGB9_E5;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_4_4_4_4:
      return GL::RGBA4;
    case Constants::TEXTURETYPE_UNSIGNED_SHORT_5_5_5_1:
      return GL::RGB5_A1;
    case Constants::TEXTURETYPE_UNSIGNED_INT_2_10_10_10_REV:
      switch (_format) {
        case Constants::TEXTUREFORMAT_RGBA:
          return GL::RGB10_A2; // By default. Other possibility is RGB5_A1.
        case Constants::TEXTUREFORMAT_RGBA_INTEGER:
          return GL::RGB10_A2UI;
        default:
          return GL::RGB10_A2;
      }
  }

  return GL::RGBA8;
}

GL::GLenum Engine::_getRGBAMultiSampleBufferFormat(unsigned int type) const
{
  if (type == Constants::TEXTURETYPE_FLOAT) {
    return GL::RGBA32F;
  }
  else if (type == Constants::TEXTURETYPE_HALF_FLOAT) {
    return GL::RGBA16F;
  }

  return GL::RGBA8;
}

Engine::GLQueryPtr Engine::createQuery()
{
  return _gl->createQuery();
}

Engine& Engine::deleteQuery(const GLQueryPtr& query)
{
  _gl->deleteQuery(query.get());

  return *this;
}

bool Engine::isQueryResultAvailable(const GLQueryPtr& query)
{
  return _gl->getQueryParameterb(query.get(), GL::QUERY_RESULT_AVAILABLE);
}

unsigned int Engine::getQueryResult(const GLQueryPtr& query)
{
  return _gl->getQueryParameteri(query.get(), GL::QUERY_RESULT);
}

Engine& Engine::beginOcclusionQuery(unsigned int algorithmType, const GLQueryPtr& query)
{
  const auto glAlgorithm = _getGlAlgorithmType(algorithmType);
  _gl->beginQuery(glAlgorithm, query);

  return *this;
}

Engine& Engine::endOcclusionQuery(unsigned int algorithmType)
{
  const auto glAlgorithm = _getGlAlgorithmType(algorithmType);
  _gl->endQuery(glAlgorithm);

  return *this;
}

Engine::GLQueryPtr Engine::_createTimeQuery()
{
  return createQuery();
}

void Engine::_deleteTimeQuery(const Engine::GLQueryPtr& query)
{
  deleteQuery(query);
}

unsigned int Engine::_getTimeQueryResult(const Engine::GLQueryPtr& query)
{
  return getQueryResult(query);
}

bool Engine::_getTimeQueryAvailability(const Engine::GLQueryPtr& query)
{
  return isQueryResultAvailable(query);
}

std::optional<_TimeToken> Engine::startTimeQuery()
{
  return std::nullopt;
}

int Engine::endTimeQuery(std::optional<_TimeToken>& /*token*/)
{
  return -1;
}

unsigned int Engine::_getGlAlgorithmType(unsigned int algorithmType) const
{
  return algorithmType == AbstractMesh::OCCLUSION_ALGORITHM_TYPE_CONSERVATIVE ?
           GL::ANY_SAMPLES_PASSED_CONSERVATIVE :
           GL::ANY_SAMPLES_PASSED;
}

GL::IGLTransformFeedbackPtr Engine::createTransformFeedback()
{
  return _gl->createTransformFeedback();
}

void Engine::deleteTransformFeedback(GL::IGLTransformFeedback* value)
{
  _gl->deleteTransformFeedback(value);
}

void Engine::bindTransformFeedback(GL::IGLTransformFeedback* value)
{
  _gl->bindTransformFeedback(GL::TRANSFORM_FEEDBACK, value);
}

void Engine::beginTransformFeedback(bool usePoints)
{
  _gl->beginTransformFeedback(usePoints ? GL::POINTS : GL::TRIANGLES);
}

void Engine::endTransformFeedback()
{
  _gl->endTransformFeedback();
}

void Engine::setTranformFeedbackVaryings(GL::IGLProgram* program,
                                         const std::vector<std::string>& value)
{
  _gl->transformFeedbackVaryings(program, value, GL::INTERLEAVED_ATTRIBS);
}

void Engine::bindTransformFeedbackBuffer(GL::IGLBuffer* value)
{
  _gl->bindBufferBase(GL::TRANSFORM_FEEDBACK_BUFFER, 0, value);
}

IFileRequest Engine::_loadFile(
  const std::string& url,
  const std::function<void(const std::variant<std::string, ArrayBuffer>& data,
                           const std::string& responseURL)>& onSuccess,
  const std::function<void(const ProgressEvent& event)>& onProgress, bool useArrayBuffer,
  const std::function<void(const std::string& message, const std::string& exception)>& onError)
{
  FileTools::LoadFile(url, onSuccess, onProgress, useArrayBuffer, onError);
  return IFileRequest();
}

std::promise<std::string> Engine::_loadFileAsync(const std::string& /*url*/)
{
  std::promise<std::string> result;
  return result;
}

void Engine::_cascadeLoadFiles(
  Scene* /*scene*/,
  const std::function<void(const std::variant<std::string, ArrayBuffer>& data,
                           const std::string& responseURL)>& /*onloaddata*/,
  const std::vector<std::string>& /*files*/,
  const std::function<void(const std::string& message, const std::string& exception)>& /*onError*/)
{
}

// Statics
bool Engine::isSupported()
{
  return true;
}

int Engine::CeilingPOT(int x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x++;
  return x;
}

int Engine::FloorPOT(int x)
{
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x - (x >> 1);
}

int Engine::NearestPOT(int x)
{
  auto c = Engine::CeilingPOT(x);
  auto f = Engine::FloorPOT(x);
  return (c - x) > (x - f) ? f : c;
}

int Engine::GetExponentOfTwo(int value, int max, unsigned int mode)
{
  int pot;

  switch (mode) {
    case Constants::SCALEMODE_FLOOR:
      pot = Engine::FloorPOT(value);
      break;
    case Constants::SCALEMODE_NEAREST:
      pot = Engine::NearestPOT(value);
      break;
    case Constants::SCALEMODE_CEILING:
      pot = Engine::CeilingPOT(value);
      break;
    default:
      throw std::runtime_error("Engine::GetExponentOfTwo: unexpected mode");
  }

  return std::min(pot, max);
}

} // end of namespace BABYLON
