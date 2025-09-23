#ifndef PERLIN_H
#define PERLIN_H

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>

class PerlinNoise {
public:
    // Constructor with optional seed
    PerlinNoise(unsigned int seed = std::random_device{}());

    // Get noise value at 2D coordinates
    double noise(double x, double y);

    // Get fractal noise (multiple octaves combined)
    double fractalNoise(double x, double y, int octaves = 4, double persistence = 0.5, double scale = 1.0);

private:
    std::vector<int> permutation;

    // Helper functions
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y);
};

#endif