#include "ComputeBuffer.h"
#include <cstring>
#include <stdexcept>

namespace StonePotatoes {
namespace GPU {

ComputeBuffer::ComputeBuffer(size_t elementSize, size_t count,
                             BufferUsage usage) {
    Allocate(elementSize, count, usage);
}

void ComputeBuffer::Allocate(size_t elementSize, size_t count,
                             BufferUsage usage) {
    if (elementSize == 0 || count == 0) {
        throw std::invalid_argument("ComputeBuffer: zero size");
    }
    m_ElementSize = elementSize;
    m_Count = count;
    m_Usage = usage;
    m_Data.assign(elementSize * count, uint8_t{0});
}

void ComputeBuffer::Release() {
    m_Data.clear();
    m_Data.shrink_to_fit();
    m_Count = 0;
    m_ElementSize = 0;
    m_Usage = BufferUsage::None;
}

void ComputeBuffer::Upload(const void* src, size_t bytes,
                           size_t dstOffset) {
    if (dstOffset + bytes > m_Data.size()) {
        throw std::out_of_range("ComputeBuffer::Upload overflow");
    }
    std::memcpy(m_Data.data() + dstOffset, src, bytes);
}

void ComputeBuffer::Download(void* dst, size_t bytes,
                             size_t srcOffset) const {
    if (srcOffset + bytes > m_Data.size()) {
        throw std::out_of_range("ComputeBuffer::Download overflow");
    }
    std::memcpy(dst, m_Data.data() + srcOffset, bytes);
}

} // namespace GPU
} // namespace StonePotatoes
