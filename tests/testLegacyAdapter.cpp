#include "rt/io/SceneAdapter.hpp"
#include "rt/io/detail/ConfigParser.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}

void near(double actual, double expected) {
    require(std::abs(actual - expected) < 1e-8, "unexpected imported scalar");
}

std::string read(const std::filesystem::path &path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("missing legacy test fixture: " + path.string());
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void rejects(const std::string &source) {
    bool failed = false;
    try {
        (void)rt::adaptScene(source);
    } catch (const std::exception &) {
        failed = true;
    }
    require(failed, "malformed legacy configuration was accepted");
}

void subjectFormat(const std::filesystem::path &fixtures) {
    const auto scene = rt::adaptScene(read(fixtures / "subject.cfg"));
    const auto &objects = scene.at("objects").array();
    require(objects.size() == 3, "libconfig object count changed");
    near(scene.at("render").number("width", 0), 1920);
    near(scene.at("render").number("height", 0), 1080);
    near(scene.at("camera").number("fov", 0), 72);
    const auto position = scene.at("camera").at("position").vector();
    const auto target = scene.at("camera").at("target").vector();
    near(position.y, -100);
    near(target.y, -99);
    near(scene.at("camera").at("up").vector().z, 1);
    near(objects[0].number("radius", 0), 25);
    near(objects[0].at("translate").vector().z, 40);
    near(objects[2].at("translate").vector().z, -20);
    const auto &red = scene.at("materials").at(objects[0].at("material").string());
    near(red.at("color").vector().x, 1);
    near(red.at("color").vector().y, 64. / 255.);
    require(scene.at("lights").array().size() == 2, "libconfig light count changed");
    near(scene.at("lights").array()[1].number("intensity", 0), .6);
}

void originalFiles(const std::filesystem::path &fixtures) {
    const std::pair<const char *, std::size_t> examples[] = {
        {"2Sphere_room.txt", 14},   {"3sphere_room.txt", 13}, {"Hollow_Purple.txt", 16},
        {"Room_AllSphere.txt", 10}, {"Sphere_room.txt", 13},  {"example_config.txt", 10}};
    for (const auto &[filename, count] : examples) {
        const auto scene = rt::adaptScene(read(fixtures / "original" / filename));
        require(scene.at("objects").array().size() == count, filename);
        near(scene.at("camera").at("target").vector().z, 0);
        require(!scene.at("materials").object().empty(), "legacy materials missing");
    }
    // These are the genuinely invalid examples in the original repository.
    rejects(read(fixtures / "original/config.txt"));
    rejects(read(fixtures / "original/final.txt"));
    const auto hollow = rt::adaptScene(read(fixtures / "original/Hollow_Purple.txt"));
    bool glass = false;
    for (const auto &[name, material] : hollow.at("materials").object()) {
        (void)name;
        if (material.number("transmission", 0) > .9)
            glass = true;
    }
    require(glass, "legacy dielectric was not preserved");
    const auto example = rt::adaptScene(read(fixtures / "original/example_config.txt"));
    bool hdr = false;
    for (const auto &[name, material] : example.at("materials").object()) {
        (void)name;
        if (material.has("emission") && material.at("emission").vector().x > 1)
            hdr = true;
    }
    require(hdr, "legacy HDR emission was clamped");
}

void syntaxCases() {
    const auto json = rt::adaptScene(" {\"camera\": {\"fov\": 37}, \"unchanged\": true}");
    near(json.at("camera").number("fov", 0), 37);
    require(json.has("unchanged"), "modern JSON was rewritten");
    for (const auto &source :
         {"", "camera={", "camera={a=1e999;};", "/* never closed", "camera={s=\"broken};",
          "camera={}; camera={};", "camera={}; primitives={unknown=();};",
          "camera={}; primitives={}; typo={};"})
        rejects(source);
    const std::string source = R"(
        // Lists and arrays can contain trailing separators.
        camera={resolution={width=64;height=32;};position=[0,-5,2];rotation=[0,0,0];};
        primitives={spheres=({x=0;y=0;z=0;r=1;color={r=1;g=0;b=0;};},);};
        lights={ambient=.3;directional=({direction=[1,-1,-1];},);}; /* done */
    )";
    const auto scene = rt::adaptScene(source);
    require(scene.at("objects").array().size() == 1, "mixed config syntax failed");
    require(scene.at("lights").array().size() == 2, "directional import failed");
    rejects(std::string(140, '{'));
    rejects("camera={};primitives={spheres=({height=3;});};");
    rejects("camera={};primitives={spheres=({raduis=999;});};");
    rejects("camera={position={x=0;y=-5;z=2;xTypo=99;};};primitives={spheres=({r=1;});};");
    const std::string cylinder = R"(
        camera { camera_position {X:0,Y:0,Z:5}, camera_direction {X:0,Y:0,Z:0}, }
        primitives { cylinder {
            position {X:0,Y:0,Z:0,X2:3,Y2:4,Z2:0}, radius:1,
            material {color {r:1,g:0,b:0}, material_type:0,}
        }}
    )";
    const auto imported = rt::adaptScene(cylinder);
    require(imported.at("objects").array()[0].at("type").string() == "limited_cylinder",
            "legacy endpoints were not converted to a finite cylinder");
    near(imported.at("objects").array()[0].number("height", 0), 5);
    auto malformedCylinder = cylinder;
    malformedCylinder.replace(malformedCylinder.find("X2:3"), 4, "X2:\"oops\"");
    rejects(malformedCylinder);
    std::string excessive = "values=(";
    for (std::size_t i = 0; i < 250001; ++i)
        excessive += "0,";
    excessive += ");";
    bool bounded = false;
    try {
        (void)rt::io::ConfigParser(excessive).parse();
    } catch (const std::exception &error) {
        bounded = std::string(error.what()).find("250000 nodes") != std::string::npos;
    }
    require(bounded, "config AST allocation is not bounded");
}

} // namespace

int main(int argc, char **argv) {
    try {
        require(argc == 2, "usage: legacy_adapter_tests <fixtures>");
        subjectFormat(argv[1]);
        originalFiles(argv[1]);
        syntaxCases();
        std::cout << "Legacy originals, libconfig specification and parser errors passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
