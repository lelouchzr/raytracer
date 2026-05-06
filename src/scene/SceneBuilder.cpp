#ifdef RT_BONUS
#include "rt/acceleration/Composite.hpp"
#include "rt/io/SceneAdapter.hpp"
#include "rt/plugins/PluginLoader.hpp"
#include "rt/transform/AffineTransform.hpp"
#endif
#include "rt/engine.hpp"
#include <fstream>
#include <sstream>

namespace rt {
SceneBuilder::SceneBuilder(bool preview, bool deferLighting)
    : preview_(preview), deferLighting_(deferLighting) {
#ifdef RT_BONUS
    registerBonus(factory_);
#endif
}

void SceneBuilder::append(const std::filesystem::path &file,
                          std::vector<std::filesystem::path> &stack, bool root) {
    auto absolute = std::filesystem::weakly_canonical(file);
    if (stack.size() >= 32 || std::find(stack.begin(), stack.end(), absolute) != stack.end())
        throw std::runtime_error("scene import cycle/depth limit: " + file.string());
    std::ifstream input(absolute);
    if (!input)
        throw std::runtime_error("cannot read scene '" + file.string() + "'");
    input.seekg(0, std::ios::end);
    if (input.tellg() > 16 * 1024 * 1024)
        throw std::runtime_error("scene exceeds 16 MiB");
    input.seekg(0);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    Json j;
    try {
#ifdef RT_BONUS
        j = adaptScene(buffer.str());
#else
        j = Json::parse(buffer.str());
#endif
    } catch (const std::exception &e) {
        throw std::runtime_error(file.string() + ": " + e.what());
    }
    keys(j, {"camera", "render", "background", "materials", "objects", "lights"
#ifdef RT_BONUS
             ,
             "include", "plugins"
#endif
            });
    stack.push_back(absolute);
#ifdef RT_BONUS
    if (j.has("plugins"))
        for (const auto &plugin : j.at("plugins").array())
            PluginLoader::load(factory_, absolute.parent_path() / plugin.string());
#endif
#ifdef RT_BONUS
    if (j.has("include"))
        for (const auto &inc : j.at("include").array())
            append(absolute.parent_path() / inc.string(), stack, false);
#endif
    if (j.has("camera")) {
        auto &c = j.at("camera");
        keys(c, {"position", "target", "up", "fov"});
        scene_.camera = {c.vector("position", {0, 2, 7}), c.vector("target", {0, 1, 0}),
                         c.vector("up", {0, 1, 0}), c.number("fov", 45)};
    } else if (root)
        throw std::runtime_error("root scene requires camera");
    if (j.has("background"))
        scene_.background = colorValue(j.at("background").vector());
    if (j.has("render")) {
        const auto &r = j.at("render");
        keys(r, {"width", "height", "output"
#ifdef RT_BONUS
                 ,
                 "samples", "depth", "threads", "ao", "exposure", "integrator", "photons",
                 "photonRadius", "adaptive", "minSamples", "toneMap"
#endif
                });
        auto &s = scene_.settings;
        s.width = integer(r.number("width", 640), 1, 8192, "width");
        s.height = integer(r.number("height", 360), 1, 8192, "height");
        s.output = r.string("output", "output.ppm");
#ifdef RT_BONUS
        s.samples = integer(r.number("samples", 16), 1, 4096, "samples");
        s.depth = integer(r.number("depth", 6), 1, 24, "depth");
        s.threads = integer(r.number("threads", 0), 0, 256, "threads");
        s.ao = integer(r.number("ao", 0), 0, 128, "ao");
        s.exposure = range(r.number("exposure", 1), .01, 100, "exposure");
        s.toneMap = r.string("toneMap", "reinhard");
        if (s.toneMap != "reinhard" && s.toneMap != "none")
            throw std::runtime_error("toneMap must be reinhard or none");
        s.integrator = r.string("integrator", "whitted");
        if (s.integrator != "whitted" && s.integrator != "path" && s.integrator != "photon")
            throw std::runtime_error("integrator must be whitted, path or photon");
        s.photons = integer(r.number("photons", 0), 0, 2000000, "photons");
        s.photonRadius = range(r.number("photonRadius", .3), .001, 10, "photon radius");
        s.adaptive = range(r.number("adaptive", 0), 0, 1, "adaptive threshold");
        s.minSamples = integer(r.number("minSamples", 4), 2, 4096, "minimum adaptive samples");
#endif
        if (static_cast<long long>(s.width) * s.height > 32000000)
            throw std::runtime_error("image exceeds 32 million pixels");
        if (s.output.empty())
            throw std::runtime_error("output path is empty");
    }
    if (j.has("materials"))
        for (const auto &[name, m] : j.at("materials").object()) {
            if (materials_.count(name))
                throw std::runtime_error("duplicate material '" + name + "'");
            auto type = m.string("type", "flat");
            auto it = factory_.materials.find(type);
            if (it == factory_.materials.end())
                throw std::runtime_error("unknown material '" + type + "'");
            materials_[name] = it->second(m, absolute.parent_path());
        }
    if (j.has("objects")) {
        std::function<Primitive(const Json &)> object = [&](const Json &o) -> Primitive {
#ifdef RT_BONUS
            if (o.at("type").string() == "group") {
                keys(o, {"type", "children", "translate", "rotate", "scale", "shear", "matrix"});
                std::vector<Primitive> children;
                for (const auto &child : o.at("children").array())
                    children.push_back(object(child));
                return std::make_shared<AffineTransform>(
                    std::make_shared<Composite>(std::move(children)), Mat4::fromObject(o), false);
            }
#endif
            auto material = o.at("material").string();
            auto it = materials_.find(material);
            if (it == materials_.end())
                throw std::runtime_error("unknown material '" + material + "'");
#ifdef RT_BONUS
            if (o.at("type").string() == "obj")
                return makeMesh(o, it->second, absolute.parent_path());
#endif
            return factory_.primitive(o, it->second);
        };
        for (const auto &o : j.at("objects").array())
            scene_.objects.push_back(object(o));
    }
    if (j.has("lights"))
        for (const auto &l : j.at("lights").array()) {
            auto type = l.at("type").string();
            auto it = factory_.lights.find(type);
            if (it == factory_.lights.end())
                throw std::runtime_error("unknown light '" + type + "'");
            Json definition = l;
#ifdef RT_BONUS
            if (type == "point" && scene_.settings.integrator != "whitted" && !l.has("physical"))
                std::get<Json::Object>(definition.value)["physical"] = Json(true);
#endif
            scene_.lights.push_back(it->second(definition));
        }
    stack.pop_back();
}

Scene SceneBuilder::build(const std::filesystem::path &file) {
    scene_ = Scene{};
    materials_.clear();
    std::vector<std::filesystem::path> stack;
    append(file, stack, true);
    scene_.modules = factory_.modules;
    if (preview_) {
        auto &settings = scene_.settings;
        double factor = std::min(1., 320. / settings.width);
        settings.width = std::max(1, static_cast<int>(settings.width * factor));
        settings.height = std::max(1, static_cast<int>(settings.height * factor));
        settings.samples = 1;
        settings.ao = 0;
        settings.depth = 2;
        settings.integrator = "whitted";
        settings.photons = 0;
    }
    scene_.camera.validate();
    if (scene_.objects.empty())
        throw std::runtime_error("scene has no objects");
    scene_.integrator = basicIntegrator();
#ifdef RT_BONUS
    configureBonus(scene_, !deferLighting_);
#endif
    return std::move(scene_);
}
} // namespace rt
