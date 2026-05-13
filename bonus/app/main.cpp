#include "rt/studio/NativeStudio.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
    try {
        rt::NativeStudio::run(argc - 1, argv + 1);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "raytracer studio: " << error.what() << '\n';
        return 84;
    }
}
