#include "rtweekend.h"
#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

int main() {
	
	//World 
	hittable_list world;
	/*
	auto material_ground = make_shared<lambertian>(color(.8f, .8f, .0f));
	auto material_center = make_shared<lambertian>(color(.1f, .2f, .5f));
	auto material_left = make_shared<dielectric>(1.5);
	auto material_right = make_shared<metal>(color(.8f, .6f, .2f), 0.f);

	world.add(make_shared<sphere>(point3(0.f, -100.5f, -1.f),  100, material_ground));
	world.add(make_shared<sphere>(point3(0.f,     0.f, -1.f),  .5f, material_center));
	world.add(make_shared<sphere>(point3(-1.f,    0.f, -1.f),  .5f, material_left));
	world.add(make_shared<sphere>(point3(-1.f,    0.f, -1.f), -.4f, material_left));
	world.add(make_shared<sphere>(point3(1.f,     0.f, -1.f),  .5f, material_right));
	*/

	auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_float();
			point3 center(a + 0.9 * random_float(), 0.2, b + 0.9 * random_float());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.75) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else if (choose_mat < 0.90) {
					// metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_float(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(2.5f);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

	//Camera
	camera cam;
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 1200;
	cam.samples_per_pixel = 500;
	cam.max_depth = 50;

	cam.vfov = 20;
	cam.lookfrom = point3(13, 2, 3);
	cam.lookat = point3(0, 0, 0);
	cam.vup = point3(0, 1, 0);

	// change the radius of aperture 
	cam.defocus_angle = .1f;
	// focus_distance
	cam.focus_dist = 10.f;

	// Render
	cam.render(world);
	
}