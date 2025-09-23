#include "perlin.h"

PerlinNoise::PerlinNoise(unsigned int seed) {
    // Create permutation table
    permutation.resize(512);

    // Fill with values 0-255
    std::vector<int> p(256);
    std::iota(p.begin(), p.end(), 0);

    // Shuffle using seed
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);

    // Duplicate for wrapping
    for (int i = 0; i < 256; i++) {
        permutation[i] = permutation[i + 256] = p[i];
    }
}

double PerlinNoise::noise(double x, double y) {
    // Find unit grid cell containing point
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;

    // Get relative xy coordinates within cell
    x -= floor(x);
    y -= floor(y);

    // Compute fade curves for x and y
    double u = fade(x);
    double v = fade(y);

    // Hash coordinates of 4 grid corners
    int A = permutation[X] + Y;
    int AA = permutation[A];
    int AB = permutation[A + 1];
    int B = permutation[X + 1] + Y;
    int BA = permutation[B];
    int BB = permutation[B + 1];

    // Blend results from 4 corners
    return lerp(v,
        lerp(u, grad(permutation[AA], x, y),
            grad(permutation[BA], x - 1, y)),
        lerp(u, grad(permutation[AB], x, y - 1),
            grad(permutation[BB], x - 1, y - 1))
    );
}

double PerlinNoise::fractalNoise(double x, double y, int octaves, double persistence, double scale) {
    double value = 0.0;
    double amplitude = 1.0;
    double frequency = scale;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; i++) {
        value += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }

    return value / maxValue; // Normalize to [-1, 1]
}

double PerlinNoise::fade(double t) {
    // Smoothstep function: 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y) {
    // Convert hash to one of 4 gradient directions
    int h = hash & 3;
    double u = h < 2 ? x : y;
    double v = h < 2 ? y : x;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}