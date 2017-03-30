#pragma once

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <glm/exponential.hpp>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include <glm/gtc/round.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <limits>
#include <cmath>

struct Vec3Hash {
	inline size_t operator()(const glm::vec3& k) const {
		return std::hash<float>()(k.x) ^ std::hash<float>()(k.y) ^ std::hash<float>()(k.z);
	}

	inline bool operator()(const glm::vec3& a, const glm::vec3& b) const {
		const float eps = a.x - b.x < glm::epsilon<float>();
		return a.x - b.x < eps && a.y - b.y < eps && a.z - b.z < eps;
	}
};

namespace glm {
extern const glm::vec3 forward;
extern const glm::vec3 backward;
extern const glm::vec3 right;
extern const glm::vec3 left;
extern const glm::vec3 up;
extern const glm::vec3 down;

GLM_FUNC_QUALIFIER vec3 transform(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	vec3 r(uninitialize);
	r.x = c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w;
	r.y = c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w;
	r.z = c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w;
	return r;
}

GLM_FUNC_QUALIFIER vec3 rotate(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	vec3 r(uninitialize);
	r.x = c1.x * v.x + c1.y * v.y + c1.z * v.z;
	r.y = c2.x * v.x + c2.y * v.y + c2.z * v.z;
	r.z = c3.x * v.x + c3.y * v.y + c3.z * v.z;
	return r;
}

GLM_FUNC_QUALIFIER vec3 project(const mat4& m, const vec3& p) {
	const vec4& r = m * vec4(p, 1);
	return vec3(r) / r.w;
}

// TODO: will be part of glm 0.9.8
GLM_FUNC_QUALIFIER bvec4 isnan(quat const & x) {
	const vec4 v(x.x, x.y, x.z, x.w);
	return isnan(v);
}

GLM_FUNC_QUALIFIER bvec4 isinf(quat const & x) {
	const vec4 v(x.x, x.y, x.z, x.w);
	return isinf(v);
}

}
