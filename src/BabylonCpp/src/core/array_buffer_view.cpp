#include <babylon/core/array_buffer_view.h>

#include <babylon/babylon_stl_util.h>

namespace BABYLON {

template<typename DestinationType>
nonstd::span<DestinationType> reinterpret_buffer_as_span(
  std::vector<byte> & buffer)
{
  nonstd::span<DestinationType> r(
    reinterpret_cast<DestinationType *>(buffer.data()),
    buffer.size() / sizeof(DestinationType)
    );
  return r;
}

void ArrayBufferView::initSpans_ShareMemory_Gory()
{
  this->_int8Array = reinterpret_buffer_as_span<int8_t>(_sharedMemoryBuffer);
  this->_uint8Array = reinterpret_buffer_as_span<uint8_t>(_sharedMemoryBuffer);
  this->_int16Array = reinterpret_buffer_as_span<int16_t>(_sharedMemoryBuffer);
  this->_uint16Array = reinterpret_buffer_as_span<uint16_t>(_sharedMemoryBuffer);
  this->_int32Array = reinterpret_buffer_as_span<int32_t>(_sharedMemoryBuffer);
  this->_uint32Array = reinterpret_buffer_as_span<uint32_t>(_sharedMemoryBuffer);
  this->_float32Array = reinterpret_buffer_as_span<float>(_sharedMemoryBuffer);
}

ArrayBufferView::ArrayBufferView()
{
  std::vector<uint8_t> emptyBuffer;
  initFromVector(emptyBuffer);
}

ArrayBufferView::ArrayBufferView(const Int8Array& buffer)
{
  initFromVector(buffer);
}

ArrayBufferView::ArrayBufferView(const ArrayBuffer& buffer)
{
  initFromVector(buffer);
}

ArrayBufferView::ArrayBufferView(const ::BABYLON::Uint16Array& buffer)
{
  initFromVector(buffer);
}

ArrayBufferView::ArrayBufferView(const ::BABYLON::Uint32Array& buffer)
{
  initFromVector(buffer);
}

ArrayBufferView::ArrayBufferView(const Float32Array& buffer)
{
  initFromVector(buffer);
}

size_t ArrayBufferView::byteLength() const
{
  return _sharedMemoryBuffer.size();
}

ArrayBufferView::operator bool() const
{
  return _sharedMemoryBuffer.empty();
}

ArrayBufferView & ArrayBufferView::operator=(const ArrayBufferView& other)
{
  if (this != &other) {
    copyData(other._uint8Array);
    initSpans_ShareMemory_Gory();
  }
  return *this;
}

ArrayBufferView::ArrayBufferView(const ArrayBufferView& other)
{
  copyData(other._uint8Array);
  initSpans_ShareMemory_Gory();
}


} // end of namespace BABYLON
