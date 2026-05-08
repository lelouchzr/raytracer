#include "rt/pbr/Bsdf.hpp"
#include "rt/Extensions.hpp"

namespace rt {
Bsdf::Bsdf(Surface material, Vec3 normal, double etaI, double etaT)
    : material_(material), normal_(normal),
      tangent_(unit(cross(std::abs(normal.y) < .9 ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, normal))),
      bitangent_(cross(normal, tangent_)), etaI_(etaI), etaT_(etaT),
      alpha_(std::max(.001, material.roughness * material.roughness)) {
    material_.color = {std::clamp(material_.color.x, 0., 1.), std::clamp(material_.color.y, 0., 1.),
                       std::clamp(material_.color.z, 0., 1.)};
}

Vec3 Bsdf::local(Vec3 direction) const {
    return {dot(direction, tangent_), dot(direction, bitangent_), dot(direction, normal_)};
}

Vec3 Bsdf::world(Vec3 direction) const {
    return tangent_ * direction.x + bitangent_ * direction.y + normal_ * direction.z;
}

double Bsdf::dielectricFresnel(double cosine, double etaI, double etaT) {
    if (etaI == etaT)
        return 0;
    cosine = std::clamp(std::abs(cosine), 0., 1.);
    double sine2 = std::pow(etaI / etaT, 2) * (1 - cosine * cosine);
    if (sine2 >= 1)
        return 1;
    double transmitted = std::sqrt(1 - sine2);
    double rs = (etaI * cosine - etaT * transmitted) / (etaI * cosine + etaT * transmitted);
    double rp = (etaT * cosine - etaI * transmitted) / (etaT * cosine + etaI * transmitted);
    return (rs * rs + rp * rp) * .5;
}

Vec3 Bsdf::fresnel(double cosine) const {
    double f = dielectricFresnel(cosine, etaI_, etaT_);
    double grazing = std::pow(1 - std::clamp(std::abs(cosine), 0., 1.), 5);
    Vec3 metal = material_.color + (Vec3{1, 1, 1} - material_.color) * grazing;
    return Vec3{f, f, f} * (1 - material_.metallic) + metal * material_.metallic;
}

double Bsdf::distribution(Vec3 m) const {
    if (m.z <= 0)
        return 0;
    double denominator = m.z * m.z * (alpha_ * alpha_ - 1) + 1;
    return alpha_ * alpha_ / (pi * denominator * denominator);
}

double Bsdf::masking(Vec3 v) const {
    if (std::abs(v.z) < 1e-12)
        return 0;
    double tan2 = (v.x * v.x + v.y * v.y) / (v.z * v.z);
    return 2 / (1 + std::sqrt(1 + alpha_ * alpha_ * tan2));
}

Vec3 Bsdf::microfacet(Vec3 outgoing, unsigned long long &rng) const {
    // Isotropic GGX visible-normal sampling: stretch, sample the projected disk, unstretch.
    Vec3 view = unit({alpha_ * outgoing.x, alpha_ * outgoing.y, outgoing.z});
    double length = view.x * view.x + view.y * view.y;
    Vec3 t1 = length > 1e-20 ? Vec3{-view.y, view.x, 0} / std::sqrt(length) : Vec3{1, 0, 0};
    Vec3 t2 = cross(view, t1);
    double radius = std::sqrt(random01(rng)), angle = 2 * pi * random01(rng);
    double x = radius * std::cos(angle), y = radius * std::sin(angle), blend = .5 * (1 + view.z);
    y = (1 - blend) * std::sqrt(std::max(0., 1 - x * x)) + blend * y;
    Vec3 projected = t1 * x + t2 * y + view * std::sqrt(std::max(0., 1 - x * x - y * y));
    return unit({alpha_ * projected.x, alpha_ * projected.y, std::max(1e-12, projected.z)});
}

double Bsdf::diffuseProbability() const {
    return .5 * (1 - material_.metallic) * (1 - material_.transmission);
}

double Bsdf::reflectionProbability(Vec3 outgoing, Vec3 micro) const {
    Vec3 f = fresnel(dot(outgoing, micro));
    double reflection = (f.x + f.y + f.z) / 3;
    double transmission = (1 - material_.metallic) * material_.transmission *
                          (1 - dielectricFresnel(dot(outgoing, micro), etaI_, etaT_));
    return reflection + transmission > 0 ? reflection / (reflection + transmission) : 1;
}

Vec3 Bsdf::evaluate(Vec3 outgoing, Vec3 incoming) const {
    if (std::abs(etaI_ - etaT_) < 1e-12 && material_.transmission > 0) {
        double straight = (1 - material_.metallic) * material_.transmission;
        if (straight >= 1)
            return {};
        auto residual = material_;
        residual.transmission = 0;
        residual.metallic /= 1 - straight;
        return Bsdf(residual, normal_, etaI_, etaT_).evaluate(outgoing, incoming) * (1 - straight);
    }
    Vec3 wo = local(outgoing), wi = local(incoming);
    if (wo.z <= 1e-10 || std::abs(wi.z) <= 1e-10)
        return {};
    bool reflection = wi.z > 0;
    Vec3 result;
    if (reflection) {
        double fo = dielectricFresnel(wo.z, etaI_, etaT_),
               fi = dielectricFresnel(wi.z, etaI_, etaT_);
        result = material_.color * ((1 - material_.metallic) * (1 - material_.transmission) *
                                    (1 - fo) * (1 - fi) / pi);
    }
    if (material_.roughness < .001)
        return result;
    double eta = etaT_ / etaI_;
    Vec3 sum = reflection ? wo + wi : wo + wi * eta;
    if (sum.length2() < 1e-20)
        return result;
    Vec3 micro = unit(sum);
    if (micro.z < 0)
        micro = -micro;
    double om = dot(wo, micro), im = dot(wi, micro);
    if (om <= 0 || im * wi.z <= 0)
        return result;
    double dg = distribution(micro) * masking(wo) * masking(wi);
    if (reflection)
        return result + fresnel(om) * (dg / (4 * wo.z * wi.z));
    double denominator = im + om / eta;
    if (std::abs(denominator) < 1e-12)
        return {};
    double factor = (1 - material_.metallic) * material_.transmission *
                    (1 - dielectricFresnel(om, etaI_, etaT_)) * dg * std::abs(om * im) /
                    (std::abs(wo.z * wi.z) * denominator * denominator * eta * eta);
    return {factor, factor, factor};
}

double Bsdf::pdf(Vec3 outgoing, Vec3 incoming) const {
    if (std::abs(etaI_ - etaT_) < 1e-12 && material_.transmission > 0) {
        double straight = (1 - material_.metallic) * material_.transmission;
        if (straight >= 1)
            return 0;
        auto residual = material_;
        residual.transmission = 0;
        residual.metallic /= 1 - straight;
        return Bsdf(residual, normal_, etaI_, etaT_).pdf(outgoing, incoming) * (1 - straight);
    }
    Vec3 wo = local(outgoing), wi = local(incoming);
    if (wo.z <= 1e-10 || std::abs(wi.z) <= 1e-10)
        return 0;
    bool reflection = wi.z > 0;
    double diffuse = diffuseProbability(), result = reflection ? diffuse * wi.z / pi : 0;
    if (material_.roughness < .001)
        return result;
    double eta = etaT_ / etaI_;
    Vec3 sum = reflection ? wo + wi : wo + wi * eta;
    if (sum.length2() < 1e-20)
        return result;
    Vec3 micro = unit(sum);
    if (micro.z < 0)
        micro = -micro;
    double om = dot(wo, micro), im = dot(wi, micro);
    if (om <= 0 || im * wi.z <= 0)
        return result;
    double density = distribution(micro) * masking(wo) * om / wo.z;
    double pr = reflectionProbability(wo, micro);
    if (reflection)
        return result + (1 - diffuse) * pr * density / (4 * om);
    double denominator = im + om / eta;
    if (std::abs(denominator) < 1e-12)
        return 0;
    return (1 - diffuse) * (1 - pr) * density * std::abs(im) / (denominator * denominator);
}

BsdfSample Bsdf::sample(Vec3 outgoing, unsigned long long &rng) const {
    if (std::abs(etaI_ - etaT_) < 1e-12 && material_.transmission > 0) {
        double straight = (1 - material_.metallic) * material_.transmission;
        if (random01(rng) < straight)
            return {-outgoing, {1, 1, 1}, straight, true, true};
        if (straight >= 1)
            return {};
        auto residual = material_;
        residual.transmission = 0;
        residual.metallic /= 1 - straight;
        auto sample = Bsdf(residual, normal_, etaI_, etaT_).sample(outgoing, rng);
        sample.pdf *= 1 - straight;
        return sample;
    }
    Vec3 wo = local(outgoing);
    if (wo.z <= 0)
        return {};
    double diffuse = diffuseProbability();
    Vec3 incoming;
    bool delta = false, transmitted = false;
    double probability = 0;
    Vec3 weight;
    if (random01(rng) < diffuse)
        incoming = hemisphere(normal_, rng);
    else {
        Vec3 micro = material_.roughness < .001 ? Vec3{0, 0, 1} : microfacet(wo, rng);
        double pr = reflectionProbability(wo, micro), cosine = dot(wo, micro);
        if (random01(rng) < pr) {
            incoming = world(reflect(-wo, micro));
            if (dot(incoming, normal_) <= 0)
                return {};
            probability = (1 - diffuse) * pr;
            weight = fresnel(cosine);
        } else {
            double eta = etaI_ / etaT_, sine2 = eta * eta * (1 - cosine * cosine);
            if (sine2 >= 1)
                return {};
            incoming = world(-wo * eta + micro * (eta * cosine - std::sqrt(1 - sine2)));
            if (dot(incoming, normal_) >= 0)
                return {};
            transmitted = true;
            probability = (1 - diffuse) * (1 - pr);
            double value = (1 - material_.metallic) * material_.transmission *
                           (1 - dielectricFresnel(cosine, etaI_, etaT_)) * eta * eta;
            weight = {value, value, value};
        }
        delta = material_.roughness < .001;
    }
    if (delta)
        return {unit(incoming), weight / probability, probability, true, transmitted};
    double density = pdf(outgoing, incoming);
    if (density <= 0)
        return {};
    return {unit(incoming),
            evaluate(outgoing, incoming) * (std::abs(dot(normal_, incoming)) / density), density,
            false, transmitted};
}
} // namespace rt
