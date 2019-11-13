#ifndef BABYLON_STENCIL_STATE_H
#define BABYLON_STENCIL_STATE_H

#include <babylon/babylon_api.h>
#include <babylon/babylon_common.h>
#include <babylon/engines/constants.h>

namespace BABYLON {

namespace GL {
class IGLRenderingContext;
} // end of namespace GL

class BABYLON_SHARED_EXPORT _StencilState {

public:
  /**
   * Passed to depthFunction or stencilFunction to specify depth or stencil
   * tests will always pass. i.e. Pixels will be drawn in the order they are
   * drawn
   */
  static constexpr unsigned int ALWAYS = Constants::ALWAYS;
  /**
   * Passed to stencilOperation to specify that stencil value must be kept
   */
  static constexpr unsigned int KEEP = Constants::KEEP;
  /**
   * Passed to stencilOperation to specify that stencil value must be replaced
   */
  static constexpr unsigned int REPLACE = Constants::REPLACE;

public:
  /**
   * Initializes the state.
   */
  _StencilState();
  ~_StencilState(); // = default

  void reset();
  void apply(GL::IGLRenderingContext& gl);

protected:
  bool get_isDirty() const;
  unsigned int get_stencilFunc() const;
  void set_stencilFunc(unsigned int value);
  int get_stencilFuncRef() const;
  void set_stencilFuncRef(int value);
  unsigned int get_stencilFuncMask() const;
  void set_stencilFuncMask(unsigned int value);
  unsigned int get_stencilOpStencilFail() const;
  void set_stencilOpStencilFail(unsigned int value);
  unsigned int get_stencilOpDepthFail() const;
  void set_stencilOpDepthFail(unsigned int value);
  unsigned int get_stencilOpStencilDepthPass() const;
  void set_stencilOpStencilDepthPass(unsigned int value);
  unsigned int get_stencilMask() const;
  void set_stencilMask(unsigned int value);
  bool get_stencilTest() const;
  void set_stencilTest(bool value);

public:
  ReadOnlyProperty<_StencilState, bool> isDirty;
  Property<_StencilState, unsigned int> stencilFunc;
  Property<_StencilState, int> stencilFuncRef;
  Property<_StencilState, unsigned int> stencilFuncMask;
  Property<_StencilState, unsigned int> stencilOpStencilFail;
  Property<_StencilState, unsigned int> stencilOpDepthFail;
  Property<_StencilState, unsigned int> stencilOpStencilDepthPass;
  Property<_StencilState, unsigned int> stencilMask;
  Property<_StencilState, bool> stencilTest;

private:
  bool _isStencilTestDirty;
  bool _isStencilMaskDirty;
  bool _isStencilFuncDirty;
  bool _isStencilOpDirty;

  bool _stencilTest;

  unsigned int _stencilMask;

  unsigned int _stencilFunc;
  int _stencilFuncRef;
  unsigned int _stencilFuncMask;

  unsigned int _stencilOpStencilFail;
  unsigned int _stencilOpDepthFail;
  unsigned int _stencilOpStencilDepthPass;

}; // end of class _StencilState

} // end of namespace BABYLON

#endif // end of BABYLON_STENCIL_STATE_H
