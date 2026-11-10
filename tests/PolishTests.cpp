#include "Test.h"
#include "world/VegetationSystem.h"
#include "world/CloudSystem.h"
#include "world/TerrainSystem.h"
#include "world/WorldConfig.h"
#include "camera/ChaseCamera.h"
#include "aircraft/Aircraft.h"
#include "effects/SpeedEffects.h"
#include "audio/AudioModel.h"
#include "ui/HUD.h"
#include <algorithm>
#include <cmath>

using namespace sky;

int main() {
    Tests t;

    // =========================================================================
    // 1. VegetationSystem: Determinism, Biome Constraints, Exclusions, Caps
    // =========================================================================
    TerrainSystem terrain;
    terrain.initialize();
    const auto& biomes = terrain.biomes();

    VegetationSystem veg1(1337);
    veg1.build(terrain, biomes);

    VegetationSystem veg2(1337);
    veg2.build(terrain, biomes);

    // Determinism with identical seed
    t.check(veg1.grassClumpCount() == veg2.grassClumpCount() && veg1.grassClumpCount() > 0,
            "vegetation grass clump determinism");
    t.check(veg1.purpleFlowerCount() == veg2.purpleFlowerCount() && veg1.purpleFlowerCount() > 0,
            "vegetation purple flower determinism");
    t.check(veg1.pinkFlowerCount() == veg2.pinkFlowerCount() && veg1.pinkFlowerCount() > 0,
            "vegetation pink flower determinism");

    t.check(veg1.grassBatch().vertices.size() == veg2.grassBatch().vertices.size(),
            "vegetation grass vertex count determinism");
    bool vegPositionsMatch = true;
    for (size_t i = 0; i < veg1.grassPositions().size(); ++i) {
        if (length(veg1.grassPositions()[i] - veg2.grassPositions()[i]) > 1e-8) {
            vegPositionsMatch = false;
            break;
        }
    }
    t.check(vegPositionsMatch, "vegetation identical clump positions with same seed");

    // Different seed produces different scatter
    VegetationSystem vegDiff(9999);
    vegDiff.build(terrain, biomes);
    t.check(vegDiff.grassPositions()[0].x != veg1.grassPositions()[0].x,
            "vegetation different seed produces different positions");

    // Batch density caps
    t.check(veg1.grassClumpCount() <= size_t(WorldConfig::maxGrassClumps),
            "vegetation grass count within WorldConfig cap");
    t.check(veg1.purpleFlowerCount() <= size_t(WorldConfig::maxPurpleFlowerTufts),
            "vegetation purple flower count within WorldConfig cap");
    t.check(veg1.pinkFlowerCount() <= size_t(WorldConfig::maxPinkFlowerTufts),
            "vegetation pink flower count within WorldConfig cap");

    // Merged batch geometry topology (each clump has 8 vertices and 4 triangles)
    t.check(veg1.grassBatch().vertices.size() == veg1.grassClumpCount() * 8,
            "vegetation grass batch 8 vertices per clump");
    t.check(veg1.grassBatch().triangles.size() == veg1.grassClumpCount() * 4,
            "vegetation grass batch 4 triangles per clump");
    t.check(veg1.purpleFlowersBatch().vertices.size() == veg1.purpleFlowerCount() * 8,
            "vegetation purple flowers batch 8 vertices per tuft");
    t.check(veg1.pinkFlowersBatch().vertices.size() == veg1.pinkFlowerCount() * 8,
            "vegetation pink flowers batch 8 vertices per tuft");

    // Biome constraint: Purple flower tufts ONLY inside western purple flower field (X < 0)
    bool purpleConstraintOk = true;
    for (const auto& pos : veg1.purpleFlowerPositions()) {
        if (pos.x >= 0.0) { purpleConstraintOk = false; break; }
        double gh = terrain.groundHeight(pos.x, pos.z);
        if (!near(pos.y, gh, 1e-3)) { purpleConstraintOk = false; break; }

        double dx = terrain.groundHeight(pos.x + 1.0, pos.z) - terrain.groundHeight(pos.x - 1.0, pos.z);
        double dz = terrain.groundHeight(pos.x, pos.z + 1.0) - terrain.groundHeight(pos.x, pos.z - 1.0);
        Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});
        auto b = biomes.classifyBiome(pos.x, pos.z, pos.y, normal);
        if (b != BiomeType::PurpleFlowerField) { purpleConstraintOk = false; break; }
    }
    t.check(purpleConstraintOk, "purple flower tufts ONLY inside purple flower field (west, X < 0)");

    // Biome constraint: Pink flower tufts ONLY inside eastern pink flower field (X > 0)
    bool pinkConstraintOk = true;
    for (const auto& pos : veg1.pinkFlowerPositions()) {
        if (pos.x <= 0.0) { pinkConstraintOk = false; break; }
        double gh = terrain.groundHeight(pos.x, pos.z);
        if (!near(pos.y, gh, 1e-3)) { pinkConstraintOk = false; break; }

        double dx = terrain.groundHeight(pos.x + 1.0, pos.z) - terrain.groundHeight(pos.x - 1.0, pos.z);
        double dz = terrain.groundHeight(pos.x, pos.z + 1.0) - terrain.groundHeight(pos.x, pos.z - 1.0);
        Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});
        auto b = biomes.classifyBiome(pos.x, pos.z, pos.y, normal);
        if (b != BiomeType::PinkFlowerField) { pinkConstraintOk = false; break; }
    }
    t.check(pinkConstraintOk, "pink flower tufts ONLY inside pink flower field (east, X > 0)");

    // Exclusions: None on rock, steep slopes, or valley floor path
    bool exclusionsOk = true;
    auto checkExclusions = [&](const std::vector<Vec3>& positions) {
        for (const auto& pos : positions) {
            double gh = terrain.groundHeight(pos.x, pos.z);
            if (!near(pos.y, gh, 1e-3)) exclusionsOk = false;

            double dx = terrain.groundHeight(pos.x + 1.0, pos.z) - terrain.groundHeight(pos.x - 1.0, pos.z);
            double dz = terrain.groundHeight(pos.x, pos.z + 1.0) - terrain.groundHeight(pos.x, pos.z - 1.0);
            Vec3 normal = normalized(Vec3{-dx, 2.0, -dz});

            if (normal.y < 0.85) exclusionsOk = false; // Steep slope exclusion
            if (pos.y > 85.0) exclusionsOk = false;    // Mountain rock zone

            auto w = biomes.evaluateWeights(pos.x, pos.z, pos.y, normal);
            if (w.mountainRock > 0.05) exclusionsOk = false;

            double valleyMask = TerrainSystem::evaluateValleyMask(pos.x, pos.z);
            if (w.valleyFloor > 0.05 || valleyMask > 0.05) exclusionsOk = false; // Valley path exclusion
        }
    };
    checkExclusions(veg1.grassPositions());
    checkExclusions(veg1.purpleFlowerPositions());
    checkExclusions(veg1.pinkFlowerPositions());
    t.check(exclusionsOk, "vegetation exclusion: none on rock, steep slopes, or valley floor path");

    // =========================================================================
    // 2. CloudSystem: Determinism, Altitude Band, Drift & Wrap
    // =========================================================================
    CloudSystem clouds1(1337);
    clouds1.initialize();
    CloudSystem clouds2(1337);
    clouds2.initialize();

    t.check(clouds1.cloudCount() == clouds2.cloudCount() &&
            clouds1.cloudCount() == size_t(WorldConfig::numCloudClusters),
            "cloud cluster count determinism");
    t.check(clouds1.mesh().vertices.size() == clouds2.mesh().vertices.size() &&
            clouds1.mesh().vertices.size() > 0,
            "cloud mesh vertex count determinism");

    // Altitude band check: all clusters strictly within [minAltitude, maxAltitude]
    bool cloudsAltitudeOk = true;
    for (const auto& cl : clouds1.clusters()) {
        if (cl.position.y < WorldConfig::cloudMinAltitude ||
            cl.position.y > WorldConfig::cloudMaxAltitude) {
            cloudsAltitudeOk = false;
            break;
        }
    }
    t.check(cloudsAltitudeOk, "cloud clusters strictly within altitude band [230m, 360m]");

    // Drift check: drift advances position.x with time delta
    double initialX0 = clouds1.clusters()[0].position.x;
    clouds1.update(2.0); // 2 seconds * 2.5 m/s = 5.0 m
    t.check(near(clouds1.clusters()[0].position.x, initialX0 + 5.0),
            "cloud slow drift rate matches cloudDriftSpeed");

    // Seamless wrap-around check: crossing 1200m wraps to -1200m
    CloudSystem cloudWrap(1337);
    cloudWrap.initialize();
    // Advance enough time to trigger wrap for any positive X cluster
    cloudWrap.update(1000.0);
    bool allWithinBounds = true;
    for (const auto& cl : cloudWrap.clusters()) {
        if (cl.position.x < -1250.0 || cl.position.x > 1250.0) {
            allWithinBounds = false;
            break;
        }
    }
    t.check(allWithinBounds, "cloud seamless wrap-around maintains map bounds");

    // =========================================================================
    // 3. Chase Camera Polish: Positional Lag, Camera Roll Cap, FOV Mapping
    // =========================================================================
    Aircraft aStraight;
    aStraight.reset();
    aStraight.visualBank = 0.0;

    Aircraft aTurnRight;
    aTurnRight.reset();
    aTurnRight.visualBank = -0.6; // Banking right

    Aircraft aTurnLeft;
    aTurnLeft.reset();
    aTurnLeft.visualBank = 0.6; // Banking left

    ChaseCamera cam;
    auto poseStraight = cam.desired(aStraight);
    auto poseRight = cam.desired(aTurnRight);
    auto poseLeft = cam.desired(aTurnLeft);

    // Positional lag: right bank pushes camera laterally to the right (+X), left pushes to -X
    t.check(poseRight.position.x > poseStraight.position.x,
            "camera positional turn lag: right bank displaces camera right");
    t.check(poseLeft.position.x < poseStraight.position.x,
            "camera positional turn lag: left bank displaces camera left");

    // Camera roll capped to cameraMaxRoll (12 degrees = 0.2094 rad)
    Aircraft aHeavyBank;
    aHeavyBank.reset();
    aHeavyBank.visualBank = -1.2; // ~68 degrees visual bank
    cam.reset(aHeavyBank);
    for (int i = 0; i < 200; ++i) cam.update(aHeavyBank, 1.0 / 60.0);

    Vec3 viewHeavy = normalized(cam.pose().target - cam.pose().position);
    Vec3 unrolledUpHeavy = normalized(aHeavyBank.transform.up() - viewHeavy * dot(aHeavyBank.transform.up(), viewHeavy));
    double rollAngleHeavy = std::acos(std::clamp(dot(cam.pose().up, unrolledUpHeavy), -1.0, 1.0));
    t.check(rollAngleHeavy <= WorldConfig::cameraMaxRoll + 1e-4,
            "camera roll fraction capped at WorldConfig::cameraMaxRoll (12 degrees)");

    // Moderate bank: camera roll = 20% of aircraft visual bank
    Aircraft aModerateBank;
    aModerateBank.reset();
    aModerateBank.visualBank = -0.3;
    cam.reset(aModerateBank);
    for (int i = 0; i < 200; ++i) cam.update(aModerateBank, 1.0 / 60.0);
    Vec3 viewMod = normalized(cam.pose().target - cam.pose().position);
    Vec3 unrolledUpMod = normalized(aModerateBank.transform.up() - viewMod * dot(aModerateBank.transform.up(), viewMod));
    double rollAngleMod = std::acos(std::clamp(dot(cam.pose().up, unrolledUpMod), -1.0, 1.0));
    t.check(near(rollAngleMod, 0.3 * WorldConfig::cameraRollFraction, 1e-3),
            "camera roll is 20% of aircraft visual bank");

    // Camera FOV smoothly mapped 65-80 degrees across speed range [25, 140] m/s
    Aircraft aSpeedMin; aSpeedMin.reset(); aSpeedMin.speed = 25.0;
    Aircraft aSpeedMid; aSpeedMid.reset(); aSpeedMid.speed = 70.0;
    Aircraft aSpeedMax; aSpeedMax.reset(); aSpeedMax.speed = 140.0;
    auto poseSpeedMin = cam.desired(aSpeedMin);
    auto poseSpeedMid = cam.desired(aSpeedMid);
    auto poseSpeedMax = cam.desired(aSpeedMax);

    t.check(near(poseSpeedMin.fovDegrees, 65.0), "camera FOV at min speed (25 m/s) is 65 degrees");
    t.check(poseSpeedMid.fovDegrees > 65.0 && poseSpeedMid.fovDegrees < 80.0,
            "camera FOV at cruise speed (70 m/s) is intermediate");
    t.check(near(poseSpeedMax.fovDegrees, 80.0), "camera FOV at max speed (140 m/s) is 80 degrees");
    t.check(poseSpeedMin.fovDegrees < poseSpeedMid.fovDegrees &&
            poseSpeedMid.fovDegrees < poseSpeedMax.fovDegrees,
            "camera FOV is strictly monotonic across speed range");

    // =========================================================================
    // 4. SpeedEffects: Threshold, Ramp, Config Toggle
    // =========================================================================
    SpeedEffects fx;
    t.check(near(fx.evaluateIntensity(25.0), 0.0), "speed-lines zero at min speed");
    t.check(near(fx.evaluateIntensity(70.0), 0.0), "speed-lines zero at cruise speed");
    t.check(near(fx.evaluateIntensity(WorldConfig::speedLineThreshold), 0.0),
            "speed-lines zero at threshold (105 m/s)");
    t.check(fx.evaluateIntensity(120.0) > 0.0, "speed-lines positive above threshold");
    t.check(near(fx.evaluateIntensity(140.0), 1.0), "speed-lines 1.0 at max speed (140 m/s)");
    t.check(fx.evaluateIntensity(120.0, true) > fx.evaluateIntensity(120.0, false),
            "speed-lines boost increases intensity");

    // Config toggle
    fx.setEnabled(false);
    t.check(near(fx.evaluateIntensity(140.0, true), 0.0),
            "speed-lines toggle disabled suppresses intensity completely");
    fx.setEnabled(true);

    // =========================================================================
    // 5. AudioModel: Monotonic Mapping & Clamping
    // =========================================================================
    AudioModel audio;
    bool audioMonotonic = true;
    double lastVol = -1.0;
    double lastPitch = -1.0;
    double lastWind = -1.0;

    for (double sp = 25.0; sp <= 140.0; sp += 5.0) {
        auto p = audio.evaluate(sp, 0.0, false);
        if (p.engineVolume < lastVol - 1e-8 ||
            p.enginePitch < lastPitch - 1e-8 ||
            p.windVolume < lastWind - 1e-8) {
            audioMonotonic = false;
            break;
        }
        lastVol = p.engineVolume;
        lastPitch = p.enginePitch;
        lastWind = p.windVolume;
    }
    t.check(audioMonotonic, "audio parameters monotonic non-decreasing with speed");

    // Audio clamping under extreme conditions
    auto extremeLow = audio.evaluate(-50.0, -2.0, false);
    t.check(extremeLow.engineVolume >= 0.0 && extremeLow.engineVolume <= 1.0 &&
            extremeLow.enginePitch >= 0.5 && extremeLow.enginePitch <= 2.0 &&
            extremeLow.windVolume >= 0.0 && extremeLow.windVolume <= 1.0,
            "audio parameters clamped under extreme negative speed");

    auto extremeHigh = audio.evaluate(400.0, 2.0, true);
    t.check(extremeHigh.engineVolume <= 1.0 && extremeHigh.enginePitch <= 2.0 &&
            extremeHigh.windVolume <= 1.0,
            "audio parameters clamped under extreme high speed and boost");

    // =========================================================================
    // 6. HUD: km/h Conversion & Clearance Warning States
    // =========================================================================
    t.check(near(HUD::toKmh(0.0), 0.0), "HUD km/h conversion at 0 m/s");
    t.check(near(HUD::toKmh(25.0), 90.0), "HUD km/h conversion at 25 m/s = 90 km/h");
    t.check(near(HUD::toKmh(70.0), 252.0), "HUD km/h conversion at 70 m/s = 252 km/h");
    t.check(near(HUD::toKmh(140.0), 504.0), "HUD km/h conversion at 140 m/s = 504 km/h");

    // Flight assist and clearance states
    Aircraft aStateNormal; aStateNormal.reset(); aStateNormal.clearance = 50.0;
    t.check(HUD::evaluateAssistState(aStateNormal) == FlightAssistState::Normal,
            "HUD assist state Normal for high clearance");

    Aircraft aStateAssist; aStateAssist.reset(); aStateAssist.terrainAssist = true; aStateAssist.clearance = 15.0;
    t.check(HUD::evaluateAssistState(aStateAssist) == FlightAssistState::AssistEngaged,
            "HUD assist state AssistEngaged for 8-25m descending");

    Aircraft aStateWarning; aStateWarning.reset(); aStateWarning.terrainWarning = true; aStateWarning.clearance = 5.0;
    t.check(HUD::evaluateAssistState(aStateWarning) == FlightAssistState::LowClearanceWarning,
            "HUD assist state LowClearanceWarning for clearance < 8m");

    Aircraft aStateCrashed; aStateCrashed.reset(); aStateCrashed.crashed = true;
    t.check(HUD::evaluateAssistState(aStateCrashed) == FlightAssistState::Crashed,
            "HUD assist state Crashed");

    // String formatting checks
    auto hudData = HUD::createData(aSpeedMid, 60.0, "Grassland");
    t.check(hudData.speedString.find("km/h") != std::string::npos,
            "HUD speedString includes km/h unit");
    t.check(hudData.altitudeString.find("m") != std::string::npos,
            "HUD altitudeString includes m unit");
    t.check(hudData.clearanceString.find("m") != std::string::npos,
            "HUD clearanceString includes m unit");
    t.check(hudData.fpsString.find("FPS") != std::string::npos,
            "HUD fpsString includes FPS");

    return t.finish();
}
