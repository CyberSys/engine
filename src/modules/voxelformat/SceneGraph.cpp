/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Common.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeMerger.h"

namespace voxel {

SceneGraph::SceneGraph() {
	clear();
}

SceneGraph::~SceneGraph() {
	clear();
}

int SceneGraph::activeNode() const {
	return _activeNodeId;
}

bool SceneGraph::setActiveNode(int nodeId) {
	if (!hasNode(nodeId)) {
		return false;
	}
	_activeNodeId = nodeId;
	return true;
}

int SceneGraph::nextLockedNode(int last) const {
	++last;
	if (last < 0) {
		return -1;
	}
	const int n = (int)size();
	for (int i = last; i < n; ++i) {
		if ((*this)[i].locked()) {
			return i;
		}
	}
	return -1;
}

void SceneGraph::foreachGroup(const std::function<void(int)>& f) {
	int nodeId = activeNode();
	if (node(nodeId).locked()) {
		nodeId = nextLockedNode(-1);
		core_assert(nodeId != -1);
		while (nodeId != -1) {
			f(nodeId);
			nodeId = nextLockedNode(nodeId);
		}
	} else {
		f(nodeId);
	}
}

SceneGraphNode& SceneGraph::node(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::error("No node for id %i found in the scene graph - returning root node", nodeId);
		return _nodes.find(0)->value;
	}
	return iter->value;
}

bool SceneGraph::hasNode(int nodeId) const {
	return _nodes.find(nodeId) != _nodes.end();
}

const SceneGraphNode& SceneGraph::root() const {
	return node(0);
}

voxel::Region SceneGraph::region() const {
	voxel::Region r;
	bool validVolume = false;
	for (const voxel::SceneGraphNode& node : *this) {
		if (validVolume) {
			r.accumulate(node.region());
			continue;
		}
		r = node.region();
		validVolume = true;
	}
	return r;
}

int SceneGraph::emplace(SceneGraphNode &&node, int parent) {
	if (node.type() == SceneGraphNodeType::Root && _nextNodeId != 0) {
		Log::error("No second root node is allowed in the scene graph");
		node.release();
		return -1;
	}
	const int nodeId = _nextNodeId;
	if (parent >= nodeId) {
		Log::error("Invalid parent id given: %i", parent);
		node.release();
		return -1;
	}
	if (parent >= 0) {
		auto parentIter = _nodes.find(parent);
		if (parentIter == _nodes.end()) {
			Log::error("Could not find parent node with id %i", parent);
			return -1;
		}
		Log::debug("Add child %i to node %i", nodeId, parent);
		parentIter->value.addChild(nodeId);
	}
	++_nextNodeId;
	node.setId(nodeId);
	node.setParent(parent);
	Log::debug("Adding scene graph node of type %i with id %i and parent %i", (int)node.type(), node.id(), node.parent());
	_nodes.emplace(nodeId, core::forward<SceneGraphNode>(node));
	return nodeId;
}

bool SceneGraph::removeNode(int nodeId) {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::debug("Could not remove node %i - not found", nodeId);
		return false;
	}
	if (iter->value.type() == SceneGraphNodeType::Root) {
		core_assert(nodeId == 0);
		clear();
		return true;
	}
	bool state = iter->value.children().empty();
	for (int childId : iter->value.children()) {
		state |= removeNode(childId);
	}
	_nodes.erase(iter);
	return state;
}

void SceneGraph::reserve(size_t size) {
}

bool SceneGraph::empty(SceneGraphNodeType type) const {
	return begin(type) == end();
}

size_t SceneGraph::size(SceneGraphNodeType type) const {
	auto iterbegin = begin(type);
	auto iterend = end();
	size_t n = 0;
	for (auto iter = iterbegin; iter != iterend; ++iter) {
		++n;
	}
	return n;
}

void SceneGraph::clear() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
	_nextNodeId = 1;

	SceneGraphNode node(SceneGraphNodeType::Root);
	node.setName("root");
	node.setId(0);
	node.setParent(-1);
	_nodes.emplace(0, core::move(node));
}

const SceneGraphNode &SceneGraph::operator[](int modelIdx) const {
	iterator iter = begin(SceneGraphNodeType::Model);
	for (int i = 0; i < modelIdx; ++i) {
		core_assert(iter != end());
		++iter;
	}
	return *iter;
}

SceneGraphNode &SceneGraph::operator[](int modelIdx) {
	iterator iter = begin(SceneGraphNodeType::Model);
	for (int i = 0; i < modelIdx; ++i) {
		core_assert(iter != end());
		++iter;
	}
	return *iter;
}

voxel::RawVolume *SceneGraph::merge() const {
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(_nodes.size() - 1);
	for (const auto &node : *this) {
		core_assert(node.type() == SceneGraphNodeType::Model);
		core_assert(node.volume() != nullptr);
		rawVolumes.push_back(node.volume());
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	if (rawVolumes.size() == 1) {
		return new voxel::RawVolume(rawVolumes[0]);
	}
	return ::voxel::merge(rawVolumes);
}

} // namespace voxel
