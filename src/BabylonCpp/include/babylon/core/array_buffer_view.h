#ifndef BABYLON_CORE_ARRAY_BUFFER_VIEW_H
#define BABYLON_CORE_ARRAY_BUFFER_VIEW_H

#include <babylon/babylon_api.h>
#include <babylon/babylon_stl_util.h>
#include <babylon/babylon_common.h>

namespace BABYLON {

/**
 * @brief ArrayBufferView is a helper type that stores a buffer
 * which can be accessed through different span types.
 *
 * The memory is shared between them!
 * Use with care and as rarely as possible.
 * We should get rid of this class.
 *
 * This class is probably a bottleneck, since any call to the constructor,
 * copy constructor, and operator= copies the data,
 * which is not needed most of the time
 */
class BABYLON_SHARED_EXPORT ArrayBufferView {
public:
  ArrayBufferView();
  ArrayBufferView(const ArrayBufferView& other);
  ArrayBufferView & operator=(const ArrayBufferView& other);

  template<typename T>
  explicit ArrayBufferView(const nonstd::span<const T>& buffer);
  template<typename T>
  explicit ArrayBufferView(const nonstd::span<T>& buffer);

  explicit ArrayBufferView(const Int8Array& buffer);
  explicit ArrayBufferView(const ArrayBuffer& buffer);
  explicit ArrayBufferView(const Uint16Array& buffer);
  explicit ArrayBufferView(const Uint32Array& buffer);
  explicit ArrayBufferView(const Float32Array& buffer);

  [[nodiscard]] size_t byteLength() const;
  [[nodiscard]] operator bool() const;

  // Int8Span_ro int8Span() const { return _int8Array;}; // unused
  // Int8Span_rw int8Span_rw() { return _int8Array;}; // unused

  Uint8Span_ro uint8Span() const { return _uint8Array;};
  Uint8Span_rw uint8Span_rw() { return _uint8Array;};

  // Int16Span_ro int16Span() const { return _int16Array;}; // unused
  // Int16Span_rw int16Span_rw() { return _int16Array;}; // unused

  Uint16Span_ro uint16Span() const { return _uint16Array;};
  // Uint16Span_rw uint16Span_rw() { return _uint16Array;}; // unused

  // Int32Span_ro int32Span() const { return _int32Array;}; // unused
  // Int32Span_rw int32Span_rw() { return _int32Array;}; // unused

  Uint32Span_ro uint32Span() const { return _uint32Array;};
  // Uint32Span_rw uint32Span_rw() { return _uint32Array;}; // unused

  Float32Span_ro float32Span() const { return _float32Array;};
  Float32Span_rw float32Span_rw() { return _float32Array;};

private:
  template<typename T> void initFromVector(const std::vector<T> & buffer);
  template<typename T> void copyData(const nonstd::span<T> & buffer);
  void initSpans_ShareMemory_Gory();

  //
  // members
  //

  // This is the actual storage
  std::vector<byte> _sharedMemoryBuffer;
  size_t byteOffset = 0;

  // Those are reinterpreted pointer to the storage!
  Int8Span_rw _int8Array;
  Uint8Span_rw _uint8Array;
  Int16Span_rw _int16Array;
  Uint16Span_rw _uint16Array;
  Int32Span_rw _int32Array;
  Uint32Span_rw _uint32Array;
  Float32Span_rw _float32Array;
}; // end of class ArrayBufferView


template<typename T> void ArrayBufferView::initFromVector(const std::vector<T> & buffer)
{
  copyData(stl_util::as_span(buffer));
  initSpans_ShareMemory_Gory();
}

template<typename T>
inline void ArrayBufferView::copyData(const nonstd::span<T> & buffer)
{
  size_t length_bytes = buffer.size() * sizeof(T);
  _sharedMemoryBuffer.resize(length_bytes);
  std::memcpy(_sharedMemoryBuffer.data(), buffer.data(), length_bytes);
}

template<typename T>
inline ArrayBufferView::ArrayBufferView(const nonstd::span<const T>& buffer)
{
  copyData(buffer);
  initSpans_ShareMemory_Gory();
}

template<typename T>
inline ArrayBufferView::ArrayBufferView(const nonstd::span<T>& buffer)
{
  copyData(buffer);
  initSpans_ShareMemory_Gory();
}

} // end of namespace BABYLON

#endif // BABYLON_CORE_ARRAY_BUFFER_VIEW_H
