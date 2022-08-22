#include "IndirectCommandsBuffer.h"

namespace pbf {

void IndirectCommandsBuffer::clear() {
	_currentBuffer = _buffers.begin();
	_elementsInLastBuffer = 0;
}

IndirectCommandsBuffer::IndirectCommandsBuffer(pbf::Context &context) :context(context) {
	_buffers.emplace_back(context, bufferSize, vk::BufferUsageFlagBits::eIndirectBuffer, MemoryType::DYNAMIC);
	_currentBuffer = _buffers.begin();
	_elementsInLastBuffer = 0;

}

void IndirectCommandsBuffer::push_back(const vk::DrawIndirectCommand &cmd) {
	if (_elementsInLastBuffer >= bufferSize) {
		++_currentBuffer;
		if (_currentBuffer == _buffers.end())
		{
			_buffers.emplace_back(context, bufferSize, vk::BufferUsageFlagBits::eIndirectBuffer, MemoryType::DYNAMIC);
			_currentBuffer = std::prev(_buffers.end());
		}
		_elementsInLastBuffer = 0;
	}
	_currentBuffer->data()[_elementsInLastBuffer++] = cmd;

}

}
