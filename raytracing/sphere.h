#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
	sphere(point3 _center, float _radius, shared_ptr<material> _material): 
		center(_center), radius(_radius), m(_material) {}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override{
		vec3 oc = r.origin() - center;
		auto a = dot(r.direction(), r.direction());
		//auto b = 2.f * dot(r.direction(), oc);
		auto half_b = dot(r.direction(), oc);
		auto c = dot(oc, oc) - radius * radius;
		auto discriminant = half_b * half_b - a * c;
		// t < 0 : the obj behind the camera
		if (discriminant < 0) return false;
		auto sqrtd = sqrt(discriminant);


		auto root = (-half_b - sqrtd) / a;
		if (!ray_t.surrounds(root)) {
			root = (-half_b + sqrtd) / a;
			if (!ray_t.surrounds(root))
				return false;
		}

		rec.t = root;
		rec.p = r.at(rec.t);
		vec3 outward_normal = (rec.p - center) / radius;
		rec.set_face_normal(r, outward_normal);
		rec.m = m;

		return true;
	}
	
private:
	point3 center;
	float radius;
	shared_ptr<material> m;
};
#endif // !SPHERE_H
