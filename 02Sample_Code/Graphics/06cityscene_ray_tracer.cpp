#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <string>
#include <sstream>

const double PI = 3.14159265358979323846;
const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-4;

// ============================================================
// Vec3
// ============================================================
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(double t)    const { return {x*t, y*t, z*t}; }
    Vec3 operator*(const Vec3& o) const { return {x*o.x, y*o.y, z*o.z}; }
    Vec3 operator/(double t)    const { return {x/t, y/t, z/t}; }
    Vec3 operator-()            const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator*=(double t)      { x*=t; y*=t; z*=t; return *this; }

    double length() const { return std::sqrt(x*x + y*y + z*z); }
    double lengthSq() const { return x*x + y*y + z*z; }
    Vec3 normalized() const { return *this / length(); }
};

double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
Vec3 reflect(const Vec3& v, const Vec3& n) { return v - n * (2.0 * dot(v, n)); }
bool refract(const Vec3& uv, const Vec3& n, double eta, Vec3& out) {
    double cosT = std::min(dot(-uv, n), 1.0);
    Vec3 rPerp = (uv + n * cosT) * eta;
    double lenPerp = rPerp.lengthSq();
    if (lenPerp > 1.0) return false;
    Vec3 rPar = n * (-std::sqrt(1.0 - lenPerp));
    out = rPerp + rPar;
    return true;
}
double rng_uniform(std::mt19937& rng) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
}
double rng_range(std::mt19937& rng, double lo, double hi) {
    return lo + (hi - lo) * rng_uniform(rng);
}
Vec3 randomInUnitSphere(std::mt19937& rng) {
    std::uniform_real_distribution<double> d(-1, 1);
    while (true) {
        Vec3 p(d(rng), d(rng), d(rng));
        if (p.lengthSq() < 1.0) return p;
    }
}
Vec3 randomUnitVector(std::mt19937& rng) { return randomInUnitSphere(rng).normalized(); }
Vec3 randomInUnitDisk(std::mt19937& rng) {
    std::uniform_real_distribution<double> d(-1, 1);
    while (true) {
        Vec3 p(d(rng), d(rng), 0);
        if (p.lengthSq() < 1.0) return p;
    }
}

// ============================================================
// Ray
// ============================================================
struct Ray {
    Vec3 origin, direction;
    Ray() {}
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
    Vec3 at(double t) const { return origin + direction * t; }
};

// ============================================================
// Material
// ============================================================
enum class MaterialType { Lambertian, Metal, Dielectric, Emissive };

struct Material {
    MaterialType type;
    Vec3 albedo;
    double roughness;
    double ior;
    Vec3 emission;
    double emissionStrength;

    Material()
        : type(MaterialType::Lambertian), albedo(0.8, 0.8, 0.8),
          roughness(0.0), ior(1.5), emission(0, 0, 0), emissionStrength(0.0) {}

    static Material lambertian(const Vec3& c) {
        Material m; m.type = MaterialType::Lambertian; m.albedo = c; return m;
    }
    static Material metal(const Vec3& c, double rough = 0.0) {
        Material m; m.type = MaterialType::Metal; m.albedo = c; m.roughness = rough; return m;
    }
    static Material glass(double ior = 1.5) {
        Material m; m.type = MaterialType::Dielectric; m.ior = ior; return m;
    }
    static Material light(const Vec3& c, double strength = 1.0) {
        Material m; m.type = MaterialType::Emissive; m.emission = c; m.emissionStrength = strength; return m;
    }
};

// ============================================================
// AABB
// ============================================================
struct AABB {
    Vec3 min, max;
    AABB() : min(INF, INF, INF), max(-INF, -INF, -INF) {}
    AABB(const Vec3& lo, const Vec3& hi) : min(lo), max(hi) {}
    void expand(const Vec3& p) {
        min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);
    }
    void expand(const AABB& b) { expand(b.min); expand(b.max); }
    bool intersect(const Ray& r, double tMin, double tMax) const {
        for (int i = 0; i < 3; ++i) {
            double o    = i == 0 ? r.origin.x    : i == 1 ? r.origin.y    : r.origin.z;
            double d    = i == 0 ? r.direction.x : i == 1 ? r.direction.y : r.direction.z;
            double lo   = i == 0 ? min.x : i == 1 ? min.y : min.z;
            double hi   = i == 0 ? max.x : i == 1 ? max.y : max.z;
            double invD = 1.0 / d;
            double t0 = (lo - o) * invD;
            double t1 = (hi - o) * invD;
            if (invD < 0) std::swap(t0, t1);
            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;
            if (tMax <= tMin) return false;
        }
        return true;
    }
};

// ============================================================
// Hit
// ============================================================
struct Hit {
    double t;
    Vec3 point, normal;
    bool frontFace;
    Material material;
    Hit() : t(INF), frontFace(true) {}
    void setFaceNormal(const Ray& r, const Vec3& outwardNormal) {
        frontFace = dot(r.direction, outwardNormal) < 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

// ============================================================
// Shape base
// ============================================================
struct Shape {
    Material material;
    virtual ~Shape() = default;
    virtual bool intersect(const Ray& r, double tMin, double tMax, Hit& hit) const = 0;
    virtual AABB bounds() const = 0;
    virtual Vec3 centroid() const = 0;
};

// ============================================================
// Sphere
// ============================================================
struct Sphere : Shape {
    Vec3 center; double radius;
    Sphere(const Vec3& c, double r, const Material& m) : center(c), radius(r) {
        material = m;
    }
    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        Vec3 oc = ray.origin - center;
        double a = ray.direction.lengthSq();
        double hb = dot(oc, ray.direction);
        double c = oc.lengthSq() - radius * radius;
        double disc = hb * hb - a * c;
        if (disc < 0) return false;
        double sq = std::sqrt(disc);
        double t = (-hb - sq) / a;
        if (t < tMin || t > tMax) {
            t = (-hb + sq) / a;
            if (t < tMin || t > tMax) return false;
        }
        hit.t = t;
        hit.point = ray.at(t);
        hit.setFaceNormal(ray, (hit.point - center) / radius);
        hit.material = material;
        return true;
    }
    AABB bounds() const override {
        return AABB(center - Vec3(radius, radius, radius),
                    center + Vec3(radius, radius, radius));
    }
    Vec3 centroid() const override { return center; }
};

// ============================================================
// Plane (infinite, defined by point + normal)
// ============================================================
struct Plane : Shape {
    Vec3 point, normal;
    Plane(const Vec3& p, const Vec3& n, const Material& m)
        : point(p), normal(n.normalized()) { material = m; }
    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        double denom = dot(normal, ray.direction);
        if (std::abs(denom) < 1e-8) return false;
        double t = dot(point - ray.origin, normal) / denom;
        if (t < tMin || t > tMax) return false;
        hit.t = t;
        hit.point = ray.at(t);
        hit.setFaceNormal(ray, normal);
        hit.material = material;
        return true;
    }
    AABB bounds() const override {
        return AABB(Vec3(-1e6, -1e6, -1e6), Vec3(1e6, 1e6, 1e6));
    }
    Vec3 centroid() const override { return point; }
};

// ============================================================
// Axis-aligned Box
// ============================================================
struct Box : Shape {
    Vec3 center, half;
    Box(const Vec3& c, const Vec3& h, const Material& m) : center(c), half(h) {
        material = m;
    }
    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        double t0 = tMin, t1 = tMax;
        int hitAxis = -1;
        double hitSign = 1;
        for (int i = 0; i < 3; ++i) {
            double o = i == 0 ? ray.origin.x    : i == 1 ? ray.origin.y    : ray.origin.z;
            double d = i == 0 ? ray.direction.x : i == 1 ? ray.direction.y : ray.direction.z;
            double lo = (i == 0 ? center.x : i == 1 ? center.y : center.z)
                      - (i == 0 ? half.x   : i == 1 ? half.y   : half.z);
            double hi = (i == 0 ? center.x : i == 1 ? center.y : center.z)
                      + (i == 0 ? half.x   : i == 1 ? half.y   : half.z);
            double invD = 1.0 / d;
            double tNear = (lo - o) * invD;
            double tFar  = (hi - o) * invD;
            double sgn = -1;
            if (tNear > tFar) { std::swap(tNear, tFar); sgn = 1; }
            if (tNear > t0) { t0 = tNear; hitAxis = i; hitSign = sgn; }
            if (tFar  < t1) { t1 = tFar; }
            if (t1 <= t0) return false;
        }
        if (t0 < tMin || t0 > tMax) return false;
        hit.t = t0;
        hit.point = ray.at(t0);
        Vec3 outward(0, 0, 0);
        if (hitAxis == 0) outward = Vec3(hitSign, 0, 0);
        else if (hitAxis == 1) outward = Vec3(0, hitSign, 0);
        else outward = Vec3(0, 0, hitSign);
        hit.setFaceNormal(ray, outward);
        hit.material = material;
        return true;
    }
    AABB bounds() const override { return AABB(center - half, center + half); }
    Vec3 centroid() const override { return center; }
};

// ============================================================
// Y-axis Cylinder
// ============================================================
struct Cylinder : Shape {
    Vec3 base;
    double height, radius;
    Cylinder(const Vec3& b, double h, double r, const Material& m)
        : base(b), height(h), radius(r) { material = m; }
    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        Vec3 oc = ray.origin - base;
        double a = ray.direction.x * ray.direction.x + ray.direction.z * ray.direction.z;
        double b = 2.0 * (oc.x * ray.direction.x + oc.z * ray.direction.z);
        double c = oc.x * oc.x + oc.z * oc.z - radius * radius;

        double bestT = INF;
        Vec3 bestNormal;
        bool found = false;

        if (std::abs(a) > 1e-8) {
            double disc = b * b - 4 * a * c;
            if (disc >= 0) {
                double sq = std::sqrt(disc);
                for (int s = -1; s <= 1; s += 2) {
                    double t = (-b + s * sq) / (2 * a);
                    if (t < tMin || t > tMax) continue;
                    Vec3 p = ray.at(t);
                    double yL = p.y - base.y;
                    if (yL < 0 || yL > height) continue;
                    if (t < bestT) {
                        bestT = t;
                        bestNormal = Vec3(p.x - base.x, 0, p.z - base.z).normalized();
                        found = true;
                    }
                }
            }
        }

        for (int s = 0; s < 2; ++s) {
            double capY = base.y + s * height;
            if (std::abs(ray.direction.y) < 1e-8) continue;
            double t = (capY - ray.origin.y) / ray.direction.y;
            if (t < tMin || t > tMax) continue;
            Vec3 p = ray.at(t);
            double dx = p.x - base.x, dz = p.z - base.z;
            if (dx*dx + dz*dz > radius*radius) continue;
            if (t < bestT) {
                bestT = t;
                bestNormal = Vec3(0, s == 0 ? -1 : 1, 0);
                found = true;
            }
        }

        if (!found) return false;
        hit.t = bestT;
        hit.point = ray.at(bestT);
        hit.setFaceNormal(ray, bestNormal);
        hit.material = material;
        return true;
    }
    AABB bounds() const override {
        return AABB(Vec3(base.x - radius, base.y, base.z - radius),
                    Vec3(base.x + radius, base.y + height, base.z + radius));
    }
    Vec3 centroid() const override {
        return Vec3(base.x, base.y + height / 2.0, base.z);
    }
};

// ============================================================
// BVH
// ============================================================
struct BVHNode : Shape {
    std::unique_ptr<Shape> left, right;
    AABB box;
    BVHNode(std::vector<std::unique_ptr<Shape>>& shapes, int start, int end) {
        AABB bbox;
        for (int i = start; i < end; ++i) bbox.expand(shapes[i]->bounds());
        box = bbox;
        int n = end - start;
        if (n == 1) { left = std::move(shapes[start]); return; }
        if (n == 2) {
            left  = std::move(shapes[start]);
            right = std::move(shapes[start + 1]);
            return;
        }
        AABB cbox;
        for (int i = start; i < end; ++i) cbox.expand(shapes[i]->centroid());
        Vec3 ext = cbox.max - cbox.min;
        int axis = 0;
        if (ext.y > ext.x) axis = 1;
        if (ext.z > (axis == 0 ? ext.x : ext.y)) axis = 2;
        int mid = (start + end) / 2;
        std::nth_element(shapes.begin() + start, shapes.begin() + mid, shapes.begin() + end,
            [axis](const std::unique_ptr<Shape>& a, const std::unique_ptr<Shape>& b) {
                Vec3 ca = a->centroid(), cb = b->centroid();
                double va = axis == 0 ? ca.x : axis == 1 ? ca.y : ca.z;
                double vb = axis == 0 ? cb.x : axis == 1 ? cb.y : cb.z;
                return va < vb;
            });
        left  = std::make_unique<BVHNode>(shapes, start, mid);
        right = std::make_unique<BVHNode>(shapes, mid, end);
    }
    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        if (!box.intersect(ray, tMin, tMax)) return false;
        Hit lh, rh;
        bool hl = left  && left->intersect(ray, tMin, tMax, lh);
        bool hr = right && right->intersect(ray, tMin, tMax, rh);
        if (hl && hr) { hit = (lh.t < rh.t) ? lh : rh; return true; }
        if (hl) { hit = lh; return true; }
        if (hr) { hit = rh; return true; }
        return false;
    }
    AABB bounds() const override { return box; }
    Vec3 centroid() const override { return (box.min + box.max) / 2.0; }
};

// ============================================================
// Camera
// ============================================================
struct Camera {
    Vec3 origin, lowerLeft, horizontal, vertical, u, v, w;
    double lensRadius;
    Camera(const Vec3& from, const Vec3& at, const Vec3& up,
           double fovDeg, double aspect, double aperture, double focusDist) {
        double theta = fovDeg * PI / 180.0;
        double hh = std::tan(theta / 2.0);
        double hw = aspect * hh;
        origin = from;
        w = (from - at).normalized();
        u = cross(up, w).normalized();
        v = cross(w, u);
        lowerLeft = origin - u*(hw*focusDist) - v*(hh*focusDist) - w*focusDist;
        horizontal = u * (2.0 * hw * focusDist);
        vertical   = v * (2.0 * hh * focusDist);
        lensRadius = aperture / 2.0;
    }
    Ray getRay(double s, double t, std::mt19937& rng) const {
        Vec3 rd = randomInUnitDisk(rng) * lensRadius;
        Vec3 off = u * rd.x + v * rd.y;
        return Ray(origin + off,
                   lowerLeft + horizontal * s + vertical * t - origin - off);
    }
};

// ============================================================
// Scene
// ============================================================
struct Scene {
    std::unique_ptr<Shape> root;
    std::vector<std::unique_ptr<Shape>> allShapes;
    Vec3 lightPos, lightColor;
    double lightRadius;
    Vec3 ambient;
    Scene() : lightPos(0,0,0), lightColor(1,1,1), lightRadius(0.3),
              ambient(0.05,0.05,0.08) {}
    void add(std::unique_ptr<Shape> s) { allShapes.push_back(std::move(s)); }
    void buildBVH() {
        std::vector<std::unique_ptr<Shape>> work;
        work.reserve(allShapes.size());
        for (auto& s : allShapes) work.push_back(std::move(s));
        allShapes.clear();
        if (work.empty()) return;
        root = std::make_unique<BVHNode>(work, 0, (int)work.size());
    }
    bool closestHit(const Ray& ray, double tMin, double tMax, Hit& hit) const {
        if (root) return root->intersect(ray, tMin, tMax, hit);
        return false;
    }
};

// ============================================================
// Sky — daylight
// ============================================================
Vec3 skyColor(const Ray& ray) {
    Vec3 d = ray.direction.normalized();
    double t = 0.5 * (d.y + 1.0);
    return Vec3(0.85, 0.88, 0.95) * (1.0 - t) + Vec3(0.35, 0.60, 1.0) * t;
}

// ============================================================
// Forward decls + shading
// ============================================================
Vec3 trace(const Scene&, const Ray&, int, std::mt19937&);
Vec3 shade(const Scene&, const Ray&, const Hit&, int, std::mt19937&);

Vec3 shade(const Scene& scene, const Ray& ray, const Hit& hit,
           int depth, std::mt19937& rng) {
    if (depth <= 0) return Vec3(0, 0, 0);
    const Material& m = hit.material;
    if (m.type == MaterialType::Emissive) return m.emission * m.emissionStrength;
    if (m.type == MaterialType::Lambertian) {
        Vec3 target = hit.point + hit.normal + randomUnitVector(rng);
        Ray sr(hit.point + hit.normal * EPS, (target - hit.point).normalized());
        return m.albedo * trace(scene, sr, depth - 1, rng);
    }
    if (m.type == MaterialType::Metal) {
        Vec3 refl = reflect(ray.direction.normalized(), hit.normal);
        refl = refl + randomUnitVector(rng) * m.roughness;
        if (dot(refl, hit.normal) <= 0) return Vec3(0, 0, 0);
        Ray sr(hit.point + hit.normal * EPS, refl.normalized());
        return m.albedo * trace(scene, sr, depth - 1, rng);
    }
    if (m.type == MaterialType::Dielectric) {
        double ratio = hit.frontFace ? (1.0 / m.ior) : m.ior;
        Vec3 ud = ray.direction.normalized();
        Vec3 refr;
        if (refract(ud, hit.normal, ratio, refr)) {
            double cosT = std::min(dot(-ud, hit.normal), 1.0);
            double r0 = (1.0 - m.ior) / (1.0 + m.ior);
            r0 = r0 * r0;
            double fr = r0 + (1.0 - r0) * std::pow(1.0 - cosT, 5.0);
            if (rng_uniform(rng) < fr) {
                Vec3 r = reflect(ud, hit.normal);
                return trace(scene, Ray(hit.point + hit.normal * EPS, r.normalized()),
                             depth - 1, rng);
            }
            return trace(scene, Ray(hit.point + refr * EPS, refr.normalized()),
                         depth - 1, rng);
        }
        Vec3 r = reflect(ud, hit.normal);
        return trace(scene, Ray(hit.point + hit.normal * EPS, r.normalized()),
                     depth - 1, rng);
    }
    return Vec3(0, 0, 0);
}

Vec3 trace(const Scene& scene, const Ray& ray, int depth, std::mt19937& rng) {
    Hit hit;
    if (!scene.closestHit(ray, EPS, INF, hit)) return skyColor(ray);
    return shade(scene, ray, hit, depth, rng);
}

Vec3 toneMap(const Vec3& c, double exposure = 1.0) {
    Vec3 e = c * exposure;
    return Vec3(e.x/(1.0+e.x), e.y/(1.0+e.y), e.z/(1.0+e.z));
}
unsigned char toByte(double v) {
    return (unsigned char)(std::sqrt(std::max(0.0, std::min(1.0, v))) * 255.99);
}

// ===PART-2-BELOW===

// ============================================================
// Scene builders
// ============================================================
void addPerson(Scene& s, const Vec3& pos, double height,
               const Vec3& shirt, const Vec3& pants) {
    double legH = height * 0.45;
    double bodyH = height * 0.40;
    double headR = height * 0.09;
    double legR = height * 0.05;

    s.add(std::make_unique<Cylinder>(
        Vec3(pos.x - height*0.05, 0, pos.z), legH, legR,
        Material::lambertian(pants)));
    s.add(std::make_unique<Cylinder>(
        Vec3(pos.x + height*0.05, 0, pos.z), legH, legR,
        Material::lambertian(pants)));
    s.add(std::make_unique<Cylinder>(
        Vec3(pos.x, legH, pos.z), bodyH, height*0.09,
        Material::lambertian(shirt)));
    s.add(std::make_unique<Sphere>(
        Vec3(pos.x, legH + bodyH + headR, pos.z), headR,
        Material::lambertian(Vec3(0.85, 0.70, 0.55))));
}

void addCar(Scene& s, const Vec3& pos, const Vec3& bodyColor,
            double length = 2.4, double width = 1.1, double height = 0.7) {
    s.add(std::make_unique<Box>(
        Vec3(pos.x, height * 0.5, pos.z),
        Vec3(length * 0.5, height * 0.5, width * 0.5),
        Material::lambertian(bodyColor)));
    s.add(std::make_unique<Box>(
        Vec3(pos.x - 0.1, height + 0.25, pos.z),
        Vec3(length * 0.28, 0.25, width * 0.42),
        Material::metal(bodyColor * 0.7, 0.1)));

    double wr = 0.22;
    Material tire = Material::lambertian(Vec3(0.08, 0.08, 0.08));
    double wx = length * 0.32;
    double wy = width * 0.5;
    s.add(std::make_unique<Cylinder>(Vec3(pos.x - wx, 0, pos.z - wy - 0.02), wr*2, wr, tire));
    s.add(std::make_unique<Cylinder>(Vec3(pos.x + wx, 0, pos.z - wy - 0.02), wr*2, wr, tire));
    s.add(std::make_unique<Cylinder>(Vec3(pos.x - wx, 0, pos.z + wy + 0.02), wr*2, wr, tire));
    s.add(std::make_unique<Cylinder>(Vec3(pos.x + wx, 0, pos.z + wy + 0.02), wr*2, wr, tire));
}

void addBuilding(Scene& s, const Vec3& c, double w, double d, double h,
                 const Vec3& facade, std::mt19937& rng) {
    s.add(std::make_unique<Box>(
        Vec3(c.x, h * 0.5, c.z),
        Vec3(w * 0.5, h * 0.5, d * 0.5),
        Material::lambertian(facade)));

    int rows = std::max(1, (int)(h / 3.0));
    for (int r = 0; r < rows; ++r) {
        double y = 1.5 + r * 3.0;
        if (y > h - 0.8) break;
        for (int col = -2; col <= 2; ++col) {
            double x = c.x + col * (w * 0.17);
            double bright = rng_range(rng, 0.3, 0.9);
            s.add(std::make_unique<Box>(
                Vec3(x, y, c.z + d * 0.5 + 0.03),
                Vec3(0.4, 0.6, 0.03),
                Material::light(Vec3(1.0, 0.92, 0.75), bright)));
        }
    }
}

// ============================================================
// Main
// ============================================================
int main() {
    const int WIDTH  = 1000;
    const int HEIGHT = 600;
    const int SAMPLES = 64;
    const int MAX_DEPTH = 5;
    const int THREADS = std::max(1u, std::thread::hardware_concurrency());

    std::cout << "City Ray Tracer\n";
    std::cout << "Resolution: " << WIDTH << "x" << HEIGHT
              << "  Samples: " << SAMPLES
              << "  Threads: " << THREADS << "\n\n";

    Scene scene;
    scene.lightPos   = Vec3(30, 45, 25);
    scene.lightColor = Vec3(1.0, 0.96, 0.88);
    scene.lightRadius = 3.0;
    scene.ambient    = Vec3(0.18, 0.20, 0.25);

    std::mt19937 rng(2026);

    // -------- Ground (asphalt) --------
    scene.add(std::make_unique<Plane>(Vec3(0, 0, 0), Vec3(0, 1, 0),
        Material::lambertian(Vec3(0.32, 0.32, 0.34))));

    // -------- Road markings (yellow center line) --------
    for (int i = -20; i < 20; i += 2) {
        scene.add(std::make_unique<Box>(
            Vec3(0, 0.01, i * 2.0),
            Vec3(0.08, 0.005, 0.6),
            Material::lambertian(Vec3(0.95, 0.85, 0.25))));
    }

    // -------- Sidewalks --------
    double roadHalf = 5.0;
    double sidewalkW = 3.0;
    double sidewalkH = 0.18;
    for (int side = -1; side <= 1; side += 2) {
        scene.add(std::make_unique<Box>(
            Vec3(side * (roadHalf + sidewalkW * 0.5), sidewalkH * 0.5, 0),
            Vec3(sidewalkW * 0.5, sidewalkH * 0.5, 60.0),
            Material::lambertian(Vec3(0.72, 0.72, 0.70))));
    }

    // -------- Buildings on both sides --------
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 8; ++i) {
            double z = -45 + i * 12.0 + rng_range(rng, -1.5, 1.5);
            double w = rng_range(rng, 6.0, 9.0);
            double d = rng_range(rng, 6.0, 9.0);
            double h = rng_range(rng, 6.0, 16.0);
            double x = side * (roadHalf + sidewalkW + w * 0.5 + 0.3);

            double shade = rng_range(rng, 0.45, 0.80);
            Vec3 facade(shade * rng_range(rng, 0.9, 1.1),
                        shade * rng_range(rng, 0.85, 1.05),
                        shade * rng_range(rng, 0.8, 1.0));

            addBuilding(scene, Vec3(x, 0, z), w, d, h, facade, rng);
        }
    }

    // -------- Parked cars along the curbs --------
    std::vector<Vec3> carColors = {
        Vec3(0.85, 0.15, 0.15),
        Vec3(0.15, 0.35, 0.85),
        Vec3(0.90, 0.90, 0.90),
        Vec3(0.15, 0.15, 0.15),
        Vec3(0.90, 0.75, 0.15),
        Vec3(0.20, 0.65, 0.40),
    };
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 6; ++i) {
            double z = -40 + i * 14.0 + rng_range(rng, -1.0, 1.0);
            double x = side * (roadHalf - 1.0);
            Vec3 col = carColors[(int)(rng_uniform(rng) * carColors.size())];
            addCar(scene, Vec3(x, 0, z), col);
        }
    }

    // -------- Pedestrians on sidewalks --------
    std::vector<Vec3> shirtColors = {
        Vec3(0.85, 0.20, 0.25), Vec3(0.20, 0.55, 0.85),
        Vec3(0.95, 0.85, 0.20), Vec3(0.30, 0.80, 0.40),
        Vec3(0.80, 0.40, 0.80), Vec3(0.95, 0.95, 0.95),
    };
    std::vector<Vec3> pantColors = {
        Vec3(0.15, 0.15, 0.25), Vec3(0.30, 0.30, 0.35),
        Vec3(0.45, 0.35, 0.25),
    };
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 14; ++i) {
            double z = -50 + rng_range(rng, 0, 100);
            double x = side * (roadHalf + rng_range(rng, 0.4, sidewalkW - 0.4));
            double h = rng_range(rng, 1.6, 1.85);
            Vec3 shirt = shirtColors[(int)(rng_uniform(rng) * shirtColors.size())];
            Vec3 pants = pantColors[(int)(rng_uniform(rng) * pantColors.size())];
            addPerson(scene, Vec3(x, sidewalkH, z), h, shirt, pants);
        }
    }

    // -------- A few street lamps (tall cylinders with an emissive bulb) --------
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 4; ++i) {
            double z = -35 + i * 20.0;
            double x = side * (roadHalf + 1.5);
            scene.add(std::make_unique<Cylinder>(
                Vec3(x, sidewalkH, z), 5.5, 0.10,
                Material::metal(Vec3(0.25, 0.25, 0.28), 0.3)));
            scene.add(std::make_unique<Sphere>(
                Vec3(x, sidewalkH + 5.7, z), 0.28,
                Material::light(Vec3(1.0, 0.95, 0.75), 3.0)));
        }
    }

    scene.buildBVH();

    // -------- Camera: street-level view looking down the avenue --------
    Camera camera(
        Vec3(0.0, 2.4, 32.0),       // position
        Vec3(0.0, 2.2, 0.0),        // look towards origin
        Vec3(0, 1, 0),              // up
        55.0,                       // fov
        double(WIDTH) / HEIGHT,
        0.08,                       // slight depth of field
        32.0                        // focus distance
    );

    // -------- Render --------
    std::vector<Vec3> framebuffer(WIDTH * HEIGHT);
    std::atomic<int> nextRow{0};
    std::atomic<int> rowsDone{0};
    std::mutex printMutex;

    auto worker = [&](int tid) {
        std::mt19937 local(12345 + tid * 7919);
        while (true) {
            int y = nextRow.fetch_add(1);
            if (y >= HEIGHT) break;
            for (int x = 0; x < WIDTH; ++x) {
                Vec3 col(0, 0, 0);
                for (int s = 0; s < SAMPLES; ++s) {
                    double u = (x + rng_uniform(local)) / WIDTH;
                    double v = 1.0 - (y + rng_uniform(local)) / HEIGHT;
                    Ray r = camera.getRay(u, v, local);
                    col += trace(scene, r, MAX_DEPTH, local);
                }
                framebuffer[y * WIDTH + x] = col / double(SAMPLES);
            }
            int done = rowsDone.fetch_add(1) + 1;
            if (done % 30 == 0 || done == HEIGHT) {
                std::lock_guard<std::mutex> lock(printMutex);
                std::cout << "  progress: " << done << " / " << HEIGHT << " rows\n";
            }
        }
    };

    std::vector<std::thread> pool;
    for (int i = 0; i < THREADS; ++i) pool.emplace_back(worker, i);
    for (auto& t : pool) t.join();

    // -------- Write PPM --------
    std::ofstream out("city_scene.ppm", std::ios::binary);
    if (!out) { std::cerr << "Failed to open city_scene.ppm\n"; return 1; }
    out << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";
    for (const Vec3& c : framebuffer) {
        Vec3 m = toneMap(c, 1.0);
        unsigned char r = toByte(m.x);
        unsigned char g = toByte(m.y);
        unsigned char b = toByte(m.z);
        out.write(reinterpret_cast<char*>(&r), 1);
        out.write(reinterpret_cast<char*>(&g), 1);
        out.write(reinterpret_cast<char*>(&b), 1);
    }
    out.close();
    std::cout << "\nDone. Written to city_scene.ppm\n";
    return 0;
}