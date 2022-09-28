/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 *
 * https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt
 * https://gist.github.com/falkreon/8b873ec6797ffad247375fc73614fd08
 *
 * @ingroup Formats
 */
class KV6Format : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
