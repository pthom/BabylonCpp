#include <babylon/animations/animation_group.h>

#include <babylon/animations/animatable.h>
#include <babylon/animations/animation.h>
#include <babylon/animations/ianimation_key.h>
#include <babylon/animations/targeted_animation.h>
#include <babylon/babylon_stl_util.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>

namespace BABYLON {

AnimationGroup::AnimationGroup(const std::string& iName, Scene* scene)
    : name{iName}
    , from{this, &AnimationGroup::get_from}
    , to{this, &AnimationGroup::get_to}
    , isStarted{this, &AnimationGroup::get_isStarted}
    , isPlaying{this, &AnimationGroup::get_isPlaying}
    , speedRatio{this, &AnimationGroup::get_speedRatio,
                 &AnimationGroup::set_speedRatio}
    , loopAnimation{this, &AnimationGroup::get_loopAnimation,
                    &AnimationGroup::set_loopAnimation}
    , targetedAnimations{this, &AnimationGroup::get_targetedAnimations}
    , animatables{this, &AnimationGroup::get_animatables}
    , _from{std::numeric_limits<float>::max()}
    , _to{std::numeric_limits<float>::lowest()}
    , _isStarted{false}
    , _isPaused{false}
    , _speedRatio{1.f}
    , _loopAnimation{false}
{
  _scene   = scene ? scene : Engine::LastCreatedScene();
  uniqueId = _scene->getUniqueId();
}

AnimationGroup::~AnimationGroup() = default;

void AnimationGroup::addToScene(const AnimationGroupPtr& newAnimationGroup)
{
  _scene->animationGroups.emplace_back(newAnimationGroup);
}

float AnimationGroup::get_from() const
{
  return _from;
}

float AnimationGroup::get_to() const
{
  return _to;
}

bool AnimationGroup::get_isStarted() const
{
  return _isStarted;
}

bool AnimationGroup::get_isPlaying() const
{
  return _isStarted && !_isPaused;
}

float AnimationGroup::get_speedRatio() const
{
  return _speedRatio;
}

void AnimationGroup::set_speedRatio(float value)
{
  if (stl_util::almost_equal(_speedRatio, value)) {
    return;
  }

  _speedRatio = value;

  for (const auto& animatable : _animatables) {
    animatable->speedRatio = _speedRatio;
  }
}

bool AnimationGroup::get_loopAnimation() const
{
  return _loopAnimation;
}

void AnimationGroup::set_loopAnimation(bool value)
{
  if (_loopAnimation == value) {
    return;
  }

  _loopAnimation = value;

  for (const auto& animatable : _animatables) {
    animatable->loopAnimation = _loopAnimation;
  }
}

std::vector<std::unique_ptr<TargetedAnimation>>&
AnimationGroup::get_targetedAnimations()
{
  return _targetedAnimations;
}

std::vector<AnimatablePtr>& AnimationGroup::get_animatables()
{
  return _animatables;
}

TargetedAnimation
AnimationGroup::addTargetedAnimation(const AnimationPtr& animation,
                                     const IAnimatablePtr& target)
{
  TargetedAnimation targetedAnimation{
    animation, // animation,
    target     // target
  };

  const auto& keys = animation->getKeys();
  if (_from > keys.front().frame) {
    _from = keys.front().frame;
  }

  if (_to < keys.back().frame) {
    _to = keys.back().frame;
  }

  _targetedAnimations.emplace_back(
    std::make_unique<TargetedAnimation>(targetedAnimation));

  return targetedAnimation;
}

AnimationGroup& AnimationGroup::normalize(const std::optional<int>& iBeginFrame,
                                          const std::optional<int>& iEndFrame)
{
  auto beginFrame = iBeginFrame ? *iBeginFrame : _from;
  auto endFrame   = iEndFrame ? *iEndFrame : _to;

  for (auto& targetedAnimation : _targetedAnimations) {
    auto& keys           = targetedAnimation->animation->getKeys();
    const auto& startKey = keys.front();
    const auto& endKey   = keys.back();

    if (startKey.frame > beginFrame) {
      IAnimationKey newKey(beginFrame, startKey.value);
      newKey.inTangent     = startKey.inTangent;
      newKey.outTangent    = startKey.outTangent;
      newKey.interpolation = startKey.interpolation;
      keys.insert(keys.begin(), newKey);
    }

    if (endKey.frame < endFrame) {
      IAnimationKey newKey(endFrame, endKey.value);
      newKey.inTangent     = endKey.inTangent;
      newKey.outTangent    = endKey.outTangent;
      newKey.interpolation = endKey.interpolation;
      keys.emplace_back(newKey);
    }
  }

  _from = beginFrame;
  _to   = endFrame;

  return *this;
}

AnimationGroup& AnimationGroup::start(bool loop, float iSpeedRatio,
                                      std::optional<float> iFrom,
                                      std::optional<float> iTo)
{
  if (_isStarted || _targetedAnimations.empty()) {
    return *this;
  }

  _loopAnimation = loop;

  for (auto& targetedAnimation : _targetedAnimations) {
    auto animatable = _scene->beginDirectAnimation(
      targetedAnimation->target, {targetedAnimation->animation},
      iFrom.has_value() ? *iFrom : _from, iTo.has_value() ? *iTo : _to, loop,
      iSpeedRatio);
    animatable->onAnimationEnd = [&]() -> void {
      onAnimationEndObservable.notifyObservers(targetedAnimation.get());
      _checkAnimationGroupEnded(animatable);
    };
    animatable->onAnimationLoop = [this, &targetedAnimation]() -> void {
      onAnimationLoopObservable.notifyObservers(targetedAnimation.get());
    };
    _animatables.emplace_back(animatable);
  }

  _speedRatio = iSpeedRatio;

  if (iFrom.has_value() && iTo.has_value()) {
    if (*iFrom < *iTo && _speedRatio < 0.f) {
      std::swap(iFrom, iTo);
    }
    else if (*iFrom > *iTo && _speedRatio > 0.f) {
      _speedRatio = -speedRatio;
    }
  }

  _isStarted = true;
  _isPaused  = false;

  onAnimationGroupPlayObservable.notifyObservers(this);

  return *this;
}

AnimationGroup& AnimationGroup::pause()
{
  if (!_isStarted) {
    return *this;
  }

  _isPaused = true;

  for (const auto& animatable : _animatables) {
    animatable->pause();
  }

  onAnimationGroupPauseObservable.notifyObservers(this);

  return *this;
}

AnimationGroup& AnimationGroup::play(const std::optional<bool> loop)
{
  // only if all animatables are ready and exist
  if (isStarted() && _animatables.size() == _targetedAnimations.size()) {
    if (loop.has_value()) {
      loopAnimation = *loop;
    }
    restart();
  }
  else {
    stop();
    start(loop.value_or(false), _speedRatio);
  }

  _isPaused = false;

  return *this;
}

AnimationGroup& AnimationGroup::reset()
{
  if (!_isStarted) {
    return *this;
  }

  for (const auto& animatable : _animatables) {
    animatable->reset();
  }

  _isStarted = false;

  return *this;
}

AnimationGroup& AnimationGroup::restart()
{
  if (!_isStarted) {
    return *this;
  }

  for (const auto& animatable : _animatables) {
    animatable->restart();
  }

  onAnimationGroupPlayObservable.notifyObservers(this);

  return *this;
}

AnimationGroup& AnimationGroup::stop()
{
  if (!_isStarted) {
    return *this;
  }

  for (const auto& animatable : _animatables) {
    animatable->stop();
  }

  _isStarted = false;

  return *this;
}

AnimationGroup& AnimationGroup::setWeightForAllAnimatables(float weight)
{
  for (const auto& animatable : _animatables) {
    animatable->weight = weight;
  }

  return *this;
}

AnimationGroup& AnimationGroup::syncAllAnimationsWith(Animatable* root)
{
  for (const auto& animatable : _animatables) {
    animatable->syncWith(root);
  }

  return *this;
}

AnimationGroup& AnimationGroup::goToFrame(float frame)
{
  if (!_isStarted) {
    return *this;
  }

  for (const auto& animatable : _animatables) {
    animatable->goToFrame(frame);
  }

  return *this;
}

void AnimationGroup::dispose(bool /*doNotRecurse*/,
                             bool /*disposeMaterialAndTextures*/)
{
  _targetedAnimations.clear();
  _animatables.clear();

  _scene->animationGroups.erase(
    std::remove_if(_scene->animationGroups.begin(),
                   _scene->animationGroups.end(),
                   [this](const AnimationGroupPtr& animationGroup) {
                     return animationGroup.get() == this;
                   }),
    _scene->animationGroups.end());

  onAnimationEndObservable.clear();
  onAnimationGroupEndObservable.clear();
  onAnimationGroupPauseObservable.clear();
  onAnimationGroupPlayObservable.clear();
  onAnimationLoopObservable.clear();
}

void AnimationGroup::_checkAnimationGroupEnded(const AnimatablePtr& animatable)
{
  // animatable should be taken out of the array
  _animatables.erase(
    std::remove(_animatables.begin(), _animatables.end(), animatable),
    _animatables.end());

  // all animatables were removed? animation group ended!
  if (_animatables.empty()) {
    _isStarted = false;
    onAnimationGroupEndObservable.notifyObservers(this);
  }
}

AnimationGroupPtr AnimationGroup::clone(
  const std::string& newName,
  const std::function<IAnimatablePtr(const IAnimatablePtr& animatible)>&
    targetConverter)
{
  auto newGroup = AnimationGroup::New(newName.empty() ? name : newName, _scene);

  for (const auto& targetAnimation : _targetedAnimations) {
    newGroup->addTargetedAnimation(targetAnimation->animation->clone(),
                                   targetConverter ?
                                     targetConverter(targetAnimation->target) :
                                     targetAnimation->target);
  }

  return newGroup;
}

json AnimationGroup::serialize() const
{
  return nullptr;
}

AnimationGroupPtr AnimationGroup::Parse(const json& /*parsedAnimationGroup*/,
                                        Scene* /*scene*/)
{
  return nullptr;
}

std::string AnimationGroup::getClassName() const
{
  return "AnimationGroup";
}

std::string AnimationGroup::toString(bool fullDetails) const
{
  std::ostringstream oss;
  oss << "Name: " << name;
  oss << ", type: " + getClassName();
  if (fullDetails) {
    oss << ", from: " << _from;
    oss << ", to: " << _to;
    oss << ", isStarted: " << _isStarted;
    oss << ", speedRatio: " << _speedRatio;
    oss << ", targetedAnimations length: " << _targetedAnimations.size();
    oss << ", animatables length: " << _animatables;
  }
  return oss.str();
}

} // end of namespace BABYLON
