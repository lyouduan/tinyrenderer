#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void projection(float coeff) {
	Projection = Matrix::identity();
	Projection[3][2] = coeff;
}
// 视口变换 
// transforming coordinates from a world or object space 
// into normalized device coordinates (NDC) or screen space
void viewport(int x, int y, int w, int h) {
	Viewport = Matrix::identity();
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = 255.f / 2.f;
	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = 255.f / 2.f;
}
// 视点矩阵：用于将世界空间中的矢量转到相机空间
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
	// defining the camera coordinate system
	Vec3f z = (eye - center).normalize();
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z, x).normalize();

	ModelView = Matrix::identity();
	for (int i = 0; i < 3; i++) {
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i];
	}
}

Vec3f barycentric(mat<3, 2, float> pts, Vec2i P) {
	Vec3f s0 = Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]);
	Vec3f s1 = Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]);
	Vec3f u = cross(s0, s1);
	/*
	* The cross product is always orthogonal to both vectors,
	* and has magnitude zero when the vectors are parallel
	* and maximum magnitude ‖a‖‖b‖ when they are orthogonal.
	* so if u.z <1 that means they are not orthogonal.
	*
	*/
	if (std::abs(u.z) < 1e-2) return Vec3f(-1, 1, 1);
	return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}


void triangle(Vec4f* pts, IShader& shader, TGAImage& image, float* zbuffer) {
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	mat<3, 2, float> pts2;
	for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::min(bboxmin[j], pts2[i][j]);
			bboxmax[j] = std::max(bboxmax[j], pts2[i][j]);
		}
	}
	Vec2i P;
	TGAColor color;

	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f c = barycentric(pts2, P);
			float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
			float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
			int frag_depth = z / w;
			if (c.x < 0 || c.y < 0 || c.z<0 || zbuffer[P.x + P.y * image.get_width()]>frag_depth) continue;
			bool discard = shader.fragment(c, color);
			if (!discard) {
				zbuffer[P.x + P.y * image.get_width()] = frag_depth;
				image.set(P.x, P.y, color);
			}
		}
	}
}
void triangle(mat<4, 3, float >& clipc, IShader& shader, TGAImage& image, float* zbuffer) {
	mat<3, 4, float> pts = (Viewport * clipc).transpose(); // transposed to ease access to each of the points
	mat<3, 2, float> pts2;
	for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);
	
	Vec2f bbox_min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bbox_max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++){
			bbox_min[j] = std::min(bbox_min[j], pts2[i][j]);
			bbox_max[j] = std::max(bbox_max[j], pts2[i][j]);
		}
	}
	Vec2i P;
	TGAColor color;
	for (P.x = bbox_min.x; P.x <= bbox_max.x; P.x++) {
		for (P.y = bbox_min.y; P.y <= bbox_max.y; P.y++) {
			Vec3f bc = barycentric(pts2, P);
			
			Vec3f bc_clip = Vec3f(bc.x / pts[0][3], bc.y / pts[1][3], bc.z / pts[2][3]);
			
			// perspective correct interpolate
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);

			// z value :perspective correct interpolate
			float frag_depth = clipc[2]*bc_clip;

			if (bc.x < 0 || bc.y < 0 || bc.z < 0 || zbuffer[P.x + P.y*image.get_width()] > frag_depth) continue;
			
			// 
			//bool discard = shader.fragment(Vec3f(P.x, P.y, frag_depth),bc_clip, color);
			bool discard = shader.fragment(bc_clip, color);
			// 或者The texture needs to be vertically flipped
			//TGAColor color = texture.get((tx.x) * texture.get_width(), ( tx.y) * texture.get_height());
			if (!discard) {
				zbuffer[P.x + P.y * image.get_width()] = frag_depth;
				image.set(P.x, P.y, color);
			}
		}
	}

}
