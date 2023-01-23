/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/collection/DynamicArray.h"
#include "render/CameraFrustum.h"
#include "voxelformat/SceneGraph.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @see voxelformat::toCameraNode()
 */
video::Camera toCamera(const glm::ivec2 &size, const voxelformat::SceneGraphNodeCamera &cameraNode);
voxelformat::SceneGraphNodeCamera toCameraNode(const video::Camera& camera);


/**
 * @brief Rendering of a voxel::SceneGraph
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _renderer;
	render::CameraFrustum _cameraRenderer;
	core::DynamicArray<video::Camera> _cameras;
	bool _sceneMode = true;

public:
	void construct();
	bool init();
	void update();
	void shutdown();

	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);

	void setSceneMode(bool sceneMode);
	bool extractRegion(voxelformat::SceneGraphNode &node, const voxel::Region& region);
	bool toMesh(voxelformat::SceneGraphNode &node, voxel::Mesh* mesh);
	bool empty(voxelformat::SceneGraphNode &node);
	void prepare(voxelformat::SceneGraph &sceneGraph, voxelformat::FrameIndex frame = 0, bool hideInactive = false, bool grayInactive = false);
	void extractAll();
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is false, you have to call @c update() manually!
	 */
	void render(RenderContext &renderContext, const video::Camera& camera, bool shadow = true, bool waitPending = false);
	void clear();
};

inline void SceneGraphRenderer::setSceneMode(bool sceneMode) {
	_sceneMode = sceneMode;
}

} // namespace voxelrender
