#include "Test.h"
#include "world/TerrainSystem.h"
#include "world/WorldCollision.h"
#include "aircraft/FlightController.h"
#include "camera/ChaseCamera.h"
#include <cmath>

using namespace sky;

int main() {
    Tests t;

    // 1. Height Determinism
    double h1 = TerrainSystem::evaluateAnalyticHeight(150.5, -320.25, 1337);
    double h2 = TerrainSystem::evaluateAnalyticHeight(150.5, -320.25, 1337);
    t.check(h1 == h2, "analytic height bitwise deterministic for identical inputs");

    TerrainSystem terrainA(1337);
    terrainA.initialize();
    TerrainSystem terrainB(1337);
    terrainB.initialize();

    bool identicalGrid = true;
    for (double x = -900; x <= 900; x += 150) {
        for (double z = -900; z <= 900; z += 150) {
            if (terrainA.groundHeight(x, z) != terrainB.groundHeight(x, z)) {
                identicalGrid = false;
            }
        }
    }
    t.check(identicalGrid, "separate TerrainSystem instances with same seed produce identical heights");

    double diffSeed = TerrainSystem::evaluateAnalyticHeight(150.5, -320.25, 9999);
    t.check(diffSeed != h1, "different seed changes heightfield");

    // 2. Layout & Spawn Safety (Section 6)
    double spawnGround = terrainA.groundHeight(0.0, 0.0);
    t.check(spawnGround >= 2.0 && spawnGround < 25.0, "spawn ground elevation is safe and gentle");
    double spawnClearance = WorldConfig::spawnPosition.y - spawnGround;
    t.check(spawnClearance > 90.0, "spawn position provides safe clearance > 90m");

    double northHighland = terrainA.groundHeight(0.0, 700.0);
    t.check(northHighland > spawnGround + 50.0, "north features high mountains/highlands");

    // South winding valley check: valley center is carved below surrounding ridge
    double valleyZ = -450.0;
    double valleyX = 180.0 * std::sin(valleyZ * 0.006) + 70.0 * std::cos(valleyZ * 0.014);
    double valleyFloor = terrainA.groundHeight(valleyX, valleyZ);
    double valleyWall = terrainA.groundHeight(valleyX + 120.0, valleyZ);
    t.check(valleyWall > valleyFloor + 10.0, "south features carved winding valley corridor");

    // 3. GroundHeight Consistency against Rendered Mesh (Section 6 & Acceptance)
    const auto& mesh = terrainA.mesh();
    t.check(mesh.vertices.size() == WorldConfig::numVertsX * WorldConfig::numVertsZ, "mesh vertex count matches grid");
    t.check(mesh.triangles.size() == WorldConfig::numCellsX * WorldConfig::numCellsZ * 2, "mesh triangle count matches grid cells * 2");

    // Exact match at all grid vertices
    bool allVertsMatch = true;
    for (int iz = 0; iz < WorldConfig::numVertsZ; ++iz) {
        for (int ix = 0; ix < WorldConfig::numVertsX; ++ix) {
            double vx = WorldConfig::minX + ix * WorldConfig::cellSize;
            double vz = WorldConfig::minZ + iz * WorldConfig::cellSize;
            double meshY = mesh.vertices[iz * WorldConfig::numVertsX + ix].position.y;
            double queryY = terrainA.groundHeight(vx, vz);
            if (std::abs(meshY - queryY) > 1e-12) {
                allVertsMatch = false;
            }
        }
    }
    t.check(allVertsMatch, "GroundHeight exactly matches mesh vertex heights at all grid vertices");

    // Triangle-consistent plane interpolation test at arbitrary fractional points
    bool allSamplePlanesMatch = true;
    for (double testX = -850.3; testX <= 850.3; testX += 83.7) {
        for (double testZ = -850.7; testZ <= 850.7; testZ += 83.7) {
            double gh = terrainA.groundHeight(testX, testZ);
            // Recompute plane analytically from cell vertices
            double u = (testX - WorldConfig::minX) / WorldConfig::cellSize;
            double v = (testZ - WorldConfig::minZ) / WorldConfig::cellSize;
            int cix = int(std::floor(u));
            int ciz = int(std::floor(v));
            double fx = u - cix;
            double fz = v - ciz;
            double h00 = terrainA.heights()[ciz * WorldConfig::numVertsX + cix];
            double h10 = terrainA.heights()[ciz * WorldConfig::numVertsX + (cix + 1)];
            double h01 = terrainA.heights()[(ciz + 1) * WorldConfig::numVertsX + cix];
            double h11 = terrainA.heights()[(ciz + 1) * WorldConfig::numVertsX + (cix + 1)];
            double expected = (fx >= fz) ? (h00 + fx * (h10 - h00) + fz * (h11 - h10))
                                         : (h00 + fx * (h11 - h01) + fz * (h01 - h00));
            if (std::abs(gh - expected) > 1e-12) {
                allSamplePlanesMatch = false;
            }
        }
    }
    t.check(allSamplePlanesMatch, "GroundHeight exactly matches triangle mesh plane at hundreds of sample points");

    // 4. World Bounds Behaviour
    t.check(terrainA.inBounds(0, 0), "center is in bounds");
    t.check(terrainA.inBounds(-1000, 1000) && terrainA.inBounds(1000, -1000), "corners are in bounds");
    t.check(!terrainA.inBounds(1001, 0) && !terrainA.inBounds(0, -1001), "outside points are out of bounds");
    double clampedOutside = terrainA.groundHeight(2500.0, -3000.0);
    double clampedCorner = terrainA.groundHeight(1000.0, -1000.0);
    t.check(near(clampedOutside, clampedCorner) && std::isfinite(clampedOutside), "sampling outside bounds clamps safely to boundary");

    // 5. FlightController & Terrain Interaction
    FlightController controller;
    Aircraft a;
    a.reset();

    // Default update without terrain maintains exact M1 behaviour
    controller.update(a, {}, 1.0);
    t.check(near(a.transform.position.z, 70.0) && near(a.speed, 70.0) && !a.terrainAssist, "controller without terrain keeps M1 behaviour");

    // Clearance calculation
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 50.0, 0};
    controller.update(a, {}, 0.016, &terrainA);
    t.check(near(a.clearance, 50.0, 0.5) && !a.terrainWarning && !a.terrainAssist, "clearance > 25m applies no assist or warning");

    // Above 25m descending: no intervention
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 30.0, 0};
    // Pitch nose down
    controller.update(a, {-1.0, 0, 0, false}, 0.2, &terrainA);
    t.check(a.transform.forward().y < 0 && a.clearance > 25.0 && !a.terrainAssist, "clearance > 25m while descending has no intervention");

    // 8m - 25m while NOT descending (level or climbing): no assist
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 18.0, 0};
    // Ensure climbing or level
    a.transform.orientation = Quat::axisAngle({1, 0, 0}, -radians(10)); // nose up
    controller.update(a, {}, 0.016, &terrainA);
    t.check(!a.terrainAssist && !a.terrainWarning, "8-25m range while climbing applies no assist");

    // 8m - 25m while descending: applies limited pitch-up assist
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 18.0, 0};
    a.transform.orientation = Quat::axisAngle({1, 0, 0}, radians(15)); // nose down
    double initialForwardY = a.transform.forward().y;
    controller.update(a, {}, 0.1, &terrainA);
    t.check(a.terrainAssist, "8-25m range while descending triggers terrainAssist");
    t.check(a.transform.forward().y > initialForwardY, "terrain assist applies pitch-up rotation to raise nose");

    // Below 8m: warning state active
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 6.0, 0};
    controller.update(a, {}, 0.016, &terrainA);
    t.check(a.terrainWarning, "clearance < 8m triggers terrainWarning");

    // Ground collision: aircraft cannot sink below mesh, produces consistent crash state
    a.reset();
    a.transform.position = {0, terrainA.groundHeight(0, 0) + 1.0, 0};
    a.transform.orientation = Quat::axisAngle({1, 0, 0}, radians(45)); // steep dive
    a.speed = 100.0;
    // Step forward into ground
    controller.update(a, {}, 0.1, &terrainA);
    double crashGroundY = terrainA.groundHeight(a.transform.position.x, a.transform.position.z);
    t.check(a.crashed, "hitting ground triggers crash state");
    t.check(near(a.speed, 0.0), "crashed aircraft speed is zeroed");
    t.check(near(a.transform.position.y, crashGroundY), "crashed aircraft never sinks below mesh surface");

    // Subsequent updates while crashed remain clamped and immobile
    Vec3 crashPos = a.transform.position;
    controller.update(a, {1.0, 1.0, 1.0, true}, 0.5, &terrainA);
    t.check(a.crashed && near(a.speed, 0.0) && near(a.transform.position.y, crashGroundY) && near(a.transform.position.x, crashPos.x) && near(a.transform.position.z, crashPos.z), "crashed aircraft remains immobile until reset");

    // Aircraft reset restores un-crashed flight
    a.reset();
    t.check(!a.crashed && near(a.speed, 70.0) && near(a.transform.position.y, 120.0), "reset restores aircraft to safe flight state");

    // Boundary handling: soft turn near boundary
    a.reset();
    a.transform.position = {940.0, 100.0, 0.0}; // near positive X boundary heading +X
    a.transform.orientation = Quat::axisAngle({0, 1, 0}, radians(90)); // heading +X
    double initXFwd = a.transform.forward().x;
    controller.update(a, {}, 0.2, &terrainA);
    t.check(a.transform.forward().x < initXFwd, "boundary soft turn steers heading back towards center");

    // Boundary hard containment
    a.reset();
    a.transform.position = {995.0, 100.0, 0.0};
    a.transform.orientation = Quat::axisAngle({0, 1, 0}, radians(90));
    a.speed = 140.0;
    controller.update(a, {}, 1.0, &terrainA);
    t.check(a.transform.position.x <= WorldConfig::maxX, "aircraft position clamped to world boundary limit");

    // 6. ChaseCamera Terrain Avoidance
    ChaseCamera cam;
    a.reset();
    double groundUnderSpawn = terrainA.groundHeight(a.transform.position.x, a.transform.position.z);
    cam.reset(a, &terrainA);
    t.check(cam.pose().position.y >= groundUnderSpawn + WorldConfig::cameraGroundMargin, "camera reset respects ground margin");

    // Place aircraft low over rising terrain so standard camera offset would penetrate ground
    double gY = terrainA.groundHeight(0.0, 0.0);
    a.transform.position = Vec3{0.0, gY + 1.0, 0.0};
    cam.update(a, 0.1, &terrainA);
    double camGroundY = terrainA.groundHeight(cam.pose().position.x, cam.pose().position.z);
    t.check(cam.pose().position.y >= camGroundY + WorldConfig::cameraGroundMargin,
            "camera update never sits below groundHeight + margin");

    return t.finish();
}
