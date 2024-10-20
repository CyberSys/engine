/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * The Minetest Schematic File Format (minetest)
 *
 * @note https://dev.minetest.net/Minetest_Schematic_File_Format
 *
 * @ingroup Formats
 */
class MTSFormat : public PaletteFormat {
public:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Minetest", {"mts"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
