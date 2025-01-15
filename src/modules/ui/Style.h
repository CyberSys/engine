/**
 * @file
 */

#pragma once

#include "core/Color.h"

namespace style {

enum StyleColor {
	ColorLockedNode,
	ColorReferenceNode,
	ColorInactiveNode,
	ColorGroupNode,
	ColorActiveNode,
	ColorHighlightArea,
	ColorGridBorder,
	ColorBone,
	ColorUVEditor,
	ColorAxisX,
	ColorAxisY,
	ColorAxisZ,
	ColorSliceRegion
};

const glm::vec4 &color(StyleColor color);

} // namespace style
