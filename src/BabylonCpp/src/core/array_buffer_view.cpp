#include <babylon/core/array_buffer_view.h>

#include <babylon/babylon_stl_util.h>

namespace BABYLON {

ArrayBufferView::ArrayBufferView() = default;

ArrayBufferView::ArrayBufferView(const Int8Array& buffer)
    : byteOffset{0}
    , int8Array{buffer}
    , uint8Array{stl_util::to_array<uint8_t>(buffer)}
    , int16Array{stl_util::to_array<int16_t>(buffer)}
    , uint16Array{stl_util::to_array<uint16_t>(buffer)}
    , int32Array{stl_util::to_array<int32_t>(buffer)}
    , uint32Array{stl_util::to_array<uint32_t>(buffer)}
    , float32Array{stl_util::to_array<float>(buffer)}
{
}

ArrayBufferView::ArrayBufferView(const ArrayBuffer& arrayBuffer)
    : byteOffset{0}
    , int8Array{stl_util::to_array<int8_t>(arrayBuffer)}
    , uint8Array{arrayBuffer}
    , int16Array{stl_util::to_array<int16_t>(arrayBuffer)}
    , uint16Array{stl_util::to_array<uint16_t>(arrayBuffer)}
    , int32Array{stl_util::to_array<int32_t>(arrayBuffer)}
    , uint32Array{stl_util::to_array<uint32_t>(arrayBuffer)}
    , float32Array{stl_util::to_array<float>(arrayBuffer)}
{
}

ArrayBufferView::ArrayBufferView(const Uint16Array& buffer)
    : byteOffset{0}
    , int8Array{stl_util::to_array<int8_t>(buffer)}
    , uint8Array{stl_util::to_array<uint8_t>(buffer)}
    , int16Array{stl_util::to_array<int16_t>(buffer)}
    , uint16Array{buffer}
    , int32Array{stl_util::to_array<int32_t>(buffer)}
    , uint32Array{stl_util::to_array<uint32_t>(buffer)}
    , float32Array{stl_util::to_array<float>(buffer)}
{
}

ArrayBufferView::ArrayBufferView(const Uint32Array& buffer)
    : byteOffset{0}
    , int8Array{stl_util::to_array<int8_t>(buffer)}
    , uint8Array{stl_util::to_array<uint8_t>(buffer)}
    , int16Array{stl_util::to_array<int16_t>(buffer)}
    , uint16Array{stl_util::to_array<uint16_t>(buffer)}
    , int32Array{stl_util::to_array<int32_t>(buffer)}
    , uint32Array{buffer}
    , float32Array{stl_util::to_array<float>(buffer)}
{
}

ArrayBufferView::ArrayBufferView(const Float32Array& buffer)
    : byteOffset{0}
    , int8Array{stl_util::to_array<int8_t>(buffer)}
    , uint8Array{stl_util::to_array<uint8_t>(buffer)}
    , int16Array{stl_util::to_array<int16_t>(buffer)}
    , uint16Array{stl_util::to_array<uint16_t>(buffer)}
    , int32Array{stl_util::to_array<int32_t>(buffer)}
    , uint32Array{stl_util::to_array<uint32_t>(buffer)}
    , float32Array{buffer}
{
}

ArrayBufferView::ArrayBufferView(const ArrayBufferView& other) = default;

ArrayBufferView::ArrayBufferView(ArrayBufferView&& other) = default;

ArrayBufferView& ArrayBufferView::operator=(const ArrayBufferView& other) = default;

ArrayBufferView& ArrayBufferView::operator=(ArrayBufferView&& other) = default;

ArrayBufferView::~ArrayBufferView() = default;

size_t ArrayBufferView::byteLength() const
{
  return uint8Array.size();
}

ArrayBufferView::operator bool() const
{
  return !int8Array.empty() && !uint8Array.empty() && !int16Array.empty()
         && !uint16Array.empty() && !int32Array.empty() && !uint32Array.empty()
         && !float32Array.empty();
}

} // end of namespace BABYLON
