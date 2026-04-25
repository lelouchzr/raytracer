#include "rt/engine.hpp"
#include <iostream>
#include <stdexcept>
using namespace rt;

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

void close(double a, double b) {
    require(std::abs(a - b) < 1e-7, "numerical mismatch");
}

template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    require(failed, "expected rejection");
}

int main() {
    try {
        Factory factory;
#ifdef RT_BONUS
        registerBonus(factory);
#endif
        auto material = std::make_shared<FlatMaterial>(Vec3{1, 0, 0});
        Hit h;
        auto sphere = factory.primitive(
            Json::parse(R"({"type":"sphere","radius":1,"translate":[0,0,-4]})"), material);
        require(sphere->intersect({{0, 0, 0}, {0, 0, -1}}, epsilon, infinity, h),
                "translated sphere missed");
        close(h.t, 3);
        require(h.front, "front face");
        require(sphere->intersect({{0, 0, -4}, {1, 0, 0}}, epsilon, infinity, h),
                "inside sphere missed");
        close(h.t, 1);
        require(!h.front, "back face");
        require(sphere->intersect({{1, 0, 0}, {0, 0, -1}}, epsilon, infinity, h),
                "tangent sphere missed");
        close(h.t, 4);
        require(!sphere->intersect({{2, 0, 0}, {0, 0, -1}}, epsilon, infinity, h),
                "sphere false positive");
        require(!sphere->intersect({{0, 0, 0}, {0, 0, -1}}, epsilon, 2, h),
                "sphere interval ignored");
        auto plane = factory.primitive(
            Json::parse(R"({"type":"plane","rotate":[0,0,90],"translate":[2,0,0]})"), material);
        require(plane->intersect({{0, 0, 0}, {1, 0, 0}}, epsilon, infinity, h),
                "rotated plane missed");
        close(h.t, 2);
        require(!plane->intersect({{0, 0, 0}, {0, 0, 1}}, epsilon, infinity, h),
                "parallel plane false hit");
        auto cylinder =
            factory.primitive(Json::parse(R"({"type":"cylinder","radius":1})"), material);
        require(cylinder->intersect({{0, 0, 3}, {0, 0, -1}}, epsilon, infinity, h),
                "cylinder missed");
        close(h.t, 2);
        require(cylinder->intersect({{0, 0, 0}, {1, 0, 0}}, epsilon, infinity, h),
                "inside cylinder missed");
        close(h.t, 1);
        require(!cylinder->intersect({{2, 0, 0}, {0, 1, 0}}, epsilon, infinity, h),
                "axis-parallel cylinder false positive");
        auto cone = factory.primitive(Json::parse(R"({"type":"cone","radius":1})"), material);
        require(cone->intersect({{0, 1, 3}, {0, 0, -1}}, epsilon, infinity, h), "cone missed");
        close(h.t, 2);
        // A ray on the cone's asymptote exercises the linear equation branch.
        require(cone->intersect({{0, 1, 3}, unit({0, 1, -1})}, epsilon, infinity, h),
                "linear cone missed");
        Bounds box{{-1, -1, -1}, {1, 1, 1}};
        require(box.hit({{0, 0, 3}, {0, 0, -1}}, 0, 10), "slab miss");
        require(!box.hit({{2, 0, 3}, {0, 0, -1}}, 0, 10), "parallel slab false hit");
        auto j = Json::parse(R"({"a":[1,-2,3e1],"utf":"\u00e9"})");
        close(j.at("a").vector().z, 30);
        for (const auto &bad :
             {"{", "[1,]", "{\"x\":1,\"x\":2}", "01", "1e", "1e999", "\"\\q\"", "true false"})
            rejects([&] { Json::parse(bad); });
        rejects(
            [&] { factory.primitive(Json::parse(R"({"type":"sphere","radius":-1})"), material); });
#ifdef RT_BONUS
        auto cap =
            factory.primitive(Json::parse(R"({"type":"limited_cylinder","height":2})"), material);
        require(cap->intersect({{0, 3, 0}, {0, -1, 0}}, epsilon, infinity, h),
                "cylinder cap missed");
        close(h.t, 1);
        auto marble = factory.materials.at("phong")(
            Json::parse(R"({"type":"phong","pattern":"marble"})"), {});
        require(std::isfinite(marble->sample({1e20, -1e20, 1e20}).color.length2()),
                "distant procedural texture is non-finite");
        auto ellipsoid =
            factory.primitive(Json::parse(R"({"type":"sphere","scale":[2,1,0.5]})"), material);
        require(ellipsoid->intersect({{0, 0, 3}, {0, 0, -1}}, epsilon, infinity, h),
                "scaled sphere missed");
        close(h.t, 2.5);
        auto tilted = factory.primitive(
            Json::parse(R"({"type":"plane","normal":[1,1,0],"scale":[2,1,1]})"), material);
        require(tilted->intersect({{0, 3, 0}, {0, -1, 0}}, epsilon, infinity, h),
                "scaled plane missed");
        close(h.normal.x / h.normal.y, .5);
        rejects([&] {
            factory.primitive(Json::parse(R"({"type":"sphere","scale":[0,1,1]})"), material);
        });
        Scene accelerated;
        accelerated.objects.push_back(plane);
        for (int i = 0; i < 60; ++i) {
            Json::Object definition;
            definition["type"] = Json(std::string("sphere"));
            definition["radius"] = Json(.2 + double(i % 4) * .1);
            definition["translate"] = Json(Json::Array{
                Json(double(i % 10) - 5), Json(double(i / 10) - 3), Json(-double(i % 5))});
            accelerated.objects.push_back(factory.primitive(Json(std::move(definition)), material));
        }
        configureBonus(accelerated);
        for (int i = 0; i < 1200; ++i) {
            Ray ray{{0, 1, 8}, unit({std::sin(i * 13.7) * 1.3, std::cos(i * 7.3), -1})};
            Hit fast, slow;
            bool a = accelerated.intersect(ray, epsilon, infinity, fast), b = false;
            double nearest = infinity;
            for (const auto &object : accelerated.objects)
                if (object->intersect(ray, epsilon, nearest, slow)) {
                    nearest = slow.t;
                    b = true;
                }
            require(a == b, "BVH disagrees with linear intersection");
            if (a)
                close(fast.t, nearest);
        }
        auto triangle = factory.primitive(
            Json::parse(R"({"type":"triangle","a":[0,0,0],"b":[1,0,0],"c":[0,1,0]})"), material);
        require(triangle->intersect({{.2, .2, 1}, {0, 0, -1}}, epsilon, infinity, h),
                "triangle missed");
        close(h.t, 1);
#endif
        std::cout << "All numerical, interface and parser tests passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
