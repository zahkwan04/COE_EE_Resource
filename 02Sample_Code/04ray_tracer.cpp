#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

// ============================================================
// Vec3 — 3D vector with basic math operations
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
    Vec3 normalized() const { return *this / length(); }
};

double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x};
}

Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - n * (2.0 * dot(v, n));
}

// ============================================================
// Ray
// ============================================================
struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}

    Vec3 at(double t) const { return origin + direction * t; }
};

// ============================================================
// Material
// ============================================================
struct Material {
    Vec3 color;
    double reflectivity;    // 0.0 = matte, 1.0 = mirror
    double shininess;       // specular highlight tightness

    Material() : color(0.8, 0.8, 0.8), reflectivity(0.0), shininess(32.0) {}
    Material(const Vec3& c, double r = 0.0, double s = 32.0)
        : color(c), reflectivity(r), shininess(s) {}
};

// ============================================================
// Hit record — info about an intersection
// ============================================================
struct Hit {
    double t;               // distance along ray
    Vec3 point;             // intersection point
    Vec3 normal;            // surface normal
    Material material;

    Hit() : t(std::numeric_limits<double>::max()) {}
};

// ============================================================
// Shape (abstract base)
// ============================================================
struct Shape {
    Material material;
    virtual ~Shape() = default;
    virtual bool intersect(const Ray& r, double tMin, double tMax, Hit& hit) const = 0;
};

// ============================================================
// Sphere
// ============================================================
struct Sphere : Shape {
    Vec3 center;
    double radius;

    Sphere(const Vec3& c, double r, const Material& m)
        : center(c), radius(r) {
        material = m;
    }

    bool intersect(const Ray& ray, double tMin, double tMax, Hit& hit) const override {
        Vec3 oc = ray.origin - center;
        double a = dot(ray.direction, ray.direction);
        double b = 2.0 * dot(oc, ray.direction);
        double c = dot(oc, oc) - radius * radius;

        double disc = b*b - 4*a*c;
        if (disc < 0) return false;

        double sqrtD = std::sqrt(disc);
        double t = (-b - sqrtD) / (2*a);
        if (t < tMin || t > tMax) {
            t = (-b + sqrtD) / (2*a);
            if (t < tMin || t > tMax) return false;
        }

        hit.t = t;
        hit.point = ray.at(t);
        hit.normal = (hit.point - center).normalized();
        hit.material = material;
        return true;
    }
};

// ============================================================
// Plane (infinite, defined by point + normal)
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
        if (std::abs(denom) < 1e-6) return false;   // parallel

        double t = dot(point - ray.origin, normal) / denom;
        if (t < tMin || t > tMax) return false;

        hit.t = t;
        hit.point = ray.at(t);
        hit.normal = normal;
        hit.material = material;
        return true;
    }
};

// ============================================================
// Scene — collection of shapes
// ============================================================
struct Scene {
    std::vector<Shape*> shapes;
    Vec3 lightPos;
    Vec3 lightColor;
    Vec3 ambient;

    Scene() : lightPos(5, 8, 3), lightColor(1.0, 1.0, 1.0), ambient(0.1, 0.1, 0.15) {}

    ~Scene() {
        for (auto s : shapes) delete s;
    }

    void add(Shape* s) { shapes.push_back(s); }

    // Find the closest intersection along [tMin, tMax]
    bool closestHit(const Ray& ray, double tMin, double tMax, Hit& hit) const {
        Hit temp;
        bool found = false;
        double closest = tMax;

        for (auto s : shapes) {
            if (s->intersect(ray, tMin, closest, temp)) {
                found = true;
                closest = temp.t;
                hit = temp;
            }
        }
        return found;
    }

    // Is this point in shadow from the light?
    bool inShadow(const Vec3& point) const {
        Vec3 toLight = lightPos - point;
        double dist = toLight.length();
        Ray shadowRay(point, toLight.normalized());

        Hit hit;
        return closestHit(shadowRay, 0.001, dist, hit);
    }
};

// ============================================================
// Sky background
// ============================================================
Vec3 skyColor(const Ray& ray) {
    Vec3 unitDir = ray.direction.normalized();
    double t = 0.5 * (unitDir.y + 1.0);
    // Lerp between white and light blue
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
}

// ============================================================
// Shading: compute color at a hit point
// ============================================================
Vec3 shade(const Scene& scene, const Ray& ray, const Hit& hit, int depth) {
    if (depth <= 0) return Vec3(0, 0, 0);

    Vec3 baseColor = hit.material.color;
    Vec3 normal = hit.normal;
    Vec3 viewDir = -ray.direction.normalized();

    // Start with ambient
    Vec3 result = baseColor * scene.ambient;

    // Direction to light
    Vec3 toLight = scene.lightPos - hit.point;
    double lightDist = toLight.length();
    Vec3 lightDir = toLight / lightDist;

    // Diffuse (Lambertian) — only if not in shadow
    if (!scene.inShadow(hit.point)) {
        double diff = std::max(0.0, dot(normal, lightDir));
        result += baseColor * scene.lightColor * diff;

        // Specular highlight (Blinn-Phong)
        Vec3 halfway = (lightDir + viewDir).normalized();
        double spec = std::pow(std::max(0.0, dot(normal, halfway)), hit.material.shininess);
        result += scene.lightColor * spec * 0.5;
    }

    // Reflection
    if (hit.material.reflectivity > 0.0 && depth > 1) {
        Vec3 reflectDir = reflect(ray.direction.normalized(), normal).normalized();
        Ray reflectRay(hit.point + normal * 0.001, reflectDir);

        Hit reflectHit;
        if (scene.closestHit(reflectRay, 0.001, 1e9, reflectHit)) {
            Vec3 reflectedColor = shade(scene, reflectRay, reflectHit, depth - 1);
            result = result * (1.0 - hit.material.reflectivity)
                   + reflectedColor * hit.material.reflectivity;
        } else {
            Vec3 reflectedColor = skyColor(reflectRay);
            result = result * (1.0 - hit.material.reflectivity)
                   + reflectedColor * hit.material.reflectivity;
        }
    }

    return result;
}

// ============================================================
// Trace a single ray through the scene
// ============================================================
Vec3 trace(const Scene& scene, const Ray& ray, int depth) {
    Hit hit;
    if (scene.closestHit(ray, 0.001, 1e9, hit)) {
        return shade(scene, ray, hit, depth);
    }
    return skyColor(ray);
}

// ============================================================
// Main — set up scene, render, write PPM
// ============================================================
int main() {
    const int WIDTH  = 600;
    const int HEIGHT = 400;
    const int SAMPLES_PER_PIXEL = 4;    // anti-aliasing
    const int MAX_DEPTH = 5;            // reflection bounces

    // ------------------------------------------------------------
    // Camera
    // ------------------------------------------------------------
    Vec3 cameraPos(0, 1.5, 5);
    Vec3 lookAt(0, 0.5, 0);
    Vec3 worldUp(0, 1, 0);

    Vec3 forward = (lookAt - cameraPos).normalized();
    Vec3 right   = cross(forward, worldUp).normalized();
    Vec3 up      = cross(right, forward);

    double fov = 60.0;   // degrees
    double aspect = double(WIDTH) / double(HEIGHT);
    double halfHeight = std::tan(fov * M_PI / 360.0);
    double halfWidth  = halfHeight * aspect;

    // ------------------------------------------------------------
    // Scene
    // ------------------------------------------------------------
    Scene scene;
    scene.lightPos = Vec3(5, 8, 4);
    scene.lightColor = Vec3(1.0, 1.0, 1.0);
    scene.ambient = Vec3(0.08, 0.08, 0.12);

    // Ground plane
    scene.add(new Plane(Vec3(0, 0, 0), Vec3(0, 1, 0),
                        Material(Vec3(0.6, 0.6, 0.6), 0.1, 16)));

    // Red sphere (matte)
    scene.add(new Sphere(Vec3(-1.2, 0.6, 0), 0.6,
                         Material(Vec3(0.85, 0.25, 0.25), 0.1, 32)));

    // Green sphere (matte)
    scene.add(new Sphere(Vec3(1.2, 0.6, 0), 0.6,
                         Material(Vec3(0.25, 0.85, 0.35), 0.1, 32)));

    // Big mirror sphere
    scene.add(new Sphere(Vec3(0, 0.9, -1.5), 0.9,
                         Material(Vec3(0.9, 0.9, 0.95), 0.85, 128)));

    // Small yellow sphere floating
    scene.add(new Sphere(Vec3(-0.3, 2.2, -3), 0.3,
                         Material(Vec3(0.95, 0.85, 0.2), 0.2, 64)));

    // ------------------------------------------------------------
    // Render
    // ------------------------------------------------------------
    std::vector<Vec3> framebuffer(WIDTH * HEIGHT);

    std::cout << "Rendering " << WIDTH << "x" << HEIGHT
              << " with " << SAMPLES_PER_PIXEL << " spp...\n";

    for (int y = 0; y < HEIGHT; ++y) {
        if (y % 40 == 0) {
            std::cout << "  row " << y << " / " << HEIGHT << "\n";
        }

        for (int x = 0; x < WIDTH; ++x) {
            Vec3 pixelColor(0, 0, 0);

            // Anti-aliasing: multiple samples per pixel
            for (int s = 0; s < SAMPLES_PER_PIXEL; ++s) {
                double jx = (double)rand() / RAND_MAX;
                double jy = (double)rand() / RAND_MAX;

                double u = (2.0 * (x + jx) / WIDTH  - 1.0) * halfWidth;
                double v = (1.0 - 2.0 * (y + jy) / HEIGHT) * halfHeight;

                Vec3 dir = (forward + right * u + up * v).normalized();
                Ray ray(cameraPos, dir);

                pixelColor += trace(scene, ray, MAX_DEPTH);
            }

            pixelColor = pixelColor / double(SAMPLES_PER_PIXEL);

            // Gamma correction (simple)
            pixelColor.x = std::sqrt(std::max(0.0, std::min(1.0, pixelColor.x)));
            pixelColor.y = std::sqrt(std::max(0.0, std::min(1.0, pixelColor.y)));
            pixelColor.z = std::sqrt(std::max(0.0, std::min(1.0, pixelColor.z)));

            framebuffer[y * WIDTH + x] = pixelColor;
        }
    }

    // ------------------------------------------------------------
    // Write PPM
    // ------------------------------------------------------------
    std::ofstream out("render.ppm", std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open render.ppm for writing\n";
        return 1;
    }

    out << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";

    for (const Vec3& c : framebuffer) {
        unsigned char r = (unsigned char)(c.x * 255.99);
        unsigned char g = (unsigned char)(c.y * 255.99);
        unsigned char b = (unsigned char)(c.z * 255.99);
        out.write(reinterpret_cast<char*>(&r), 1);
        out.write(reinterpret_cast<char*>(&g), 1);
        out.write(reinterpret_cast<char*>(&b), 1);
    }

    out.close();
    std::cout << "\nDone! Written to render.ppm\n";
    std::cout << "Open it with any image viewer (IrfanView, GIMP, Photoshop, etc.)\n";
    return 0;
}