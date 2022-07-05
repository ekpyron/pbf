//
// Created by daniel on 04.07.22.
//

#ifndef PBF_INDIRECTCOMMANDSBUFFER_H
#define PBF_INDIRECTCOMMANDSBUFFER_H

#include "common.h"
#include <map>
#include <crampl/MultiKeyMap.h>
#include <list>
#include "Context.h"
#include "Buffer.h"
#include "Quad.h"


namespace pbf {

class IndirectCommandsBuffer
{
public:
	IndirectCommandsBuffer(pbf::Context *context);
	void clear();
	void push_back(const vk::DrawIndirectCommand &cmd);
	const auto &buffers() const { return _buffers; }
	std::uint32_t elementsInLastBuffer() const { return _elementsInLastBuffer; }
	static constexpr std::uint32_t bufferSize = 128;
private:
	pbf::Context *context;
	std::list<pbf::Buffer> _buffers;
	std::list<pbf::Buffer>::iterator _currentBuffer;
	std::uint32_t _elementsInLastBuffer {bufferSize};
};

}


#endif //PBF_INDIRECTCOMMANDSBUFFER_H