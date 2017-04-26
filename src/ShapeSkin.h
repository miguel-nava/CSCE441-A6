#pragma once
#ifndef _SHAPESKIN_H_
#define _SHAPESKIN_H_

#include <memory>

class MatrixStack;
class Program;

class ShapeSkin
{
public:
	ShapeSkin();
	virtual ~ShapeSkin();
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename);

	void loadSkeleton(const std::string &skeletonFile);
	std::vector<glm::mat4> getFrameTransforms(int frame) { return frames[frame%frames.size()]; };
	std::vector<float> getBoneWeights(int bone) { return weiBuf[bone%weiBuf.size()]; };

	void setProgram(std::shared_ptr<Program> p) { prog = p; };
	void init();
	void draw() const;
	void draw_animation(int k);
	
	
private:
	std::shared_ptr<Program> prog;
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> original_posBuf;
	std::vector<float> norBuf;
	std::vector<float> original_norBuf;
	std::vector<float> texBuf;


	std::vector<std::vector<glm::mat4> > frames;
	std::vector<glm::mat4> bind;
	std::vector<std::vector<float> > weiBuf;

	std::vector<std::vector<float> > weights;
	std::vector<std::vector<int> > bones;

	int influences;

	unsigned elemBufID;
	unsigned posBufID;
	unsigned norBufID;
	unsigned texBufID;
	unsigned weiBufID;

};

#endif
