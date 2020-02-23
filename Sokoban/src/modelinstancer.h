#pragma once

#include "model.h"

struct InstanceData;
enum class BlockTexture;

class ModelInstancer {
public:
	ModelInstancer(std::string path);
	virtual ~ModelInstancer();
	virtual void push_instance(glm::vec3 pos, glm::vec3 scale, BlockTexture tex_id, int color);
	virtual void push_instance(glm::vec3 pos, glm::vec3 scale, BlockTexture tex_id, glm::vec4 color);
	virtual void draw() = 0;
	void set_instance_attributes(int VAO);
protected:
	Model model_;
	std::vector<InstanceData> instances_;
	unsigned int buffer_;
	void setup_buffer();
	void fill_buffer();
};

class DynamicInstancer : public ModelInstancer {
public:
	DynamicInstancer(std::string path);
	~DynamicInstancer();
	void draw();
};

class StaticInstancer : public ModelInstancer {
public:
	StaticInstancer(std::string path);
	~StaticInstancer();
	void draw();
};