#include "rt/io/ResourcePaths.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        std::cout << rt::resourceRoot().string() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 84;
    }
}
