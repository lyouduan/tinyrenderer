#ifndef RTWEEKENED_H
#define RTWEEKENED_H

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

//Using 
using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// Constants
const double infinity = std::numeric_limits<float>::infinity();
const float pi = 3.141592653589793;

// Utility Functions
inline float degrees_to_radians(float degrees) {
	return degrees * pi / 180.f;
}

inline float random_float() {
	return rand() / (RAND_MAX + 1.0);
}

inline float random_float(float min, float max) {
	return min + (max - min) * random_float();
}

// Common Headers
#include "ray.h"
#include "vec3.h"
#include "interval.h"

#endif // !RTWEEKENED_H
