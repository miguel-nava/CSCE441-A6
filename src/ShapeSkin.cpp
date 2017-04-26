#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"

using namespace std;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0),
	weiBufID(0),
	influences(0)
{
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		original_posBuf = attrib.vertices;
		norBuf = attrib.normals;
		original_norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	int nverts, nbones;
	ifstream in;
	in.open(filename);
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	string line;
	getline(in, line); // comment
	getline(in, line); // comment
	getline(in, line);
	stringstream ss0(line);
	ss0 >> nverts;
	ss0 >> nbones;
	assert(nverts == posBuf.size()/3);
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		// Parse line
		stringstream ss(line);
		
		float w;
		std::vector<float> ws;		// fst implementation
		// task 2 implementation
		std::vector<float> weight(16);	// makes vectors of size 16, filled with 0's 
		std::vector<int> bone(16);		// makes vector of size 16, filled with 0's 
		int index = 0;	// used to keep track of index for the vectors
		for (int i = 0; i < nbones; i++) {
			ss >> w;

			if (w != 0) 
			{
				weight[index] = w;
				bone[index] = i;
				index++;
			}
				ws.push_back(w);		// part of first implementation
		}
		weights.push_back(weight);		//snd
		bones.push_back(bone);			//snd

		weiBuf.push_back(ws);			// part of first implementation
	}
	in.close();
	cout << "Finished loading attachment. weiBuf.size() = " << weiBuf.size() << endl;
}

void ShapeSkin::loadSkeleton(const std::string &skeletonFile) {
	int nframes, nbones;
	
	ifstream in;
	in.open(skeletonFile);
	if (!in.good()) {
		cout << "Cannot read " << skeletonFile << endl;
		return;
	}
	string line;
	getline(in, line); // comment
	getline(in, line); // comment
	getline(in, line); // comment
	getline(in, line);
	stringstream ss0(line);
	ss0 >> nframes;
	ss0 >> nbones;
	// maybe assert something

	while (1) {
		getline(in, line);
		if (in.eof()) {
			break;
		}
		// Parse line
		stringstream ss(line);

		std::vector<glm::mat4> transformations;
		// there are 7 data points for every bone
		// each line contains transforms for all bones
		for (int i = 0; i < nbones*7; i += 7) {
			glm::mat4 E;
			glm::quat q;
			glm::vec3 p;
			ss >> q.x;
			ss >> q.y;
			ss >> q.z;
			ss >> q.w;
			ss >> p.x;
			ss >> p.y;
			ss >> p.z;
			E = glm::mat4_cast(q);
			E[3] = glm::vec4(p, 1.0f);
			transformations.push_back(E);
			if (frames.size() == 0) {
				bind.push_back(glm::inverse(E));
			}
		}
		frames.push_back(transformations);
	}
	std::cout << "Finished loading skeleton. frames.size() = " << frames.size() << std::endl;
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);

	// No texture info
	texBufID = 0;

	// Send weights array to the GPU
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	assert(glGetError() == GL_NO_ERROR);
}

void ShapeSkin::draw() const
{
	assert(prog);
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// need to enable the vertex array  for the weights

	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ShapeSkin::draw_animation(int k) {
	assert(prog);
	posBuf.clear();
	norBuf.clear();
	std::vector < glm::mat4 > M = getFrameTransforms(k);

	for (int i = 0; i < original_posBuf.size(); i+=3)	// go through all vertices
	{
		// first implementation
		//std::vector<float> w = getBoneWeights(i/3);

		//for second implementation
		std::vector<float> weight = weights[i/3];
		std::vector<int> bone = bones[i/3];
	
		glm::vec4 oPos(original_posBuf[i], original_posBuf[i + 1], original_posBuf[i + 2], 1);
		glm::vec4 oNor(original_norBuf[i], original_norBuf[i + 1], original_norBuf[i + 2], 1);
		//glm::vec4 p = w[0] * M[0] * (bind[0] * oPos);
		//glm::vec4 n = w[0] * M[0] * (bind[0] * oNor);
		
		glm::vec4 p(0,0,0,0);
		glm::vec4 n(0,0,0,0);

		// First implementation
		//for (int j = 0; j < 18; j++)	// go through all the weights
		//{
		//	p += w[j] * M[j] * bind[j] * oPos;
		//	n += w[j] * M[j] * bind[j] * oNor;
		//}

		// Second implementation
		for (int j = 0; j < M.size(); j++) {
			if (weight[j] != 0) {
				p += weight[j] * M[bone[j]] * bind[bone[j]] * oPos;
				n += weight[j] * M[bone[j]] * bind[bone[j]] * oNor;
			}
			else {
				break;
			}
		}
		
		posBuf.push_back(p.x); norBuf.push_back(n.x);
		posBuf.push_back(p.y); norBuf.push_back(n.y);
		posBuf.push_back(p.z); norBuf.push_back(n.z);
	}

	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);

	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	// need to enable the vertex array  for the weights

	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);

	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
