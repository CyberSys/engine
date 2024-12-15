/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "MeshMaterial.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

class Polygon {
private:
	core::DynamicArray<glm::vec2> _uvs;
	core::DynamicArray<glm::vec3> _vertices;
	core::DynamicArray<core::RGBA> _colors;
	MeshMaterialPtr _material;
	void addTriangle(MeshFormat::MeshTriCollection &tris, int idx0, int idx1, int idx2) const;

public:
	Polygon &setMaterial(const MeshMaterialPtr &material);
	Polygon &addVertex(const glm::vec3 &vertex, const glm::vec2 &uv, core::RGBA color = core::RGBA(0, 0, 0));
	glm::vec2 uv(int x, int y) const;
	bool toTris(MeshFormat::MeshTriCollection &tris) const;

	glm::vec3 center() const;
	size_t size() const;
	glm::vec3 vertex(int idx) const;
	void setVertex(int idx, const glm::vec3 &vertex);
};

} // namespace voxelformat
