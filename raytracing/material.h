#ifndef MATERIAL_H
#define MATERIAL_H

#include "rtweekend.h"
#include "hittable_list.h"

class hit_record;

class material {
public:
	virtual ~material() = default;

	virtual bool scatter(
		const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {
public:
	lambertian(const color& a): albedo(a) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) 
	const override{
		auto scatter_direction = rec.normal + random_unit_vector();

		// catch degenerate the scatter_direction 
		if (scatter_direction.near_zero())
			scatter_direction = rec.normal;

		scattered = ray(rec.p, scatter_direction);
		attenuation = albedo;

		return true;
	}
private:
	//Albedo is the fraction of light that a surface reflects. 
	color albedo;
};

class metal : public material {
public:
	metal(const color& a, float f) : albedo(a), fuzzy(f < 1 ? f : 1) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
		const override {

		vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);

		scattered = ray(rec.p, reflected + fuzzy * random_unit_vector());
		attenuation = albedo;

		return (dot(scattered.direction(), rec.normal) > 0);

	}
private:
	color albedo;
	float fuzzy;
};

class dielectric : public material {
public:
	dielectric(float index_of_refraction) : ir(index_of_refraction) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
		const override {

		attenuation = color(1.f, 1.f, 1.f);

		// from air to the material, index of refraction of air is 1.0
		float refraction_ratio = rec.front_face ? (1.f / ir) : ir;

		vec3 unit_direction = unit_vector(r_in.direction());

		float cos_theta = fmin(dot(-unit_direction, rec.normal), 1.f);
		float sin_theta = sqrt(1.f - cos_theta * cos_theta);

		bool cannot_refract = refraction_ratio * sin_theta > 1.f;
		vec3 direction;

		if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float())
			direction = reflect(unit_direction, rec.normal);
		else
			direction = refract(unit_direction, rec.normal, refraction_ratio);

		scattered = ray(rec.p, direction);

		return true;

	}
private:
	float ir; // Index of Refraction

	static float reflectance(float cosine, float ref_idx) {

		// Use Schlick's approximation for reflectance
		auto r0 = (1.f - ref_idx) / (1 + ref_idx);
		r0 = r0 * r0;
		return r0 + (1 - r0) * pow((1 - cosine), 5);
	}
};
#endif // !MATERIAL_H
