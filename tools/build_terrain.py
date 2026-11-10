#!/usr/bin/env python3
"""Build the SkyCombat 3D world terrain in Blender (headless).

World layout (top-down, 2000 m x 2000 m, +Y = north, +X = east):
- Centre: wide gentle green grassland plain (flight / dogfight area)
- North edge: high mountains and highland (rocky, highest elevation)
- South: a winding valley carved through (S-curve flight path)
- West side: purple flower field patch
- East side: pink flower field patch
- Gentle rolling hills elsewhere; deterministic height function (fixed seed).

Outputs in this folder: terrain.blend, terrain.obj, terrain.glb, terrain_preview.png
"""
import math
import os

import bpy
from mathutils import Vector

BASE = os.path.dirname(os.path.abspath(__file__))

# ----------------------------------------------------------------------------
# Deterministic value noise (integer hash, no RNG state)
# ----------------------------------------------------------------------------

def _hash2(ix, iy):
    h = (ix * 374761393 + iy * 668265263) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    h = h ^ (h >> 16)
    return h / 4294967295.0


def value_noise(x, y):
    ix = math.floor(x)
    iy = math.floor(y)
    fx = x - ix
    fy = y - iy
    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    v00 = _hash2(ix, iy)
    v10 = _hash2(ix + 1, iy)
    v01 = _hash2(ix, iy + 1)
    v11 = _hash2(ix + 1, iy + 1)
    a = v00 + (v10 - v00) * sx
    b = v01 + (v11 - v01) * sx
    return a + (b - a) * sy  # 0..1


def fbm(x, y, octaves=4):
    total = 0.0
    amp = 0.5
    freq = 1.0
    norm = 0.0
    for _ in range(octaves):
        total += amp * value_noise(x * freq, y * freq)
        norm += amp
        amp *= 0.5
        freq *= 2.03
    return total / norm  # 0..1


def clamp(v, lo, hi):
    return lo if v < lo else hi if v > hi else v


def smoothstep(e0, e1, v):
    t = clamp((v - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    return a + (b - a) * t


# ----------------------------------------------------------------------------
# Height function
# ----------------------------------------------------------------------------

WORLD = 2000.0  # metres, square


def valley_center_x(y):
    """S-curve path of the southern valley."""
    return 420.0 * math.sin(y * 0.0022 + 0.6) + 180.0 * math.sin(y * 0.0051)


def terrain_height(x, y):
    # Rolling hills; gentler near the centre plain.
    hills_n = fbm(x * 0.0016 + 3.1, y * 0.0016 + 8.7, 4)
    r = math.hypot(x, y)
    centre_calm = 0.45 + 0.55 * smoothstep(150.0, 750.0, r)
    h = 12.0 + hills_n * 52.0 * centre_calm

    # Small detail noise everywhere.
    h += fbm(x * 0.009 + 21.0, y * 0.009 + 4.0, 3) * 5.0

    # Northern mountains: mask grows towards the north edge, ridged noise.
    m = smoothstep(330.0, 950.0, y)
    if m > 0.0:
        ridge = 1.0 - abs(2.0 * fbm(x * 0.0011 + 13.7, y * 0.0011 + 7.3, 4) - 1.0)
        ridge = ridge * ridge
        h += m * (85.0 + ridge * 250.0)
        h += m * fbm(x * 0.006 + 9.0, y * 0.006 + 1.0, 3) * 22.0

    # Southern valley: carve along the S-curve.
    vm = clamp((230.0 - y) / 320.0, 0.0, 1.0)  # 1 deep south, 0 by y=+230
    if vm > 0.0:
        d = abs(x - valley_center_x(y))
        g = math.exp(-((d / 210.0) ** 2))
        h += vm * 26.0 * (1.0 - g)  # raise the southern shoulders so the cut reads
        carve = g * vm
        if carve > 0.003:
            floor_h = 3.5 + (y + 1000.0) * 0.005 + fbm(x * 0.008, y * 0.008, 2) * 2.0
            h = mix(h, floor_h, clamp(carve * 1.05, 0.0, 0.97))
    return h


# ----------------------------------------------------------------------------
# Vertex colours by biome
# ----------------------------------------------------------------------------

def terrain_color(x, y, h, slope):
    # Grassland base, varied by noise + altitude.
    gn = fbm(x * 0.004 + 31.0, y * 0.004 + 17.0, 3)
    r = mix(0.15, 0.29, gn)
    g = mix(0.38, 0.56, gn)
    b = mix(0.12, 0.20, gn)
    col = (r, g, b)

    # West purple flower field (elliptical patch).
    ew = ((x + 620.0) / 440.0) ** 2 + ((y + 30.0) / 540.0) ** 2
    fw = clamp((1.12 - ew) / 0.45, 0.0, 1.0)
    if fw > 0.0:
        sparkle = value_noise(x * 0.05, y * 0.05)
        violet = (mix(0.46, 0.64, sparkle), mix(0.22, 0.33, sparkle), mix(0.78, 0.94, sparkle))
        col = tuple(mix(c, v, fw * 0.9) for c, v in zip(col, violet))

    # East pink flower field (elliptical patch).
    ee = ((x - 640.0) / 440.0) ** 2 + ((y - 60.0) / 540.0) ** 2
    fe = clamp((1.12 - ee) / 0.45, 0.0, 1.0)
    if fe > 0.0:
        sparkle = value_noise(x * 0.05 + 40.0, y * 0.05 + 9.0)
        pink = (mix(0.93, 1.0, sparkle), mix(0.42, 0.58, sparkle), mix(0.60, 0.74, sparkle))
        col = tuple(mix(c, v, fe * 0.9) for c, v in zip(col, pink))

    # Valley floor: lighter sandy-green where the carve is strong and low.
    vm = clamp((230.0 - y) / 320.0, 0.0, 1.0)
    if vm > 0.0 and h < 30.0:
        d = abs(x - valley_center_x(y))
        carve = math.exp(-((d / 210.0) ** 2)) * vm
        if carve > 0.35:
            t = clamp((carve - 0.35) / 0.65, 0.0, 1.0) * 0.9
            sandy = (0.62, 0.63, 0.34)
            col = tuple(mix(c, s, t) for c, s in zip(col, sandy))

    # Rocky grey on high altitude or steep slopes (applied last: mountains win).
    rock_h = smoothstep(115.0, 200.0, h)
    rock_s = smoothstep(0.55, 0.95, slope) * (1.0 - rock_h * 0.35)
    rock = clamp(max(rock_h, rock_s), 0.0, 1.0)
    if rock > 0.0:
        rn = fbm(x * 0.01 + 5.0, y * 0.01 + 27.0, 3)
        grey = mix(0.24, 0.40, rn)
        rock_col = (grey, grey * 0.96, grey * 0.90)
        col = tuple(mix(c, rc, rock) for c, rc in zip(col, rock_col))
        # Slight pale tint only on the very highest summits.
        peak = smoothstep(330.0, 390.0, h)
        if peak > 0.0:
            pale = (0.60, 0.59, 0.56)
            col = tuple(mix(c, p, peak * 0.35) for c, p in zip(col, pale))
    return col


# ----------------------------------------------------------------------------
# Build the mesh (129 x 129 vertices for 128 x 128 segments)
# ----------------------------------------------------------------------------

SEG = 128
N = SEG + 1
STEP = WORLD / SEG
HALF = WORLD / 2.0

bpy.ops.wm.read_factory_settings(use_empty=True)

heights = [[0.0] * N for _ in range(N)]
verts = []
for j in range(N):
    y = -HALF + j * STEP
    for i in range(N):
        x = -HALF + i * STEP
        h = terrain_height(x, y)
        heights[j][i] = h
        verts.append((x, y, h))

faces = []
for j in range(SEG):
    for i in range(SEG):
        a = j * N + i
        b = a + 1
        c = a + N
        d = c + 1
        faces.append((a, b, d, c))

mesh = bpy.data.meshes.new("TerrainMesh")
mesh.from_pydata(verts, [], faces)
mesh.update()

obj = bpy.data.objects.new("Terrain", mesh)
bpy.context.collection.objects.link(obj)
bpy.context.view_layer.objects.active = obj
obj.select_set(True)

# Flat shading (low-poly look).
for poly in mesh.polygons:
    poly.use_smooth = False

# Vertex colours (per-point domain).
color_attr = mesh.color_attributes.new(name="Col", type="FLOAT_COLOR", domain="POINT")
for j in range(N):
    y = -HALF + j * STEP
    for i in range(N):
        x = -HALF + i * STEP
        h = heights[j][i]
        # Slope from finite differences (rise over run, per metre).
        hx = heights[j][min(i + 1, SEG)] - heights[j][max(i - 1, 0)]
        hy = heights[min(j + 1, SEG)][i] - heights[max(j - 1, 0)][i]
        span = STEP * (2 if 0 < i < SEG else 1)
        spany = STEP * (2 if 0 < j < SEG else 1)
        slope = math.hypot(hx / span, hy / spany)
        idx = j * N + i
        r, g, b = terrain_color(x, y, h, slope)
        color_attr.data[idx].color = (r, g, b, 1.0)

# Material driven by vertex colours.
mat = bpy.data.materials.new("TerrainMat")
mat.use_nodes = True
bsdf = mat.node_tree.nodes["Principled BSDF"]
bsdf.inputs["Roughness"].default_value = 0.95
vc = mat.node_tree.nodes.new("ShaderNodeVertexColor")
vc.layer_name = "Col"
mat.node_tree.links.new(vc.outputs["Color"], bsdf.inputs["Base Color"])
mesh.materials.append(mat)

# ----------------------------------------------------------------------------
# Sun light + light-blue world
# ----------------------------------------------------------------------------

sun_data = bpy.data.lights.new("Sun", type="SUN")
sun_data.energy = 2.6
sun_data.angle = math.radians(8.0)
sun = bpy.data.objects.new("Sun", sun_data)
bpy.context.collection.objects.link(sun)
sun.location = (500.0, -600.0, 900.0)
sun.rotation_euler = (math.radians(52.0), math.radians(12.0), math.radians(38.0))

world = bpy.data.worlds.new("World")
bpy.context.scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes["Background"]
bg.inputs[0].default_value = (0.55, 0.75, 0.95, 1.0)  # light blue sky
bg.inputs[1].default_value = 0.9

# ----------------------------------------------------------------------------
# Camera: oblique aerial from the south-east looking north-west
# ----------------------------------------------------------------------------

cam_data = bpy.data.cameras.new("Camera")
cam_data.lens = 40.0
cam_data.clip_end = 12000.0
cam = bpy.data.objects.new("Camera", cam_data)
bpy.context.collection.objects.link(cam)
cam.location = (1620.0, -1620.0, 1120.0)
target = Vector((-80.0, 180.0, 0.0))
direction = target - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
bpy.context.scene.camera = cam

# ----------------------------------------------------------------------------
# Render preview
# ----------------------------------------------------------------------------

scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.samples = 64
scene.cycles.use_denoising = False  # this Blender build has no OpenImageDenoiser
scene.render.resolution_x = 1280
scene.render.resolution_y = 960
scene.render.filepath = os.path.join(BASE, "terrain_preview.png")
bpy.ops.render.render(write_still=True)

# ----------------------------------------------------------------------------
# Exports
# ----------------------------------------------------------------------------

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(BASE, "terrain.blend"))

bpy.ops.wm.obj_export(
    filepath=os.path.join(BASE, "terrain.obj"),
    export_colors=True,
    export_materials=True,
)

bpy.ops.export_scene.gltf(
    filepath=os.path.join(BASE, "terrain.glb"),
    export_format="GLB",
)

print("TERRAIN_DONE verts=%d faces=%d" % (len(mesh.vertices), len(mesh.polygons)))
print("Height range: min=%.1f max=%.1f" % (
    min(min(row) for row in heights), max(max(row) for row in heights)))
