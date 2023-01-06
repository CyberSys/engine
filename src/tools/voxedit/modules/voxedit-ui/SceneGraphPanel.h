/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"

namespace video {
class Camera;
}
namespace voxelformat {
class SceneGraph;
class SceneGraphNode;
}

namespace voxedit {

struct ModelNodeSettings;

class SceneGraphPanel {
private:
	void newLayerButton(const voxelformat::SceneGraph &sceneGraph, ModelNodeSettings* layerSettings);
	core::VarPtr _animationSpeedVar;
	bool _showNodeDetails = true;
	bool _hasFocus = false;

public:
	bool _popupNewModelNode = false;
	void update(video::Camera& camera, const char *title, ModelNodeSettings* layerSettings, command::CommandExecutionListener &listener);
	bool hasFocus() const;
};

}
