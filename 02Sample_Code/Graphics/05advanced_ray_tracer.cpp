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
    bool nearZero() const {
        return std::abs(x) < 1e-8 && std::abs(y) < 1e-8 && std::abs(z) < 1e-8;
    }
};

double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - n * (2.0 * dot(v, n));
}

// Refract using Snell's law. Returns false on total internal reflection.
bool refract(const Vec3& uv, const Vec3& n, double etaiOverEtat, Vec3& out) {
    double cosTheta = std::min(dot(-uv, n), 1.0);
    Vec3 rPerp = (uv + n * cosTheta) * etaiOverEtat;
    double lenPerp = rPerp.lengthSq();
    if (lenPerp > 1.0) return false;
    Vec3 rParallel = n * (-std::sqrt(1.0 - lenPerp));
    out = rPerp + rParallel;
    return true;
}

// Helper: uniform random double [0, 1)
double rng_uniform(std::mt19937& rng) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
}

Vec3 randomInUnitSphere(std::mt19937& rng) {
    std::uniform_real_distribution<double> d(-1, 1);
    while (true) {
        Vec3 p(d(rng), d(rng), d(rng));
        if (p.lengthSq() < 1.0) return p;
    }
}
Vec3 randomUnitVector(std::mt19937& rng) {
    return randomInUnitSphere(rng).normalized();
}
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
    Vec3 origin;
    Vec3 direction;
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
        Material m; m.type = MaterialType::Dielectric; m.albedo = Vec3(1,1,1); m.ior = ior; return m;
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
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }
    void expand(const AABB& b) {
        expand(b.min);
        expand(b.max);
    }
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
// Hit record
// ============================================================
struct Hit {
    double t;
    Vec3 point;
    Vec3 normal;
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
    Vec3 center;
    double radius;

    Sphere(const Vec3& c, double r, const Material& m) : center(c), radius(r) {
        material = m;
    }

    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        Vec3 oc = ray.origin - center;
        double a = ray.direction.lengthSq();
        double halfB = dot(oc, ray.direction);
        double c = oc.lengthSq() - radius * radius;
        double disc = halfB * halfB - a * c;
        if (disc < 0) return false;
        double sqrtD = std::sqrt(disc);
        double t = (-halfB - sqrtD) / a;
        if (t < tMin || t > tMax) {
            t = (-halfB + sqrtD) / a;
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
// Plane
// ============================================================
struct Plane : Shape {
    Vec3 point;
    Vec3 normal;

    Plane(const Vec3& p, const Vec3& n, const Material& m)
        : point(p), normal(n.normalized()) {
        material = m;
    }

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
// Triangle
// ============================================================
struct Triangle : Shape {
    Vec3 v0, v1, v2;

    Triangle(const Vec3& a, const Vec3& b, const Vec3& c, const Material& m)
        : v0(a), v1(b), v2(c) {
        material = m;
    }

    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        Vec3 pvec = cross(ray.direction, e2);
        double det = dot(e1, pvec);
        if (std::abs(det) < 1e-8) return false;
        double invDet = 1.0 / det;

        Vec3 tvec = ray.origin - v0;
        double u = dot(tvec, pvec) * invDet;
        if (u < 0 || u > 1) return false;

        Vec3 qvec = cross(tvec, e1);
        double v = dot(ray.direction, qvec) * invDet;
        if (v < 0 || u + v > 1) return false;

        double t = dot(e2, qvec) * invDet;
        if (t < tMin || t > tMax) return false;

        hit.t = t;
        hit.point = ray.at(t);
        Vec3 normal = cross(e1, e2).normalized();
        hit.setFaceNormal(ray, normal);
        hit.material = material;
        return true;
    }

    AABB bounds() const override {
        AABB b;
        b.expand(v0); b.expand(v1); b.expand(v2);
        return b;
    }
    Vec3 centroid() const override {
        return (v0 + v1 + v2) / 3.0;
    }
};

// ============================================================
// BVH
// ============================================================
struct BVHNode : Shape {
    std::unique_ptr<Shape> left;
    std::unique_ptr<Shape> right;
    AABB box;

    BVHNode(std::vector<std::unique_ptr<Shape>>& shapes, int start, int end) {
        AABB bbox;
        for (int i = start; i < end; ++i) bbox.expand(shapes[i]->bounds());
        box = bbox;

        int n = end - start;
        if (n == 1) {
            left = std::move(shapes[start]);
            right = nullptr;
            return;
        }
        if (n == 2) {
            left = std::move(shapes[start]);
            right = std::move(shapes[start + 1]);
            return;
        }

        // Split along longest centroid extent
        AABB centroidBox;
        for (int i = start; i < end; ++i) centroidBox.expand(shapes[i]->centroid());
        Vec3 extent = centroidBox.max - centroidBox.min;

        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > (axis == 0 ? extent.x : extent.y)) axis = 2;

        int mid = (start + end) / 2;
        std::nth_element(shapes.begin() + start, shapes.begin() + mid, shapes.begin() + end,
            [axis](const std::unique_ptr<Shape>& a, const std::unique_ptr<Shape>& b) {
                Vec3 ca = a->centroid();
                Vec3 cb = b->centroid();
                double va = axis == 0 ? ca.x : axis == 1 ? ca.y : ca.z;
                double vb = axis == 0 ? cb.x : axis == 1 ? cb.y : cb.z;
                return va < vb;
            });

        left = std::make_unique<BVHNode>(shapes, start, mid);
        right = std::make_unique<BVHNode>(shapes, mid, end);
    }

    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        if (!box.intersect(ray, tMin, tMax)) return false;

        Hit leftHit, rightHit;
        bool hitLeft = left && left->intersect(ray, tMin, tMax, leftHit);
        bool hitRight = right && right->intersect(ray, tMin, tMax, rightHit);

        if (hitLeft && hitRight) {
            hit = (leftHit.t < rightHit.t) ? leftHit : rightHit;
            return true;
        } else if (hitLeft) {
            hit = leftHit;
            return true;
        } else if (hitRight) {
            hit = rightHit;
            return true;
        }
        return false;
    }

    AABB bounds() const override { return box; }
    Vec3 centroid() const override {
        return (box.min + box.max) / 2.0;
    }
};

// ============================================================
// OBJ loader
// ============================================================
bool loadOBJ(const std::string& filename,
             const Material& mat,
             std::vector<std::unique_ptr<Shape>>& out,
             const Vec3& offset = Vec3(0,0,0),
             double scale = 1.0) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open OBJ: " << filename << "\n";
        return false;
    }

    std::vector<Vec3> vertices;
    std::string line;
    int triangleCount = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vec3(x, y, z) * scale + offset);
        } else if (tag == "f") {
            std::vector<int> idx;
            std::string token;
            while (iss >> token) {
                size_t slash = token.find('/');
                int i = std::stoi(slash == std::string::npos ? token : token.substr(0, slash));
                if (i < 0) i = (int)vertices.size() + i;
                else i -= 1;
                idx.push_back(i);
            }
            for (size_t i = 1; i + 1 < idx.size(); ++i) {
                if (idx[0] < 0 || idx[0] >= (int)vertices.size()) continue;
                if (idx[i] < 0 || idx[i] >= (int)vertices.size()) continue;
                if (idx[i+1] < 0 || idx[i+1] >= (int)vertices.size()) continue;
                out.push_back(std::make_unique<Triangle>(
                    vertices[idx[0]], vertices[idx[i]], vertices[idx[i+1]], mat));
                triangleCount++;
            }
        }
    }
    std::cout << "Loaded " << triangleCount << " triangles from " << filename << "\n";
    return true;
}

// ============================================================
// Camera
// ============================================================
struct Camera {
    Vec3 origin;
    Vec3 lowerLeft;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w;
    double lensRadius;

    Camera(const Vec3& lookFrom, const Vec3& lookAt, const Vec3& up,
           double fovDegrees, double aspect, double aperture, double focusDist) {
        double theta = fovDegrees * PI / 180.0;
        double halfHeight = std::tan(theta / 2.0);
        double halfWidth = aspect * halfHeight;

        origin = lookFrom;
        w = (lookFrom - lookAt).normalized();
        u = cross(up, w).normalized();
        v = cross(w, u);

        lowerLeft = origin - u * (halfWidth * focusDist)
                            - v * (halfHeight * focusDist)
                            - w * focusDist;
        horizontal = u * (2.0 * halfWidth * focusDist);
        vertical   = v * (2.0 * halfHeight * focusDist);
        lensRadius = aperture / 2.0;
    }

    Ray getRay(double s, double t, std::mt19937& rng) const {
        Vec3 rd = randomInUnitDisk(rng) * lensRadius;
        Vec3 offset = u * rd.x + v * rd.y;
        return Ray(origin + offset,
                   lowerLeft + horizontal * s + vertical * t - origin - offset);
    }
};

// ============================================================
// Scene
// ============================================================
struct Scene {
    std::unique_ptr<Shape> root;
    std::vector<std::unique_ptr<Shape>> allShapes;
    Vec3 lightPos;
    Vec3 lightColor;
    double lightRadius;
    Vec3 ambient;

    Scene() : lightPos(0, 0, 0), lightColor(1, 1, 1), lightRadius(0.3),
              ambient(0.05, 0.05, 0.08) {}

    void add(std::unique_ptr<Shape> s) {
        allShapes.push_back(std::move(s));
    }

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
// Sky
// ============================================================
Vec3 skyColor(const Ray& ray) {
    Vec3 unitDir = ray.direction.normalized();
    double t = 0.5 * (unitDir.y + 1.0);
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.4, 0.6, 1.0) * t;
}

// ============================================================
// Forward declarations — mutually recursive
// ============================================================
Vec3 trace(const Scene& scene, const Ray& ray, int depth, std::mt19937& rng);
Vec3 shade(const Scene& scene, const Ray& ray, const Hit& hit, int depth, std::mt19937& rng);

// ============================================================
// Shade one bounce
// ============================================================
Vec3 shade(const Scene& scene, const Ray& ray, const Hit& hit,
           int depth, std::mt19937& rng) {
    if (depth <= 0) return Vec3(0, 0, 0);

    const Material& m = hit.material;

    // Emissive
    if (m.type == MaterialType::Emissive) {
        return m.emission * m.emissionStrength;
    }

    // Lambertian
    if (m.type == MaterialType::Lambertian) {
        Vec3 target = hit.point + hit.normal + randomUnitVector(rng);
        Ray scatterRay(hit.point + hit.normal * EPS, (target - hit.point).normalized());
        Vec3 incoming = trace(scene, scatterRay, depth - 1, rng);
        return m.albedo * incoming;
    }

    // Metal
    if (m.type == MaterialType::Metal) {
        Vec3 reflected = reflect(ray.direction.normalized(), hit.normal);
        reflected = reflected + randomUnitVector(rng) * m.roughness;
        if (dot(reflected, hit.normal) <= 0) return Vec3(0, 0, 0);
        Ray scatterRay(hit.point + hit.normal * EPS, reflected.normalized());
        Vec3 incoming = trace(scene, scatterRay, depth - 1, rng);
        return m.albedo * incoming;
    }

    // Dielectric (glass)
    if (m.type == MaterialType::Dielectric) {
        double ratio = hit.frontFace ? (1.0 / m.ior) : m.ior;
        Vec3 unitDir = ray.direction.normalized();

        Vec3 refracted;
        if (refract(unitDir, hit.normal, ratio, refracted)) {
            double cosTheta = std::min(dot(-unitDir, hit.normal), 1.0);
            double r0 = (1.0 - m.ior) / (1.0 + m.ior);
            r0 = r0 * r0;
            double fresnel = r0 + (1.0 - r0) * std::pow(1.0 - cosTheta, 5.0);
            if (rng_uniform(rng) < fresnel) {
                Vec3 reflected = reflect(unitDir, hit.normal);
                Ray r(hit.point + hit.normal * EPS, reflected.normalized());
                return trace(scene, r, depth - 1, rng);
            } else {
                Ray r(hit.point + refracted * EPS, refracted.normalized());
                return trace(scene, r, depth - 1, rng);
            }
        } else {
            Vec3 reflected = reflect(unitDir, hit.normal);
            Ray r(hit.point + hit.normal * EPS, reflected.normalized());
            return trace(scene, r, depth - 1, rng);
        }
    }

    return Vec3(0, 0, 0);
}

// ============================================================
// Trace
// ============================================================
Vec3 trace(const Scene& scene, const Ray& ray, int depth, std::mt19937& rng) {
    Hit hit;
    if (!scene.closestHit(ray, EPS, INF, hit)) {
        return skyColor(ray);
    }
    return shade(scene, ray, hit, depth, rng);
}

// ============================================================
// Tone mapping
// ============================================================
Vec3 toneMap(const Vec3& c, double exposure = 1.0) {
    Vec3 e = c * exposure;
    return Vec3(e.x / (1.0 + e.x), e.y / (1.0 + e.y), e.z / (1.0 + e.z));
}

unsigned char toByte(double v) {
    return (unsigned char)(std::sqrt(std::max(0.0, std::min(1.0, v))) * 255.99);
}

// ============================================================
// Main
// ============================================================
int main(int argc, char** argv) {
    const int WIDTH  = 900;
    const int HEIGHT = 600;
    const int SAMPLES = 64;
    const int MAX_DEPTH = 8;
    const int THREADS = std::max(1u, std::thread::hardware_concurrency());

    std::cout << "Ray Tracer (Advanced)\n";
    std::cout << "Resolution: " << WIDTH << "x" << HEIGHT << "\n";
    std::cout << "Samples/pixel: " << SAMPLES << "\n";
    std::cout << "Max depth: " << MAX_DEPTH << "\n";
    std::cout << "Threads: " << THREADS << "\n\n";

    // ------------------------------------------------------------
    // Scene
    // ------------------------------------------------------------
    Scene scene;
    scene.lightPos = Vec3(-4, 8, 4);
    scene.lightColor = Vec3(1.0, 0.95, 0.85);
    scene.lightRadius = 0.8;
    scene.ambient = Vec3(0.05, 0.05, 0.08);

    scene.add(std::make_unique<Plane>(Vec3(0, 0, 0), Vec3(0, 1, 0),
        Material::lambertian(Vec3(0.55, 0.55, 0.6))));

    scene.add(std::make_unique<Sphere>(Vec3(-1.8, 0.6, 0), 0.6,
        Material::lambertian(Vec3(0.85, 0.25, 0.25))));

    scene.add(std::make_unique<Sphere>(Vec3(1.8, 0.6, 0), 0.6,
        Material::lambertian(Vec3(0.2, 0.8, 0.3))));

    scene.add(std::make_unique<Sphere>(Vec3(0, 0.9, -1.5), 0.9,
        Material::metal(Vec3(0.95, 0.95, 0.95), 0.02)));

    scene.add(std::make_unique<Sphere>(Vec3(-0.9, 0.5, 1.3), 0.5,
        Material::glass(1.5)));

    scene.add(std::make_unique<Sphere>(Vec3(1.5, 0.5, 1.2), 0.5,
        Material::metal(Vec3(0.9, 0.7, 0.4), 0.3)));

    scene.add(std::make_unique<Sphere>(Vec3(-3, 3, -2), 0.25,
        Material::light(Vec3(1.0, 0.6, 0.3), 4.0)));

    // Optional OBJ model (uncomment if you have one):
    // std::vector<std::unique_ptr<Shape>> objShapes;
    // loadOBJ("bunny.obj", Material::lambertian(Vec3(0.8, 0.6, 0.9)),
    //         objShapes, Vec3(0, 0.3, 0), 1.0);
    // for (auto& s : objShapes) scene.add(std::move(s));

    scene.buildBVH();

    // ------------------------------------------------------------
    // Camera
    // ------------------------------------------------------------
    Camera camera(
        Vec3(0, 2.2, 6),
        Vec3(0, 0.8, 0),
        Vec3(0, 1, 0),
        55.0,
        double(WIDTH)/HEIGHT,
        0.15,
        7.0
    );

    // ------------------------------------------------------------
    // Render
    // ------------------------------------------------------------
    std::vector<Vec3> framebuffer(WIDTH * HEIGHT);
    std::atomic<int> nextRow{0};
    std::atomic<int> rowsDone{0};
    std::mutex printMutex;

    auto worker = [&](int threadId) {
        std::mt19937 rng(12345 + threadId * 7919);

        while (true) {
            int y = nextRow.fetch_add(1);
            if (y >= HEIGHT) break;

            for (int x = 0; x < WIDTH; ++x) {
                Vec3 pixelColor(0, 0, 0);
                for (int s = 0; s < SAMPLES; ++s) {
                    double u = (x + rng_uniform(rng)) / WIDTH;
                    double v = 1.0 - (y + rng_uniform(rng)) / HEIGHT;
                    Ray ray = camera.getRay(u, v, rng);
                    pixelColor += trace(scene, ray, MAX_DEPTH, rng);
                }
                pixelColor = pixelColor / double(SAMPLES);
                framebuffer[y * WIDTH + x] = pixelColor;
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

    // ------------------------------------------------------------
    // Output
    // ------------------------------------------------------------
    std::ofstream out("render_advanced.ppm", std::ios::binary);
    if (!out) {
        std::cerr << "Failed to write render_advanced.ppm\n";
        return 1;
    }
    out << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";

    for (const Vec3& c : framebuffer) {
        Vec3 mapped = toneMap(c, 1.0);
        unsigned char r = toByte(mapped.x);
        unsigned char g = toByte(mapped.y);
        unsigned char b = toByte(mapped.z);
        out.write(reinterpret_cast<char*>(&r), 1);
        out.write(reinterpret_cast<char*>(&g), 1);
        out.write(reinterpret_cast<char*>(&b), 1);
    }

    out.close();
    std::cout << "\nDone. Written to render_advanced.ppm\n";
    return 0;
}