/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/List.h"
#include "core/collection/StringMap.h"
#include <stdint.h>
#include <string.h>
#include <memory>
#include <vector>
#include <glm/fwd.hpp>
#include "ShaderTypes.h"

namespace video {

class UniformBuffer;

#ifndef VERTEX_POSTFIX
#define VERTEX_POSTFIX ".vert"
#endif

#ifndef FRAGMENT_POSTFIX
#define FRAGMENT_POSTFIX ".frag"
#endif

#ifndef GEOMETRY_POSTFIX
#define GEOMETRY_POSTFIX ".geom"
#endif

#ifndef COMPUTE_POSTFIX
#define COMPUTE_POSTFIX ".comp"
#endif

// activate this to validate that every uniform was set
#define VALIDATE_UNIFORMS 0

/**
 * @brief Shader wrapper for GLSL. See shadertool for autogenerated shader wrapper code
 * from vertex and fragment shaders
 * @ingroup Video
 */
class Shader {
protected:
	typedef core::Array<Id, (int)ShaderType::Max> ShaderArray;
	ShaderArray _shader { InvalidId, InvalidId, InvalidId, InvalidId };

	typedef core::Map<int, uint32_t, 8> UniformStateMap;
	mutable UniformStateMap _uniformStateMap;

	Id _program = InvalidId;
	bool _initialized = false;
	mutable bool _active = false;
	bool _dirty = true;

	typedef core::StringMap<core::String> ShaderDefines;
	ShaderDefines _defines;

	typedef core::StringMap<int> ShaderUniformArraySizes;
	ShaderUniformArraySizes _uniformArraySizes;

	ShaderUniforms _uniforms;

	TransformFeedbackCaptureMode _transformFormat = TransformFeedbackCaptureMode::Max;
	core::List<core::String> _transformVaryings;

	// can be used to validate that every uniform was set. The value type is the location index
	mutable core::Map<int, bool, 4> _usedUniforms;
	bool _recordUsedUniforms = false;
	void addUsedUniform(int location) const;

	ShaderAttributes _attributes;

	typedef core::Map<int, int, 8> AttributeComponents;
	AttributeComponents _attributeComponents;

	mutable uint32_t _time = 0u;

	core::String _name;

	const Uniform* getUniform(const core::String& name) const;

	int fetchUniforms();

	int fetchAttributes();

	bool createProgramFromShaders();

	/**
	 * @param[in] location The uniform location in the shader
	 * @param[in] value The buffer with the data
	 * @param[in] length The length in bytes of the given value buffer
	 * @return @c false if no change is needed, @c true if we have to update the value
	 */
	bool checkUniformCache(int location, const void* value, size_t length) const;
public:
	Shader();
	virtual ~Shader();

	static core::String validPreprocessorName(const core::String& name);

	static int glslVersion;

	virtual void shutdown();

	bool load(const core::String& name, const core::String& buffer, ShaderType shaderType);

	core::String getSource(ShaderType shaderType, const core::String& buffer, bool finalize = true, core::List<core::String>* includedFiles = nullptr) const;

	/**
	 * If the shaders were loaded manually via @c ::load, then you have to initialize the shader manually, too
	 */
	bool init();

	bool isInitialized() const;

	/**
	 * @brief The dirty state can be used to determine whether you have to set some
	 * uniforms again because the shader was reinitialized. This must be used manually
	 * if you set e.g. uniforms only once after init
	 * @sa isDirty()
	 */
	void markClean();
	/**
	 * @sa markClean()
	 */
	void markDirty();
	/**
	 * @sa markClean()
	 */
	bool isDirty() const;

	/**
	 * @brief Make sure to configure feedback transform varying before you link the shader
	 * @see setupTransformFeedback()
	 */
	virtual bool setup() {
		return false;
	}

	/**
	 * @note Must be called before calling @c setup()
	 * @see setup()
	 */
	void setupTransformFeedback(const core::List<core::String>& transformVaryings, TransformFeedbackCaptureMode mode);

	void recordUsedUniforms(bool state);

	void clearUsedUniforms();

	bool loadFromFile(const core::String& filename, ShaderType shaderType);

	/**
	 * @brief Loads a vertex and fragment shader for the given base filename.
	 *
	 * The filename is hand over to your @c Context implementation with the appropriate filename postfixes
	 *
	 * @see VERTEX_POSTFIX
	 * @see FRAGMENT_POSTFIX
	 */
	bool loadProgram(const core::String& filename);
	bool reload();

	/**
	 * @brief Returns the raw shader handle
	 */
	Id getShader(ShaderType shaderType) const;

	/**
	 * @brief Ticks the shader
	 */
	virtual void update(uint32_t deltaTime);

	/**
	 * @brief Bind the shader program
	 *
	 * @return @c true if is is useable now, @c false if not
	 */
	virtual bool activate() const;

	virtual bool deactivate() const;

	bool isActive() const;

	/**
	 * @brief Run the compute shader.
	 * @return @c false if this is no compute shader, or the execution failed.
	 */
	bool run(const glm::uvec3& workGroups, bool wait = false);

	bool transformFeedback() const;

	void checkAttribute(const core::String& attribute);
	void checkUniform(const core::String& uniform);
	void checkAttributes(std::initializer_list<core::String> attributes);
	void checkUniforms(std::initializer_list<core::String> uniforms);

	/**
	 * @brief Adds a new define in the form '#define value' to the shader source code
	 */
	void addDefine(const core::String& name, const core::String& value);

	void setUniformArraySize(const core::String& name, int size);
	void setAttributeComponents(int location, int size);
	int getAttributeComponents(int location) const;
	int getAttributeComponents(const core::String& name) const;

	/**
	 * @return -1 if uniform wasn't found, or no size is known. If the uniform is known, but
	 * it is no array, this will return 0
	 */
	int getUniformArraySize(const core::String& name) const;

	int checkAttributeLocation(const core::String& name) const;
	int getAttributeLocation(const core::String& name) const;
	bool setAttributeLocation(const core::String& name, int location);

	int getUniformLocation(const core::String& name) const;

	void setUniformui(const core::String& name, unsigned int value) const;
	void setUniform(const core::String& name, TextureUnit value) const;
	void setUniform(int location, TextureUnit value) const;
	void setUniformfv(int location, const float* values, int length, int components) const;
	void setUniformi(const core::String& name, int value) const;
	void setUniformi(const core::String& name, int value1, int value2) const;
	void setUniformi(const core::String& name, int value1, int value2, int value3) const;
	void setUniformi(const core::String& name, int value1, int value2, int value3, int value4) const;
	void setUniform1iv(const core::String& name, const int* values, int length) const;
	void setUniform2iv(const core::String& name, const int* values, int length) const;
	void setUniform3iv(const core::String& name, const int* values, int length) const;
	void setUniformf(const core::String& name, float value) const;
	void setUniformf(const core::String& name, float value1, float value2) const;
	void setUniformf(const core::String& name, float value1, float value2, float value3) const;
	void setUniformf(const core::String& name, float value1, float value2, float value3, float value4) const;
	void setUniformfv(const core::String& name, const float* values, int length, int components) const;
	void setUniform1fv(const core::String& name, const float* values, int length) const;
	void setUniform2fv(const core::String& name, const float* values, int length) const;
	void setUniform3fv(const core::String& name, const float* values, int length) const;
	void setUniformVec2(const core::String& name, const glm::vec2& value) const;
	void setUniformVec2(int location, const glm::vec2& value) const;
	void setUniformVec2v(const core::String& name, const glm::vec2* value, int length) const;
	void setUniformVec3(const core::String& name, const glm::vec3& value) const;
	void setUniformVec3v(const core::String& name, const glm::vec3* value, int length) const;
	void setUniform4fv(const core::String& name, const float* values, int length) const;
	void setUniformVec4(const core::String& name, const glm::vec4& value) const;
	void setUniformVec4(int location, const glm::vec4& value) const;
	void setUniformVec4v(const core::String& name, const glm::vec4* value, int length) const;
	void setUniformMatrix(const core::String& name, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(const core::String& name, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformMatrixv(const core::String& name, const glm::mat4* matrixes, int amount, bool transpose = false) const;
	void setUniformMatrixv(const core::String& name, const glm::mat3* matrixes, int amount, bool transpose = false) const;
	void setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose = false) const;
	void setUniformf(const core::String& name, const glm::vec2& values) const;
	void setUniformf(int location, const glm::vec2& values) const;
	void setUniformf(const core::String& name, const glm::vec3& values) const;
	void setUniformf(int location, const glm::vec3& values) const;
	void setUniformf(const core::String& name, const glm::vec4& values) const;
	void setUniformf(int location, const glm::vec4& values) const;
	void setVertexAttribute(const core::String& name, int size, DataType type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttributeInt(const core::String& name, int size, DataType type, int stride, const void* buffer) const;
	void disableVertexAttribute(const core::String& name) const;
	int enableVertexAttributeArray(const core::String& name) const;
	bool hasAttribute(const core::String& name) const;
	bool hasUniform(const core::String& name) const;
	bool isUniformBlock(const core::String& name) const;

	// particular renderer api must implement this

	bool setUniformBuffer(const core::String& name, const UniformBuffer& buffer);
	void setUniformui(int location, unsigned int value) const;
	void setUniformi(int location, int value) const;
	void setUniformi(int location, int value1, int value2) const;
	void setUniformi(int location, int value1, int value2, int value3) const;
	void setUniformi(int location, int value1, int value2, int value3, int value4) const;
	/**
	 * @param[in] amount The amount of int values
	 */
	void setUniform1iv(int location, const int* values, int amount) const;
	/**
	 * @param[in] amount The amount of int values
	 */
	void setUniform2iv(int location, const int* values, int amount) const;
	/**
	 * @param[in] amount The amount of int values
	 */
	void setUniform3iv(int location, const int* values, int amount) const;
	void setUniformIVec2v(int location, const glm::ivec2* value, int amount) const;
	void setUniformIVec3v(int location, const glm::ivec3* value, int amount) const;
	void setUniformIVec4v(int location, const glm::ivec4* value, int amount) const;
	void setUniformf(int location, float value) const;
	void setUniformf(int location, float value1, float value2) const;
	void setUniformf(int location, float value1, float value2, float value3) const;
	void setUniformf(int location, float value1, float value2, float value3, float value4) const;
	/**
	 * @param[in] amount The amount of float values
	 */
	void setUniform1fv(int location, const float* values, int amount) const;
	/**
	 * @param[in] amount The amount of float values
	 */
	void setUniform2fv(int location, const float* values, int amount) const;
	/**
	 * @param[in] amount The amount of the matrix instances
	 */
	void setUniformVec2v(int location, const glm::vec2* value, int amount) const;
	void setUniformVec3(int location, const glm::vec3& value) const;
	/**
	 * @param[in] amount The amount of float values
	 */
	void setUniform4fv(int location, const float* values, int amount) const;
	/**
	 * @param[in] amount The amount of float values
	 */
	void setUniform3fv(int location, const float* values, int amount) const;
	/**
	 * @param[in] amount The amount of the vector instances
	 */
	void setUniformVec3v(int location, const glm::vec3* value, int amount) const;
	/**
	 * @param[in] amount The amount of the vector instances
	 */
	void setUniformVec4v(int location, const glm::vec4* value, int amount) const;
	/**
	 * @param[in] amount The amount of the matrix instances
	 */
	void setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose = false) const;
	void setVertexAttribute(int location, int size, DataType type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttributeInt(int location, int size, DataType type, int stride, const void* buffer) const;
	void disableVertexAttribute(int location) const;
	bool enableVertexAttributeArray(int location) const;
	bool setDivisor(int location, uint32_t divisor) const;
};

inline bool Shader::isDirty() const {
	return _dirty;
}

inline bool Shader::isInitialized() const {
	return _initialized;
}

inline void Shader::addUsedUniform(int location) const {
	_usedUniforms.put(location, true);
}

inline void Shader::recordUsedUniforms(bool state) {
	_recordUsedUniforms = state;
}

inline void Shader::clearUsedUniforms() {
	_usedUniforms.clear();
}

inline void Shader::setUniformi(const core::String& name, int value) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value);
}

inline void Shader::setUniform(const core::String& name, TextureUnit value) const {
	const int location = getUniformLocation(name);
	setUniform(location, value);
}

inline void Shader::setUniform(int location, TextureUnit value) const {
	setUniformi(location, core::enumVal(value));
}

inline void Shader::setUniformui(const core::String& name, unsigned int value) const {
	const int location = getUniformLocation(name);
	setUniformui(location, value);
}

inline void Shader::setUniformi(const core::String& name, int value1, int value2) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2);
}

inline void Shader::setUniformi(const core::String& name, int value1, int value2, int value3) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2, value3);
}

inline void Shader::setUniformi(const core::String& name, int value1, int value2, int value3, int value4) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2, value3, value4);
}

inline void Shader::setUniform1iv(const core::String& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform1iv(location, values, length);
}

inline void Shader::setUniform2iv(const core::String& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform2iv(location, values, length);
}

inline void Shader::setUniform3iv(const core::String& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform3iv(location, values, length);
}

inline void Shader::setUniformf(const core::String& name, float value) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value);
}

inline void Shader::setUniformf(const core::String& name, float value1, float value2) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2);
}

inline void Shader::setUniformf(const core::String& name, float value1, float value2, float value3) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2, value3);
}

inline void Shader::setUniformf(const core::String& name, float value1, float value2, float value3, float value4) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2, value3, value4);
}

inline void Shader::setUniform1fv(const core::String& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform1fv(location, values, length);
}

inline void Shader::setUniform2fv(const core::String& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform2fv(location, values, length);
}

inline void Shader::setUniform3fv(const core::String& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform3fv(location, values, length);
}

inline void Shader::setUniformVec2(const core::String& name, const glm::vec2& value) const {
	const int location = getUniformLocation(name);
	setUniformVec2(location, value);
}

inline void Shader::setUniformVec2v(const core::String& name, const glm::vec2* value, int length) const {
	const int location = getUniformLocation(name);
	setUniformVec2v(location, value, length);
}

inline void Shader::setUniformVec3(const core::String& name, const glm::vec3& value) const {
	const int location = getUniformLocation(name);
	setUniformVec3(location, value);
}

inline void Shader::setUniformVec3v(const core::String& name, const glm::vec3* value, int length) const {
	const int location = getUniformLocation(name);
	setUniformVec3v(location, value, length);
}

inline void Shader::setUniformfv(const core::String& name, const float* values, int length, int components) const {
	const int location = getUniformLocation(name);
	setUniformfv(location, values, length, components);
}

inline void Shader::setUniformfv(int location, const float* values, int length, int components) const {
	if (components == 1) {
		setUniform1fv(location, values, length);
	} else if (components == 2) {
		setUniform2fv(location, values, length);
	} else if (components == 3) {
		setUniform3fv(location, values, length);
	} else {
		setUniform4fv(location, values, length);
	}
}

inline void Shader::setUniform4fv(const core::String& name, const float* values, int length) const {
	int location = getUniformLocation(name);
	setUniform4fv(location, values, length);
}

inline void Shader::setUniformVec4(const core::String& name, const glm::vec4& value) const {
	const int location = getUniformLocation(name);
	setUniformVec4(location, value);
}

inline void Shader::setUniformVec4v(const core::String& name, const glm::vec4* value, int length) const {
	const int location = getUniformLocation(name);
	setUniformVec4v(location, value, length);
}

inline void Shader::setUniformMatrix(const core::String& name, const glm::mat4& matrix, bool transpose) const {
	setUniformMatrixv(name, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat4& matrix, bool transpose) const {
	setUniformMatrixv(location, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(const core::String& name, const glm::mat3& matrix, bool transpose) const {
	setUniformMatrixv(name, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat3& matrix, bool transpose) const {
	setUniformMatrixv(location, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrixv(const core::String& name, const glm::mat4* matrixes, int amount, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrixv(location, matrixes, amount, transpose);
}

inline void Shader::setUniformMatrixv(const core::String& name, const glm::mat3* matrixes, int amount, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrixv(location, matrixes, amount, transpose);
}

class ScopedShader {
private:
	const Shader& _shader;
	const Id _oldShader;
	bool _alreadyActive;
public:
	ScopedShader(const Shader& shader);

	~ScopedShader();};

#define shaderSetUniformIf(shader, func, var, ...) if (shader.hasUniform(var)) { shader.func(var, __VA_ARGS__); }

typedef std::shared_ptr<Shader> ShaderPtr;

}
