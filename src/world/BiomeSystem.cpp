#include "BiomeSystem.h"
#include "TerrainSystem.h"
#include <algorithm>
#include <cmath>

namespace sky {

namespace {

inline double smoothstep(double e0, double e1, double x) {
    if (e0 == e1) return x < e0 ? 0.0 : 1.0;
    double t = std::clamp((x - e0) / (e1 - e0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

} // namespace

BiomeWeights BiomeSystem::evaluateWeights(double x, double z, double y, const Vec3& normal) const {
    BiomeWeights w;

    // 1. Mountain Rock weight (altitude and slope based)
    double slope = std::clamp((0.85 - normal.y) / 0.30, 0.0, 1.0); // 0 on flat ground, 1 on steep cliffs
    double altRock = smoothstep(85.0, 150.0, y);
    w.mountainRock = std::clamp(altRock + slope * 0.75, 0.0, 1.0);

    // 2. Valley Floor weight (south winding canyon floor)
    double valleyMask = TerrainSystem::evaluateValleyMask(x, z);
    double valleyFloorFactor = smoothstep(22.0, 6.0, y);
    w.valleyFloor = valleyMask * valleyFloorFactor * (1.0 - w.mountainRock);

    // 3. Flower fields in West (purple) and East (pink)
    // Lateral presence with gentle transition starting near +-120m, full by +-380m
    double lateralWest = smoothstep(-120.0, -380.0, x);
    double lateralEast = smoothstep(120.0, 380.0, x);
    // Flowers stay in mid-latitudes (not extreme north mountains or deep south canyon)
    double midLatitude = smoothstep(650.0, 350.0, z) * smoothstep(-650.0, -250.0, z);
    // Flowers do not grow on high mountains, cliffs, or valley riverbeds
    double flowerAltitude = smoothstep(20.0, 30.0, y) * smoothstep(100.0, 65.0, y);
    double flowerMask = midLatitude * flowerAltitude * (1.0 - slope);

    w.purpleFlower = lateralWest * flowerMask * (1.0 - w.mountainRock) * (1.0 - w.valleyFloor);
    w.pinkFlower = lateralEast * flowerMask * (1.0 - w.mountainRock) * (1.0 - w.valleyFloor);

    // 4. Highland weight (elevated meadows and rolling hills between 35m and 90m)
    double altHighland = smoothstep(32.0, 65.0, y) * smoothstep(110.0, 80.0, y);
    w.highland = altHighland * (1.0 - w.mountainRock) * (1.0 - w.purpleFlower) * (1.0 - w.pinkFlower) * (1.0 - w.valleyFloor);

    // 5. Base Grassland weight (fills central airfield and low rolling meadows)
    double nonGrass = w.mountainRock + w.valleyFloor + w.purpleFlower + w.pinkFlower + w.highland;
    w.grassland = std::max(0.0, 1.0 - nonGrass);

    // Normalize weights to sum to 1.0
    double total = w.grassland + w.purpleFlower + w.pinkFlower + w.highland + w.mountainRock + w.valleyFloor;
    if (total > 1e-9) {
        w.grassland /= total;
        w.purpleFlower /= total;
        w.pinkFlower /= total;
        w.highland /= total;
        w.mountainRock /= total;
        w.valleyFloor /= total;
    } else {
        w.grassland = 1.0;
    }

    return w;
}

BiomeType BiomeSystem::classifyBiome(double x, double z, double y, const Vec3& normal) const {
    auto w = evaluateWeights(x, z, y, normal);
    BiomeType best = BiomeType::Grassland;
    double maxW = w.grassland;

    if (w.purpleFlower > maxW) { maxW = w.purpleFlower; best = BiomeType::PurpleFlowerField; }
    if (w.pinkFlower > maxW) { maxW = w.pinkFlower; best = BiomeType::PinkFlowerField; }
    if (w.highland > maxW) { maxW = w.highland; best = BiomeType::Highland; }
    if (w.mountainRock > maxW) { maxW = w.mountainRock; best = BiomeType::MountainRock; }
    if (w.valleyFloor > maxW) { best = BiomeType::ValleyFloor; }

    return best;
}

Vec3 BiomeSystem::evaluateColor(double x, double z, double y, const Vec3& normal) const {
    auto w = evaluateWeights(x, z, y, normal);

    // High mountain peaks fade toward snowy granite
    double snowFactor = smoothstep(140.0, 180.0, y);
    Vec3 rockCol = lerp(colorMountainRock, colorPeakSnow, snowFactor);

    Vec3 blended = colorGrassland * w.grassland +
                   colorPurpleFlower * w.purpleFlower +
                   colorPinkFlower * w.pinkFlower +
                   colorHighland * w.highland +
                   rockCol * w.mountainRock +
                   colorValleyFloor * w.valleyFloor;

    return blended;
}

uint32_t BiomeSystem::evaluatePackedColor(double x, double z, double y, const Vec3& normal, const Vec3& lightDir) const {
    Vec3 base = evaluateColor(x, z, y, normal);

    // Directional lighting with soft ambient fill for low-poly clarity
    Vec3 l = normalized(lightDir);
    double diffuse = 0.58 + 0.42 * std::max(0.0, dot(normal, l));

    int r = std::clamp(int(base.x * diffuse * 255.0 + 0.5), 0, 255);
    int g = std::clamp(int(base.y * diffuse * 255.0 + 0.5), 0, 255);
    int b = std::clamp(int(base.z * diffuse * 255.0 + 0.5), 0, 255);

    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

std::string_view BiomeSystem::biomeName(BiomeType type) {
    switch (type) {
        case BiomeType::Grassland: return "Grassland";
        case BiomeType::PurpleFlowerField: return "PurpleFlowerField";
        case BiomeType::PinkFlowerField: return "PinkFlowerField";
        case BiomeType::Highland: return "Highland";
        case BiomeType::MountainRock: return "MountainRock";
        case BiomeType::ValleyFloor: return "ValleyFloor";
    }
    return "Unknown";
}

} // namespace sky
