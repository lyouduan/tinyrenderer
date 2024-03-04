#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"

#include "color.h"
#include "hittable.h"
#include "material.h"

#include "time.h"

#include<iostream>
#include<fstream>

class camera {
public:
	float aspect_ratio = 1.f;
	int image_width = 100;
	int samples_per_pixel = 10;
	int max_depth = 10;
	
	float vfov = 90; // Vertical view angle(field of view)
	point3 lookfrom = point3(0, 0, -1); // Point camera is looking from
	point3 lookat = point3(0, 0, 0); // Point camera is looking at
	vec3 vup = vec3(0, 1, 0); // Camera-relative "up" direction

	float defocus_angle = 0; // Variation angle of rays through each pixel
	float focus_dist = 0; // Distance from camera lookfrom point to plane of perfect focus

	void render(const hittable &world) {

		std::clog << "=========Initialize...=========" << std::endl;

		initialize();

		timer time;
		// Image
		std::string filename = "image.ppm";
		std::ofstream file(filename);

		if (file.is_open()) {

			std::clog << "=========Rendering...=========" << std::endl;

			file << "P3\n" << image_width << " " << image_height << "\n255\n";
			for (int j = 0; j < image_height; j++) {
				for (int i = 0; i < image_width; i++) {
					color pixel_color(0, 0, 0);

					//multiple samples for one pixel
					for (int sample = 0; sample < samples_per_pixel; sample++) {
						ray r = get_ray(i, j);
						pixel_color += ray_color(r, world, max_depth);
					}
					// average color 
					pixel_color /= samples_per_pixel;
					write_color(file, pixel_color);
				}
			}
			

		}
		else {
			std::cout << "File open error" << std::endl;
		}

		float duration = time.duration();
		file.close();
		std::clog << "\nCompleted the output, ran for " << duration << " seconds" << std::endl;
	}

private:
	int image_height;
	point3 center;
	point3 pixel00_loc;
	vec3 pixel_delta_u;
	vec3 pixel_delta_v;
	vec3 u, v, w;
	vec3 defocus_disk_u;
	vec3 defocus_disk_v;

	void initialize() {
		image_height = static_cast<int>(image_width / aspect_ratio);
		image_height = image_height < 1 ? 1 : image_height;

		center = lookfrom;

		// Determine viewport dimensions
		//float focal_length = (lookfrom - lookat).length();
		auto theta = degrees_to_radians(vfov);
		auto h = tan(theta / 2);
		//float viewport_height = 2.f * h * focal_length;
		float viewport_height = 2.f * h * focus_dist;
		float viewport_width = viewport_height * (static_cast<float>(image_width) / image_height);

		// Calculate the uvw unit basis vectors for the camera coordinate frame
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = unit_vector(cross(w, u));

		vec3 viewport_u = viewport_width * u; // Vector across viewport horizontal edge
		vec3 viewport_v = viewport_height * -v; // Vector down viewport vertical edge

		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		//auto viewport_upper_left = center - focal_length * w - viewport_u / 2 - viewport_v / 2;
		auto viewport_upper_left = center - focus_dist * w - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	} 

	ray get_ray(int i, int j) const {
		auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
		
		// multiple sample for each pixel to antialiasing
		auto pixel_sample = pixel_center + pixel_sample_square();

		// without defocus blur, all scene rays originate from the (pinhole) camera center
		//auto ray_origin = center;
		
		// defocus blur: all rays originate at the (thin lens)disk center
		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();

		auto ray_direction = pixel_sample - ray_origin;

		return ray(ray_origin, ray_direction);
	}

	vec3 pixel_sample_square() const {

		auto px = -0.5 + random_float();
		auto py = -0.5 + random_float();

		return (px * pixel_delta_u) + (py * pixel_delta_v);
	}

	point3 defocus_disk_sample() const {
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	color ray_color(const ray& r, const hittable& world, int depth) const {
		hit_record rec;

		// limit the ray bounce
		if (depth < 0) return color(0.f, 0.f, 0.f);

		if (world.hit(r, interval(0.001f, infinity), rec)) {
			ray scattered;
			color attenuation;

			// recursive
			if (rec.m->scatter(r, rec, attenuation, scattered)) {
				return attenuation * ray_color(scattered, world, depth - 1);
			}
			return color(0, 0, 0);
		}

		vec3 unit_direction = unit_vector(r.direction());
		auto a = 0.5 * (unit_direction.y() + 1.0);
		return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
	}
};
#endif // !CAMERA_H
