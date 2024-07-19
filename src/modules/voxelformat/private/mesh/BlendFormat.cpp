/**
 * @file
 */

#include "BlendFormat.h"
#include "BlendShared.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/collection/DynamicArray.h"
#include "io/EndianStreamReadWrapper.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"

namespace voxelformat {

// prevent endless reading on malformed files
static constexpr int MaxStringLength = 1000;

static bool readChunk(DNAChunk &chunk, io::EndianStreamReadWrapper &stream, bool is64Bit) {
	if (stream.readUInt32(chunk.identifier) != 0) {
		return false;
	}
	if (stream.readUInt32(chunk.length) != 0) {
		return false;
	}
	uint8_t buffer[4];
	FourCCRev(buffer, chunk.identifier);
	Log::debug("Found chunk %c%c%c%c: len %i", buffer[0], buffer[1], buffer[2], buffer[3], chunk.length);
	if (is64Bit) {
		if (stream.readUInt64(chunk.oldMemoryAddress) != 0) {
			return false;
		}
	} else {
		uint32_t addr;
		if (stream.readUInt32(addr) != 0) {
			return false;
		}
		chunk.oldMemoryAddress = addr;
	}
	if (stream.readUInt32(chunk.indexSDNA) != 0) {
		return false;
	}
	if (stream.readUInt32(chunk.count) != 0) {
		return false;
	}

	return true;
}

static bool skipChunk(const DNAChunk &chunk, io::EndianStreamReadWrapper &stream) {
	uint8_t buffer[4];
	FourCCRev(buffer, chunk.identifier);
	Log::debug("Skip chunk %c%c%c%c: len %i", buffer[0], buffer[1], buffer[2], buffer[3], chunk.length);
	return stream.skipDelta(chunk.length) == (int64_t)chunk.length;
}

static bool readChunkDNA1Names(core::DynamicArray<core::String> &names, io::EndianStreamReadWrapper &stream) {
	uint32_t nameChunkId;
	if (stream.readUInt32(nameChunkId) != 0) {
		Log::error("Could not read name chunk id from DNA1");
		return false;
	}
	if (nameChunkId != FourCC('N', 'A', 'M', 'E')) {
		Log::error("Invalid chunk id in DNA1 - expected NAME");
		return false;
	}
	uint32_t namesCount;
	if (stream.readUInt32(namesCount) != 0) {
		Log::error("Could not read name chunk length from DNA1");
		return false;
	}

	names.reserve(namesCount);
	uint32_t bytes = 0;
	for (uint32_t n = 0; n < namesCount; ++n) {
		core::String name;
		if (!stream.readString(MaxStringLength, name, true)) {
			Log::error("Could not read name from DNA1");
			return false;
		}
		names.push_back(name);
		bytes += name.size() + 1;
	}
	// alignment
	Log::debug("read %i bytes from %i names", (int)bytes, (int)namesCount);
	stream.skipDelta(bytes & 0x3);

	return true;
}

static bool readChunkDNA1Types(core::DynamicArray<Type> &types, io::EndianStreamReadWrapper &stream) {
	uint32_t typeChunkId;
	if (stream.readUInt32(typeChunkId) != 0) {
		Log::error("Could not read type chunk id from DNA1");
		return false;
	}
	if (typeChunkId != FourCC('T', 'Y', 'P', 'E')) {
		Log::error("Invalid chunk id in DNA1 - expected TYPE");
		return false;
	}
	uint32_t typesCount;
	if (stream.readUInt32(typesCount) != 0) {
		Log::error("Could not read type chunk length from DNA1");
		return false;
	}

	types.reserve(typesCount);
	uint32_t bytes = 0;
	for (uint32_t n = 0; n < typesCount; ++n) {
		Type type;
		if (!stream.readString(MaxStringLength, type.name, true)) {
			Log::error("Could not read type name from DNA1");
			return false;
		}
		types.push_back(type);
		bytes += type.name.size() + 1;
	}
	// alignment
	stream.skipDelta(bytes & 0x3);

	uint32_t typeLenChunkId;
	if (stream.readUInt32(typeLenChunkId) != 0) {
		Log::error("Could not read type length chunk id from DNA1");
		return false;
	}
	if (typeLenChunkId != FourCC('T', 'L', 'E', 'N')) {
		Log::error("Invalid chunk id in DNA1 - expected TLEN");
		return false;
	}

	bytes = 0;
	for (Type &type : types) {
		if (stream.readInt16(type.size) != 0) {
			Log::error("Could not read type size from DNA1");
			return false;
		}
		bytes += 2;
	}
	// alignment
	stream.skipDelta(bytes & 0x3);
	return true;
}

static bool readChunkDNA1Structures(core::DynamicArray<Structure> &structures, const core::DynamicArray<Type> &types,
									const core::DynamicArray<core::String> &names, io::EndianStreamReadWrapper &stream,
									bool is64Bit) {
	uint32_t structureChunkId;
	if (stream.readUInt32(structureChunkId) != 0) {
		Log::error("Could not read structure chunk id from DNA1");
		return false;
	}
	if (structureChunkId != FourCC('S', 'T', 'R', 'C')) {
		Log::error("Invalid chunk id in DNA1 - expected STRC");
		return false;
	}
	uint32_t structureCount;
	if (stream.readUInt32(structureCount) != 0) {
		Log::error("Could not read structure chunk length from DNA1");
		return false;
	}

	Log::debug("Structure count %i", (int)structureCount);
	structures.reserve(structureCount);
	for (uint32_t n = 0; n < structureCount; ++n) {
		Structure structure;
		if (stream.readUInt16(structure.type) != 0) {
			Log::error("Could not read structure type from DNA1");
			return false;
		}
		structure.name = types[structure.type].name;
		uint16_t fieldCount;
		if (stream.readUInt16(fieldCount) != 0) {
			Log::error("Could not read structure field count from DNA1");
			return false;
		}
		Log::debug("Field count %i", (int)fieldCount);
		structure.fields.reserve(fieldCount);
		for (uint16_t f = 0; f < fieldCount; ++f) {
			Field field;
			uint16_t typeIdx;
			if (stream.readUInt16(typeIdx) != 0) {
				Log::error("Could not read field type from DNA1");
				return false;
			}
			uint16_t nameIdx;
			if (stream.readUInt16(nameIdx) != 0) {
				Log::error("Could not read field name from DNA1");
				return false;
			}
			field.type = types[typeIdx].name;
			field.name = names[nameIdx];
			calcSize(field, types[typeIdx], is64Bit);
			Log::debug("field name %s type %s size %i", field.name.c_str(), field.type.c_str(), (int)field.size);
			structure.fields.push_back(field);
		}
		structures.push_back(structure);
	}
	return true;
}

static bool dna_Object(const Structure &structure) {
	Log::debug("Object %s", structure.name.c_str());
	return false; // TODO:
}

static bool readChunkDNA1(DNAChunk &chunk, io::EndianStreamReadWrapper &stream, bool is64Bit) {
	uint32_t chunkId;
	if (stream.readUInt32(chunkId) != 0) {
		Log::error("Could not read chunk id from DNA1");
		return false;
	}
	if (chunkId != FourCC('S', 'D', 'N', 'A')) {
		Log::error("Invalid chunk id in DNA1 - expected SDNA");
		return false;
	}

	core::DynamicArray<core::String> names;
	if (!readChunkDNA1Names(names, stream)) {
		return false;
	}

	core::DynamicArray<Type> types;
	if (!readChunkDNA1Types(types, stream)) {
		return false;
	}

	core::DynamicArray<Structure> structures;
	if (!readChunkDNA1Structures(structures, types, names, stream, is64Bit)) {
		return false;
	}

	const struct Handler {
		const char *name;
		bool (*handler)(const Structure &structure);
	} DNAHandlers[] {
		{"Object", dna_Object}
	};
	for (const Structure &structure : structures) {
		for (const Handler &handler : DNAHandlers) {
			if (structure.name != handler.name) {
				continue;
			}
			core_assert(handler.handler);
			if (!handler.handler(structure)) {
				Log::error("Failed to load structure '%s'", structure.name.c_str());
				return false;
			}
			Log::debug("Successfully loaded structure '%s'", structure.name.c_str());
		}
	}

	return true;
}

bool BlendFormat::loadBlend(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, io::ReadStream &stream) const {
	uint8_t pointerSize;
	if (stream.readUInt8(pointerSize) != 0) {
		Log::error("Could not read pointer size from file %s", filename.c_str());
		return false;
	}

	uint8_t endianess;
	if (stream.readUInt8(endianess) != 0) {
		Log::error("Could not read endianess from file %s", filename.c_str());
		return false;
	}

	uint8_t version[3];
	if (stream.read(version, sizeof(version)) != 3) {
		Log::error("Could not read version from file %s", filename.c_str());
		return false;
	}

	// _ is 32 bit, - is 64 bit
	const bool is64Bit = pointerSize == '-';
	// V is big endian, v is little endian
	const bool isBigEndian = endianess == 'V';

	Log::debug("found blender version %c.%c%c %s %s", version[0], version[1], version[2], is64Bit ? "64 bit" : "32 bit",
			   isBigEndian ? "big endian" : "little endian");

	if (!is64Bit) {
		Log::error("Only 64 bit blend files are supported");
		return false;
	}
	io::EndianStreamReadWrapper endianStream(stream, isBigEndian);
	DNAChunk chunk;
	while (readChunk(chunk, endianStream, is64Bit)) {
		switch (chunk.identifier) {
		case FourCC('E', 'N', 'D', 'B'): {
			break;
		}
		case FourCC('D', 'N', 'A', '1'): {
			if (!readChunkDNA1(chunk, endianStream, is64Bit)) {
				return false;
			}
			break;
		}
		default: {
			skipChunk(chunk, endianStream);
			break;
		}
		}
	}
	return true;
}

bool BlendFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	uint8_t magic[7]; // BLENDER
	if (stream->read(magic, sizeof(magic)) != 7) {
		Log::error("Could not read magic from file %s", filename.c_str());
		return false;
	}

	if (core_memcmp(magic, "BLENDER", 7) != 0) {
		io::ZipReadStream zipStream(*stream);
		if (zipStream.err()) {
			Log::error("Could not load blend file %s", filename.c_str());
			return false;
		}

		if (zipStream.read(magic, sizeof(magic)) != 7) {
			Log::error("Could not read magic from file %s", filename.c_str());
			return false;
		}
		if (core_memcmp(magic, "BLENDER", 7) != 0) {
			Log::error("Invalid magic in compressed file %s", filename.c_str());
			return false;
		}

		return loadBlend(filename, archive, sceneGraph, ctx, zipStream);
	}

	return loadBlend(filename, archive, sceneGraph, ctx, *stream);
}

} // namespace voxelformat
