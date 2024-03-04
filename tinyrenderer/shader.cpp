#include "our_gl.h"
#include "model.h"
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

#include <algorithm>
#include <vector> 
#include <iostream>
struct GouraudShader : public IShader {
	Vec3f varying_intensity; // written by vertex shader, read by fragment shader
	mat<2, 3, float> varying_uv;
	mat<4, 3, float> varying_tri;
	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface.z) * light_dir); // get diffuse lighting intensity
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, gl_Vertex);
		return gl_Vertex; // transform it to screen coordinates
	}

	// fragment shader
	// 1 to determine the color of the current pixel
	// 2 discard the current pixel by return true
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
		Vec2f uv = varying_uv * bar;
		color = model->diffuse(uv) * intensity;
		return false;                              // no, we do not discard this pixel
	}
};

struct PhongShader : public IShader {
	mat<4, 3, float> varying_tri;
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_nrm;
	mat<3, 3, float> ndc_tri;
	mat<4, 4, float> uniform_M;   //  Projection*ModelView
	mat<4, 4, float> uniform_MIT; // (Projection*ModelView).invert_transpose()
	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		varying_nrm.set_col(nthvert, proj<3>(uniform_MIT * embed<4>(model->normal(iface.z), 0.f)));
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, gl_Vertex);
		ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return gl_Vertex; // transform it to screen coordinates
	}

	// fragment shader
	// 1 to determine the color of the current pixel
	// 2 discard the current pixel by return true
	virtual bool fragment(Vec3f bar, TGAColor& color) {

		// texture coordinate: perspective correct interpolate
		Vec2f uv = varying_uv * bar;
		//Vec3f n = varying_nrm * bar;
		Vec3f bn = (varying_nrm * bar).normalize();
		mat<3, 3, float> A;
		A[0] = ndc_tri.col(1) - ndc_tri.col(0);
		A[1] = ndc_tri.col(2) - ndc_tri.col(0);
		A[2] = bn;
			
		mat<3, 3, float> AI = A.invert();

		Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
		Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

		mat<3, 3, float> B;
		B.set_col(0, i.normalize());
		B.set_col(1, j.normalize());
		B.set_col(2, bn);
		Vec3f n = (B * model->normal(uv)).normalize();

		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f e = proj<3>(uniform_M * embed<4>(eye)).normalize();
		Vec3f h = (l + e).normalize();
		float spec = pow(std::max(h * n, 0.f), model->specular(uv));

		//Vec3f r = (n * (n * l * 2.f) - l).normalize();
		//float spec = pow(std::max(r * n, 0.f), model->specular(uv));

		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		color = c;
		for (int i = 0; i < 3; i++) {
			// ambient + diffuse + specular
			color[i] = std::min<float>(5 + c[i] * (diff + 0.6 * spec), 255);
		}
		return false; // no, we do not discard this pixel
	}
};

struct normalShader : public IShader {
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_nrm;
	mat<4, 3, float> varying_tri;
	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		varying_nrm.set_col(nthvert, model->normal(iface.z));
		Vec4f gl_Vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, gl_Vertex);
		return gl_Vertex; // transform it to screen coordinates
	}

	// fragment shader
	// 1 to determine the color of the current pixel
	// 2 discard the current pixel by return true
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		// interpolate uv
		Vec2f uv = varying_uv * bar;
		// interpolate normal 
		Vec3f bn = (varying_nrm * bar).normalize();

		float diff = std::max(0.f, bn * light_dir);
		color = model->diffuse(uv) * diff;

		return false; // no, we do not discard this pixel
	}
};

// using the normal maps in the tangent space
// update the normal tangent texture
struct BumpShader : public IShader {
	mat<4, 3, float> varying_tri;
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_nrm;
	mat<3, 3, float> ndc_tri; // triangle in normalized device coordinates

	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface.z), 0.f)));
		// 将顶点从对象坐标转到世界坐标
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, gl_Vertex);
		// 四维齐次坐标，再除以 w 后，进入三维的归一化设备坐标(normalized device coordinates)
		ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return  gl_Vertex; // transform it to screen coordinates
	}

	// fragment shader
	// 1 to determine the color of the current pixel
	// 2 discard the current pixel by return true
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		//float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
		// interpolate uv
		Vec2f uv = varying_uv * bar;
		// interpolate normal 
		Vec3f bn = (varying_nrm * bar).normalize();

		mat< 3, 3, float> A;
		A[0] = ndc_tri.col(1) - ndc_tri.col(0);
		A[1] = ndc_tri.col(2) - ndc_tri.col(0);
		A[2] = bn;

		mat< 3, 3, float> AI = A.invert();
		Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
		Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);
		
		mat<3, 3, float> B;
		B.set_col(0, i.normalize());
		B.set_col(1, j.normalize());
		B.set_col(2, bn);
		// tangent space to world space
		Vec3f n = (B * model->normal(uv)).normalize();

		float diff = std::max(0.f, n * light_dir);
		color = model->diffuse(uv) * diff;

		return false; // no, we do not discard this pixel
	}
};

struct DepthShader : public IShader {
	mat<3, 3, float> varying_tri;

	DepthShader() : varying_tri() {}

	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		Vec4f gl_Vertex = embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;          // transform it to screen coordinates
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {
		Vec3f p = varying_tri * bar;
		color = TGAColor(255, 255, 255) * (p.z / depth);
		return false;
	}
};
struct Shader : public IShader {
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_tri;
	mat<4, 4, float> uniform_M;   //  ModelView
	mat<4, 4, float> uniform_MIT; // ModelView.invert_transpose():(A-1)T
	mat<4, 4, float> uniform_Mshadow; //M * Viewport * (Projection*ModelView).invert_transpose().invert()

	Shader(Matrix M, Matrix MIT, Matrix MS) :
		uniform_M(M), uniform_MIT(MIT), uniform_Mshadow(MS), varying_uv(), varying_tri() {}

	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		Vec4f gl_Vertex = Viewport * Projection* ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return gl_Vertex; // transform it to screen coordinates
	}

	// fragment shader
	// 1 to determine the color of the current pixel
	// 2 discard the current pixel by return true
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		// transform framebuffer screen space to shadowbuffer screen space
		Vec4f sb_p = uniform_Mshadow * embed<4>(varying_tri * bar);
		sb_p = sb_p / sb_p[3];

		int idx = int(sb_p[0]) + int(sb_p[1]) * width;
		// compare the z-values in shadowbuffer screen space
		float shadow = .3 + .7 * (shadowbuffer[idx] < sb_p[2] + 43.34);// magic coeff to avoid z-fighting
		Vec2f uv = varying_uv * bar;

		// caculate the light dirtion in world space
		// transform object space to world space
		// pay attention: normal vecotr multiply by invert transpoe matrix, not invert matrix
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
		// transform object space to world space
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f e = proj<3>(uniform_M * embed<4>(eye)).normalize();
		Vec3f h = (l + e).normalize();
		// Phong shading model
		Vec3f r = (n * (n * l * 2.f) - l).normalize();
		float spec = pow(std::max(r * n, 1.f), model->specular(uv));

		// Blin-Phong shading model
		// float spec = pow(std::max(h * n, 1.f), model->specular(uv));

		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		for (int i = 0; i < 3; i++) {
			// ambient + diffuse + specular
			color[i] = std::min<float>(5 + c[i] * shadow * (diff + 0.6 * spec), 255);
		}
		return false;                              // no, we do not discard this pixel
	}
};
/*
struct ZShader : public IShader {

	mat<4, 3, float> varying_tri;
	
	virtual Vec4f vertex(Vec3i iface, int nthvert)  {
		Vec4f glVertex = Projection * ModelView * embed<4>(model->vert(iface.x));
		varying_tri.set_col(nthvert, glVertex);
		return glVertex;
	}
	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor& color)  {
		color = TGAColor(0, 0, 0);
		return false;
	}

	
};
*/