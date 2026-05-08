#include "rt/materials/AdvancedMaterial.hpp"

namespace rt {
AdvancedMaterial::AdvancedMaterial(const Json &j, const std::filesystem::path &base) {
    keys(j, {"type", "color", "second", "pattern", "scale", "reflection", "transmission", "ior",
             "specular", "shininess", "emission", "file", "metallic", "normalFile",
             "normalStrength", "roughness", "absorption"});
    surface_.physical = j.string("type", "phong") == "pbr";
    surface_.roughness = range(j.number("roughness", .3), 0, 1, "roughness");
    surface_.absorption = j.vector("absorption", {});
    for (int channel = 0; channel < 3; ++channel)
        range(surface_.absorption[channel], 0, 1000, "absorption");
    surface_.color = colorValue(j.vector("color", {.7, .7, .7}));
    second_ = colorValue(j.vector("second", {.1, .1, .1}));
    pattern_ = j.string("pattern", "solid");
    scale_ = range(j.number("scale", 1), 1e-4, 1000, "texture scale");
    normalStrength_ = range(j.number("normalStrength", 1), 0, 10, "normal strength");
    surface_.reflection = range(j.number("reflection", 0), 0, 1, "reflection");
    surface_.transmission = range(j.number("transmission", 0), 0, 1, "transmission");
    if (surface_.reflection + surface_.transmission > 1)
        throw std::runtime_error("reflection + transmission exceeds 1");
    surface_.ior = range(j.number("ior", 1.5), 1, 3, "ior");
    surface_.metallic = range(j.number("metallic", 0), 0, 1, "metallic");
    surface_.specular = range(j.number("specular", .25), 0, 1, "specular");
    surface_.shininess = range(j.number("shininess", 64), 1, 2048, "shininess");
    surface_.emission = j.vector("emission", {});
    for (int i = 0; i < 3; ++i)
        range(surface_.emission[i], 0, 100, "emission");
    if (pattern_ != "solid" && pattern_ != "checker" && pattern_ != "marble" &&
        pattern_ != "perlin" && pattern_ != "image")
        throw std::runtime_error("unknown material pattern");
    if (pattern_ == "image")
        texture_ = std::make_shared<Image>(base / j.at("file").string());
    if (j.has("normalFile"))
        normalMap_ = std::make_shared<Image>(base / j.at("normalFile").string(), false);
}

Surface AdvancedMaterial::sample(Vec3 p) const {
    Surface s = surface_;
    p = p * scale_;
    if (pattern_ == "checker") {
        if (std::abs(std::fmod(std::floor(p.x) + std::floor(p.y) + std::floor(p.z), 2)) > .5)
            s.color = second_;
    } else if (pattern_ == "marble" || pattern_ == "perlin") {
        double n = perlin_.noise(p) + .5 * perlin_.noise(p * 2) + .25 * perlin_.noise(p * 4);
        double f = pattern_ == "marble" ? .5 + .5 * std::sin(p.x * 3 + n * 6)
                                        : std::clamp(.5 + n * .3, 0., 1.);
        s.color = s.color * f + second_ * (1 - f);
    } else if (texture_)
        s.color = s.color * texture_->sample(p.x, p.z);
    return s;
}

Surface AdvancedMaterial::sampleHit(const Hit &h) const {
    double footprint = h.footprint * h.textureScale * scale_;
    if (texture_) {
        Surface s = surface_;
        Vec3 coordinates = h.hasUV ? Vec3{h.uv.x, 1 - h.uv.y, 0} : Vec3{h.local.x, h.local.z, 0};
        s.color =
            s.color * texture_->sample(coordinates.x * scale_, coordinates.y * scale_, footprint);
        return s;
    }
    if (pattern_ == "checker" && footprint > 1e-8) {
        auto stripeIntegral = [](double coordinate) {
            coordinate -= 2 * std::floor(coordinate / 2);
            return coordinate <= 1 ? coordinate : 2 - coordinate;
        };
        Vec3 p = h.local * scale_;
        double product = 1;
        for (int axis = 0; axis < 3; ++axis)
            product *= (stripeIntegral(p[axis] + footprint * .5) -
                        stripeIntegral(p[axis] - footprint * .5)) /
                       footprint;
        double mix = std::clamp(.5 + .5 * product, 0., 1.);
        Surface s = surface_;
        s.color = surface_.color * mix + second_ * (1 - mix);
        return s;
    }
    return sample(h.local);
}

Vec3 AdvancedMaterial::shadingNormal(const Hit &h) const {
    if (!normalMap_)
        return h.normal;
    Vec3 p = h.hasUV ? Vec3{h.uv.x, 1 - h.uv.y, 0} : Vec3{h.local.x, h.local.z, 0};
    Vec3 n =
        normalMap_->sample(p.x * scale_, p.y * scale_, h.footprint * h.textureScale * scale_) * 2 -
        Vec3{1, 1, 1};
    n.x *= normalStrength_;
    n.y *= normalStrength_;
    Vec3 tangent = unit(h.tangent - h.normal * dot(h.tangent, h.normal));
    Vec3 bitangent = unit(cross(h.normal, tangent));
    if (dot(bitangent, h.bitangent) < 0)
        bitangent = -bitangent;
    Vec3 mapped = tangent * n.x + bitangent * n.y + h.normal * n.z;
    if (mapped.length2() < 1e-12)
        return h.normal;
    mapped = unit(mapped);
    return dot(mapped, h.geometric) < 0 ? -mapped : mapped;
}
} // namespace rt
