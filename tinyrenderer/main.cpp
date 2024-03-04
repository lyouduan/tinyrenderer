#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

#include <algorithm>
#include <vector> 
#include <iostream>
#define M_PI 3.141526
Model *model = NULL;
const int width = 800;
const int height = 800;
const float depth = 2000.f;
float* shadowbuffer = NULL;
Vec3f light_dir(-1, 1, 1);
Vec3f       eye(1, 1, 4);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);

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
	mat<4, 4, float> uniform_Mshadow;
	// vertex shader
	// 1 to transform the coordinates of the vertexs
	// 2 prepare the data for the fragment shader
	virtual Vec4f vertex(Vec3i iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface.y));
		// transform the normal to world space
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
		Vec4f sb_p = uniform_Mshadow * embed<4>(varying_tri * bar);
		sb_p = sb_p / sb_p[3];

		int idx = int(sb_p[0]) + int(sb_p[1]) * width;
		// compare the z-values in shadowbuffer screen space
		float shadow = .3 + .7 * (shadowbuffer[idx] < sb_p[2] + 43.34);

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
		//mat<3, 3, float> B;
		//B.set_col(0, i.normalize());
		//B.set_col(1, j.normalize());
		//B.set_col(2, bn);

		Vec3f E1 = ndc_tri.col(1) - ndc_tri.col(0);
		Vec3f E2 = ndc_tri.col(2) - ndc_tri.col(0);
		float delta_u1 = varying_uv[0][1] - varying_uv[0][0];
		float delta_v1 = varying_uv[1][1] - varying_uv[1][0];

		float delta_u2 = varying_uv[0][2] - varying_uv[0][0];
		float delta_v2 = varying_uv[1][2] - varying_uv[1][0];

		float div = 1.f / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
		Vec3f T = E2 * delta_v1 - E1 * delta_v2;
		T = T * div;
		Vec3f B = E1 * delta_u2 - E2 * delta_u1;
		B = B * div;
		
		T = (T - bn * (T*bn)).normalize();
		B = (B - bn * (B * bn) - T * (B * T)).normalize();
		
		mat<3, 3, float> TBN;
		TBN.set_col(0, T.normalize());
		TBN.set_col(1, B.normalize());
		TBN.set_col(2, bn);
		Vec3f n = (TBN * model->normal(uv)).normalize();
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f e = proj<3>(uniform_M * embed<4>(eye)).normalize();
		Vec3f h = (l + e).normalize();
		// Blin-Phong shading model
		float spec = pow(std::max(h * n, 0.f), model->specular(uv));

		//Vec3f r = (n * (n * l * 2.f) - l).normalize();
		//float spec = pow(std::max(r * n, 0.f), model->specular(uv));

		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		color = c;
		for (int i = 0; i < 3; i++) {
			// ambient + diffuse + specular
			color[i] = std::min<float>(5 + c[i] * shadow * (diff + 0.6 * spec), 255);
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
		// transform the tangent space to world space by B matrix
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
		// transform the normal to world space
		Vec4f gl_Vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface.x)); // read the vertex from .obj file
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex/ gl_Vertex[3]));
		return gl_Vertex; // transform it to screen coordinates
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
float max_elevation_agle(float* zbuffer, Vec2f p, Vec2f dir) {
	float maxangle = 0;
	for (float t = 0.f; t < 1000.f; t += 1.f) {
		Vec2f cur = p + dir * t;
		if (cur.x >= width || cur.y >= height || cur.x < 0 || cur.y < 0) return maxangle;
		float d = (p - cur).norm();
		if (d < 1.f) continue;
		float elevation = zbuffer[int(cur.x) + int(cur.y) * width] - zbuffer[int(p.x) + int(p.y) * width];
		maxangle = std::max(maxangle, atanf(elevation / d));
	}
	return maxangle;
}

int main(int argc, char** argv) {
	model = new Model("obj/floor/floor.obj");

	// The texture needs to be vertically flipped:
	// texture.flip_vertically();
	float* zbuffer = new float[width * height];
	shadowbuffer = new float[width * height];

	light_dir.normalize();

	for (int i = width * height; i--; shadowbuffer[i] = zbuffer[i] = -std::numeric_limits<float>::max());

	{ // rendering the shadow buffer
		TGAImage depth(width, height, TGAImage::RGB);
		// from light_dir to object
		lookat(light_dir, center, up);
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		projection(0);

		DepthShader depthshader;
		Vec4f screen_coords[3];
		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<Vec3i> face = model->face(i);
			for (int j = 0; j < 3; j++) {
				screen_coords[j] = depthshader.vertex(face[j], j);
			}
			triangle(screen_coords, depthshader, depth, shadowbuffer);
		}
		depth.flip_vertically(); // to place the origin in the bottom left corner of the image
		depth.write_tga_file("depth.tga");
	}
	// record the relationship of the transformation between the light and obj
	Matrix M = Viewport * Projection * ModelView;

	{
		projection(-1.f / (eye - center).norm());
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		lookat(eye, center, up);

		//light_dir = proj<3>((Projection * ModelView * embed<4>(light_dir, 0.f))).normalize();

		TGAImage image(width, height, TGAImage::RGB);

		//PhongShader shader;
		Shader shader(ModelView, ModelView.invert_transpose(), M * (Viewport * Projection * ModelView).invert());
		//shader.uniform_M = Projection * ModelView;
		//shader.uniform_MIT = (Projection * ModelView).invert_transpose();
		//shader.uniform_Mshadow = M * (Projection * ModelView).invert();
		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<Vec3i> face = model->face(i);
			Vec4f screen_coords[3];
			for (int j = 0; j < 3; j++) {
				screen_coords[j] = shader.vertex(face[j], j);
				//Vec3f v = model->vert(face[j].x);
				//Vec3f uv = model->uv(face[j].y);
				//intensity[j] = model->normal(face[j].z) * light_dir;
			}
			triangle(screen_coords, shader, image, zbuffer);
		}
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("output.tga");

	}

	/*
	TGAImage frame(width, height, TGAImage::RGB);
	projection(-1.f / (eye - center).norm());
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(eye, center, up);
	ZShader shader;

	for (int i = 0; i < model->nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			shader.vertex(model->face(i)[j], j);
		}
		triangle(shader.varying_tri, shader, frame, zbuffer);
	}

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			if (zbuffer[x + y * width] < -1e4) continue;
			float total = 0;
			for (float a = 0; a < M_PI * 2 - 1e-4; a += M_PI / 4) {
				total += M_PI / 2 - max_elevation_agle(zbuffer, Vec2f(x, y), Vec2f(cos(a), sin(a)));
			}
			total /= (M_PI / 2) * 8;
			total = std::pow(total, 100.f);
			frame.set(x, y, TGAColor(total * 255, total * 255, total * 255));
		}
	}
	frame.flip_vertically();
	frame.write_tga_file("framebuffer.tga");
	*/

	delete model;
	delete[] zbuffer;
	delete[] shadowbuffer;
	
	return 0;
}