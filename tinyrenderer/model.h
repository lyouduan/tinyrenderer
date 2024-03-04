#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec2f> uv_;
	std::vector<Vec3f> normal_;
	std::vector<std::vector<Vec3i> > faces_;
	TGAImage diffusemap_;
	TGAImage normalmap_;
	TGAImage specularmap_;
	void load_texture(std::string filename, const char* suffix, TGAImage& img);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec2f uv(int i);
	Vec3f normal(int i);
	Vec3f normal(Vec2f uvf);
	TGAColor diffuse(Vec2f uvf);
	float specular(Vec2f uvf);
	
	std::vector<Vec3i> face(int idx);
};

#endif //__MODEL_H__