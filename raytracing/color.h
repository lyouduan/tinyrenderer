#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <fstream>

using color = vec3;

inline float liner_to_gamma(float linear_component) {
	return sqrt(linear_component);
}

void write_color(std::ofstream& file, color pixel_color) {

	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// apply the linear to gamma transform
	r = liner_to_gamma(r);
	g = liner_to_gamma(g);
	b = liner_to_gamma(b);
	
	//Write the translated [0, 255] value of each color component
	static const interval intensity(0.000f, 0.999f);
	file << static_cast<int>(255.999 * intensity.clamp(r)) << " "
		 << static_cast<int>(255.999 * intensity.clamp(g)) << " "
		 << static_cast<int>(255.999 * intensity.clamp(b)) << "\n";
	
}
#endif // !COLOR_H
