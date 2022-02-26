/**
 * @file
 */

#include "MementoHandler.h"

#include "core/ArrayLength.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "command/Command.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/Log.h"
#include "core/Zip.h"

namespace voxedit {

static const MementoState InvalidMementoState{MementoType::Max, MementoData(), -1, -1, "", voxel::Region::InvalidRegion};
const int MementoHandler::MaxStates = 64;

MementoData::MementoData(const uint8_t* buf, size_t bufSize,
		const voxel::Region& _region) :
		_compressedSize(bufSize), _region(_region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t*)core_malloc(_compressedSize);
		core_memcpy(_buffer, buf, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(MementoData&& o) noexcept :
		_compressedSize(std::exchange(o._compressedSize, 0)),
		_buffer(std::exchange(o._buffer, nullptr)),
		_region(o._region) {
}

MementoData::~MementoData() {
	if (_buffer != nullptr) {
		core_free(_buffer);
		_buffer = nullptr;
	}
}

MementoData::MementoData(const MementoData& o) :
		_compressedSize(o._compressedSize),
		_region(o._region) {
	if (o._buffer != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t*)core_malloc(_compressedSize);
		core_memcpy(_buffer, o._buffer, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData& MementoData::operator=(MementoData &&o) noexcept {
	if (this != &o) {
		_compressedSize = std::exchange(o._compressedSize, 0);
		if (_buffer) {
			core_free(_buffer);
		}
		_buffer = std::exchange(o._buffer, nullptr);
		_region = o._region;
	}
	return *this;
}

MementoData MementoData::fromVolume(const voxel::RawVolume* volume) {
	if (volume == nullptr) {
		return MementoData();
	}
	const size_t uncompressedBufferSize = volume->region().voxels() * sizeof(voxel::Voxel);
	const uint32_t compressedBufferSize = core::zip::compressBound(uncompressedBufferSize);
	uint8_t* compressedBuf = (uint8_t*)core_malloc(compressedBufferSize);
	size_t finalBufSize = 0u;
	if (!core::zip::compress(volume->data(), uncompressedBufferSize, compressedBuf, compressedBufferSize, &finalBufSize)) {
		core_free(compressedBuf);
		return MementoData();
	}
	MementoData data(compressedBuf, finalBufSize, volume->region());
	core_free(compressedBuf);

	Log::debug("Memento state. Volume: %i, compressed: %i",
			(int)uncompressedBufferSize, (int)data._compressedSize);
	return data;
}

voxel::RawVolume* MementoData::toVolume(const MementoData& mementoData) {
	if (mementoData._buffer == nullptr) {
		return nullptr;
	}
	const size_t uncompressedBufferSize = mementoData._region.voxels() * sizeof(voxel::Voxel);
	uint8_t *uncompressedBuf = (uint8_t*)core_malloc(uncompressedBufferSize);
	if (!core::zip::uncompress(mementoData._buffer, mementoData._compressedSize, uncompressedBuf, uncompressedBufferSize)) {
		return nullptr;
	}
	return voxel::RawVolume::createRaw((voxel::Voxel*)uncompressedBuf, mementoData._region);
}

MementoHandler::MementoHandler() {
}

MementoHandler::~MementoHandler() {
	shutdown();
}

bool MementoHandler::init() {
	_states.reserve(MaxStates);
	return true;
}

void MementoHandler::shutdown() {
	clearStates();
}

void MementoHandler::lock() {
	++_locked;
}

void MementoHandler::unlock() {
	--_locked;
}

void MementoHandler::print() const {
	Log::info("Current memento state index: %i", _statePosition);
	Log::info("Maximum memento states: %i", MaxStates);
	int i = 0;

	const char *states[] = {
		"Modification",
		"SceneNodeMove",
		"SceneNodeAdded",
		"SceneNodeRemoved",
		"SceneNodeRenamed"
	};
	static_assert((int)MementoType::Max == lengthof(states), "Array sizes don't match");

	for (MementoState& state : _states) {
		const glm::ivec3& mins = state.region.getLowerCorner();
		const glm::ivec3& maxs = state.region.getUpperCorner();
		Log::info("%4i: (%s) node id: %i (parent: %i) - %s (%s) [mins(%i:%i:%i)/maxs(%i:%i:%i)] (size: %ib)",
				i++, states[(int)state.type], state.nodeId, state.parentId, state.name.c_str(), state.data._buffer == nullptr ? "empty" : "volume",
						mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, (int)state.data.size());
	}
}

void MementoHandler::construct() {
	command::Command::registerCommand("ve_mementoinfo", [&] (const command::CmdArgs& args) {
		print();
	});
}

void MementoHandler::clearStates() {
	_states.clear();
	_statePosition = 0u;
}

MementoState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidMementoState;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const MementoState& s = state();
	--_statePosition;
	if (s.type != MementoType::Modification) {
		return s;
	}
	for (uint8_t i = _statePosition; i > 0; --i) {
		MementoState& prevS = _states[i];
		if ((prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) && prevS.nodeId == s.nodeId) {
			core_assert(prevS.hasVolumeData());
			// use the region from the current state - but the volume from the previous state of this node
			return MementoState{s.type, prevS.data, s.parentId, s.nodeId, s.name, s.region};
		}
	}
	core_assert(_states[0].type == MementoType::Modification);
	return _states[0];
}

MementoState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidMementoState;
	}
	++_statePosition;
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	return state();
}

void MementoHandler::updateNodeId(int nodeId, int newNodeId) {
	for (MementoState& state : _states) {
		if (state.nodeId == nodeId) {
			state.nodeId = newNodeId;
		}
		if (state.parentId == nodeId) {
			state.parentId = newNodeId;
		}
	}
}

void MementoHandler::markNodeRemoved(int parentId, int nodeId, const core::String& name, const voxel::RawVolume* volume) {
	Log::debug("Mark node %i as deleted (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeRemoved);
}

void MementoHandler::markNodeAdded(int parentId, int nodeId, const core::String& name, const voxel::RawVolume* volume) {
	Log::debug("Mark node %i as added (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeAdded);
}

void MementoHandler::markUndo(int parentId, int nodeId, const core::String& name, const voxel::RawVolume* volume, MementoType type, const voxel::Region& region) {
	if (_locked > 0) {
		Log::debug("Don't add undo state - we are currently in locked mode");
		return;
	}
	core_assert(nodeId >= 0);
	if (!_states.empty()) {
		// if we mark something as new undo state, we can throw away
		// every other state that follows the new one (everything after
		// the current state position)
		_states.erase(_statePosition + 1, _states.size());
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume);
	_states.emplace_back(type, data, parentId, nodeId, name, region);
	while (_states.size() > MaxStates) {
		_states.erase(0);
	}
	_statePosition = stateSize() - 1;
}

}
