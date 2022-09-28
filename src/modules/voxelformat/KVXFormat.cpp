/**
 * @file
 */

#include "KVXFormat.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "io/FileStream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/PaletteLookup.h"
#include "voxel/Palette.h"
#include <glm/common.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KVXFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level
	uint32_t numbytes;
	wrap(stream.readUInt32(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream.readUInt32(xsiz_w))
	wrap(stream.readUInt32(ysiz_d))
	wrap(stream.readUInt32(zsiz_h))

	if (xsiz_w > 256 || ysiz_d > 256 || zsiz_h > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	const voxel::Region region(0, 0, 0, (int)xsiz_w - 1, (int)zsiz_h - 1, (int)ysiz_d - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	SceneGraphTransform transform;
	uint32_t pivx_w, pivy_d, pivz_h;
	wrap(stream.readUInt32(pivx_w))
	wrap(stream.readUInt32(pivy_d))
	wrap(stream.readUInt32(pivz_h))

	pivx_w >>= 8;
	pivy_d >>= 8;
	pivz_h >>= 8;

	pivz_h = zsiz_h - 1 - pivz_h;

	glm::vec3 normalizedPivot;
	normalizedPivot.x = (float)pivx_w / (float)xsiz_w;
	normalizedPivot.y = (float)pivy_d / (float)ysiz_d;
	normalizedPivot.z = (float)pivz_h / (float)zsiz_h;
	core::exchange(normalizedPivot.y, normalizedPivot.z);
	transform.setPivot(normalizedPivot);

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint16_t xyoffset[256][257];
	uint32_t xoffset[257];
	for (uint32_t x = 0u; x <= xsiz_w; ++x) {
		wrap(stream.readUInt32(xoffset[x]))
	}

	for (uint32_t x = 0u; x < xsiz_w; ++x) {
		for (uint32_t y = 0u; y <= ysiz_d; ++y) {
			wrap(stream.readUInt16(xyoffset[x][y]))
		}
	}

	const uint32_t offset = (xsiz_w + 1) * 4 + xsiz_w * (ysiz_d + 1) * 2;
	if (xoffset[0] != offset) {
		Log::error("Invalid offset values found");
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const size_t currentPos = stream.pos();
	palette.colorCount = voxel::PaletteMaxColors;
	stream.seek(-3l * palette.colorCount, SEEK_END);

	/**
	 * The last 768 bytes of the KVX file is a standard 256-color VGA palette.
	 * The palette is in (Red:0, Green:1, Blue:2) order and intensities range
	 * from 0-63.
	 */
	for (int i = 0; i < palette.colorCount; ++i) {
		uint8_t r, g, b;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))

		const uint8_t nr = glm::clamp((uint32_t)glm::round(((float)r * 255.0f) / 63.0f), 0u, 255u);
		const uint8_t ng = glm::clamp((uint32_t)glm::round(((float)g * 255.0f) / 63.0f), 0u, 255u);
		const uint8_t nb = glm::clamp((uint32_t)glm::round(((float)b * 255.0f) / 63.0f), 0u, 255u);

		const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255);
		palette.colors[i] = core::Color::getRGBA(color);
	}
	stream.seek((int64_t)currentPos);

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	/**
	 * voxdata: stored in sequential format.  Here's how you can get pointers to
	 * the start and end of any (x, y) column:
	 *
	 * @code
	 * //pointer to start of slabs on column (x, y):
	 * startptr = &voxdata[xoffset[x] + xyoffset[x][y]];
	 *
	 * //pointer to end of slabs on column (x, y):
	 * endptr = &voxdata[xoffset[x] + xyoffset[x][y+1]];
	 * @endcode
	 *
	 * Note: endptr is actually the first piece of data in the next column
	 *
	 * Once you get these pointers, you can run through all of the "slabs" in
	 * the column. Each slab has 3 bytes of header, then an array of colors.
	 * Here's the format:
	 *
	 * @code
	 * char slabztop;             //Starting z coordinate of top of slab
	 * char slabzleng;            //# of bytes in the color array - slab height
	 * char slabbackfacecullinfo; //Low 6 bits tell which of 6 faces are exposed
	 * char col[slabzleng];       //The array of colors from top to bottom
	 * @endcode
	 */

	struct slab {
		uint8_t slabztop;
		uint8_t slabzleng;
		uint8_t slabbackfacecullinfo;
	};

	uint32_t lastZ = 0;
	voxel::Voxel lastCol;

	for (uint32_t x = 0; x < xsiz_w; ++x) {
		for (uint32_t y = 0; y < ysiz_d; ++y) {
			const uint16_t end = xyoffset[x][y + 1];
			const uint16_t start = xyoffset[x][y];
			int32_t n = end - start;

			while (n > 0) {
				slab header;
				wrap(stream.readUInt8(header.slabztop))
				wrap(stream.readUInt8(header.slabzleng))
				wrap(stream.readUInt8(header.slabbackfacecullinfo))
				for (uint8_t i = 0u; i < header.slabzleng; ++i) {
					uint8_t col;
					wrap(stream.readUInt8(col))
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, col);
					volume->setVoxel((int)x, (int)((zsiz_h - 1) - (header.slabztop + i)), (int)y, lastCol);
				}

				/**
				 * the format only saves the visible voxels - we have to take the face info to
				 * fill the inner voxels
				 */
				if (!(header.slabbackfacecullinfo & (1 << 4))) {
					for (uint32_t i = lastZ + 1; i < header.slabztop; ++i) {
						volume->setVoxel((int)x, (int)((zsiz_h - 1) - i), (int)y, lastCol);
					}
				}
				if (!(header.slabbackfacecullinfo & (1 << 5))) {
					lastZ = header.slabztop + header.slabzleng;
				}
				n -= (int32_t)(header.slabzleng + sizeof(header));
			}
		}
	}

	return true;
}

#undef wrap

bool KVXFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

}
