#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>

// Bresenham's Line Algorithm

void line(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
	bool steep = false;

	// 斜率非0~1
	if (std::abs(v1.x - v0.x) < std::abs(v1.y - v0.y)) {
		std::swap(v0.x, v0.y);
		std::swap(v1.x, v1.y);
		steep = true;
	}
	// 正向增长
	if (v0.x > v1.x) {
		std::swap(v0.x, v1.x);
		std::swap(v0.y, v1.y);
	}

	int dx = v1.x - v0.x;
	int dy = std::abs(v1.y - v0.y);
	int delta = dy * 2;
	int error = 0;
	int y = v0.y;
	for (int x = v0.x; x <= v1.x; x++) {
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		// x加1，y对应的增长率
		error += delta;
		if (error > 1) {
			y += (v1.y > v0.y ? 1 : -1);
			error -= dx * 2;
		}
	}
}
// Old-school method: Line sweeping
void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);
	int total = t2.y - t0.y;

	int half1 = t1.y - t0.y + 1;
	for (int y = t0.y; y <= t1.y; y++) {

		float alpha = (y - t0.y) / (float)half1;
		float beta = (y - t0.y) / (float)total;
		Vec2i A = t0 + (t2 - t0) * beta;
		Vec2i B = t0 + (t1 - t0) * alpha;
		// let A.x <= B.x
		if (A.x > B.x) std::swap(A, B);
		for (int x = A.x; x <= B.x; x++) {
			image.set(x, y, color);
		}
	}
	int half2 = t2.y - t1.y + 1;//avoid division by zero
	for (int y = t2.y; y >= t1.y; y--) {

		float alpha = (y - t2.y) / (float)half2;
		float beta = (y - t2.y) / (float)total;
		Vec2i A = t2 + (t2 - t0) * beta;
		Vec2i B = t2 + (t2 - t1) * alpha;
		// let A.x <= B.x
		if (A.x > B.x) std::swap(A, B);
		for (int x = A.x; x <= B.x; x++) {
			image.set(x, y, color);
		}
	}
}
