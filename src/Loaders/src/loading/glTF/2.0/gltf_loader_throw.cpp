#include <babylon/loading/glTF/2.0/gltf_loader.h>
#include <babylon/loading/glTF/igltf_loader.h>
#include <babylon/loading/iscene_loader_plugin_async.h>

#define THROW_NOT_IMPLEMENTED { throw std::runtime_error("Not implemented");}

#pragma clang diagnostic ignored "-Weverything"

namespace BABYLON {

namespace GLTF2 {

void
GLTFLoader::RegisterExtension(const std::string& name,
                  const std::function<IGLTFLoaderExtensionPtr(GLTFLoader& loader)>& factory) THROW_NOT_IMPLEMENTED

bool UnregisterExtension(const std::string& name) THROW_NOT_IMPLEMENTED

GLTFLoader::GLTFLoader(GLTFFileLoader& parent) : _parent(parent) THROW_NOT_IMPLEMENTED


GLTFLoader::~GLTFLoader() THROW_NOT_IMPLEMENTED // = default

GLTFLoaderState& GLTFLoader::state() THROW_NOT_IMPLEMENTED

std::unique_ptr<IGLTF>& GLTFLoader::gltf() THROW_NOT_IMPLEMENTED

Scene* GLTFLoader::babylonScene() THROW_NOT_IMPLEMENTED

MeshPtr GLTFLoader::rootBabylonMesh() THROW_NOT_IMPLEMENTED

void GLTFLoader::dispose(bool doNotRecurse, bool disposeMaterialAndTextures)  THROW_NOT_IMPLEMENTED

ImportedMeshes
GLTFLoader::importMeshAsync(const std::vector<std::string>& meshesNames, Scene* scene,
                const IGLTFLoaderData& data, const std::string& rootUrl,
                const std::function<void(const SceneLoaderProgressEvent& event)>& onProgress,
                const std::string& fileName)  THROW_NOT_IMPLEMENTED

void GLTFLoader::loadAsync(Scene* scene, const IGLTFLoaderData& data, const std::string& rootUrl,
               const std::function<void(const SceneLoaderProgressEvent& event)>& onProgress,
               const std::string& fileName)  THROW_NOT_IMPLEMENTED

bool GLTFLoader::loadSceneAsync(const std::string& context, const IScene& scene) THROW_NOT_IMPLEMENTED

TransformNodePtr
GLTFLoader::loadNodeAsync(const std::string& context, INode& node,
              const std::function<void(const TransformNodePtr& babylonTransformNode)>& assign) THROW_NOT_IMPLEMENTED

AbstractMeshPtr
GLTFLoader::_loadMeshPrimitiveAsync(const std::string& context, const std::string& name, INode& node,
                        const IMesh& mesh, IMeshPrimitive& primitive,
                        const std::function<void(const AbstractMeshPtr& babylonMesh)>& assign) THROW_NOT_IMPLEMENTED

CameraPtr GLTFLoader::loadCameraAsync(const std::string& context, const ICamera& camera,
                          const std::function<void(const CameraPtr& babylonCamera)>& assign) THROW_NOT_IMPLEMENTED

AnimationGroupPtr GLTFLoader::loadAnimationAsync(const std::string& context, std::shared_ptr<IAnimation>& animation) THROW_NOT_IMPLEMENTED

void GLTFLoader::_loadAnimationChannelAsync(const std::string& context, const std::string& animationContext,
                                            const std::shared_ptr<IAnimation>& animation, const IAnimationChannel& channel,
                                            const AnimationGroupPtr& babylonAnimationGroup,
                                            const IAnimatablePtr& animationTargetOverride) THROW_NOT_IMPLEMENTED

std::shared_ptr<ArrayBufferView>& GLTFLoader::loadBufferViewAsync(const std::string& context, IBufferView& bufferView) THROW_NOT_IMPLEMENTED

MaterialPtr
GLTFLoader::_loadMaterialAsync(const std::string& context, IMaterial& material, const MeshPtr& babylonMesh,
                   unsigned int babylonDrawMode,
                   const std::function<void(const MaterialPtr& babylonMaterial)>& assign) THROW_NOT_IMPLEMENTED

MaterialPtr GLTFLoader::createMaterial(const std::string& context, const IMaterial& material,
                           unsigned int babylonDrawMode) THROW_NOT_IMPLEMENTED

bool GLTFLoader::loadMaterialPropertiesAsync(const std::string& context, const IMaterial& material,
                                 const MaterialPtr& babylonMaterial) THROW_NOT_IMPLEMENTED

void GLTFLoader::loadMaterialBasePropertiesAsync(const std::string& context, const IMaterial& material,
                                     const MaterialPtr& babylonMaterial) THROW_NOT_IMPLEMENTED

void GLTFLoader::loadMaterialAlphaProperties(const std::string& context, const IMaterial& material,
                                 const MaterialPtr& babylonMaterial) THROW_NOT_IMPLEMENTED

BaseTexturePtr
GLTFLoader::loadTextureInfoAsync(const std::string& context, const IGLTF2::ITextureInfo& textureInfo,
                     const std::function<void(const BaseTexturePtr& babylonTexture)>& assign) THROW_NOT_IMPLEMENTED

std::shared_ptr<ArrayBufferView> GLTFLoader::loadImageAsync(const std::string& context, IImage& image) THROW_NOT_IMPLEMENTED

std::shared_ptr<ArrayBufferView> GLTFLoader::loadUriAsync(const std::string& context, const std::string& uri) THROW_NOT_IMPLEMENTED

void GLTFLoader::AddPointerMetadata(const BaseTexturePtr& babylonObject, const std::string& pointer) THROW_NOT_IMPLEMENTED

void GLTFLoader::AddPointerMetadata(const TransformNodePtr& babylonObject, const std::string& pointer) THROW_NOT_IMPLEMENTED

void GLTFLoader::AddPointerMetadata(const MaterialPtr& babylonObject, const std::string& pointer) THROW_NOT_IMPLEMENTED

void GLTFLoader::AddPointerMetadata(const CameraPtr& babylonObject, const std::string& pointer) THROW_NOT_IMPLEMENTED

void GLTFLoader::logOpen(const std::string& message) THROW_NOT_IMPLEMENTED

void GLTFLoader::logClose() THROW_NOT_IMPLEMENTED

void GLTFLoader::log(const std::string& message) THROW_NOT_IMPLEMENTED

void GLTFLoader::startPerformanceCounter(const std::string& counterName) THROW_NOT_IMPLEMENTED

void GLTFLoader::endPerformanceCounter(const std::string& counterName) THROW_NOT_IMPLEMENTED


} // namespace GLTF2

} // namespace BABYLON