# Studio Assets (from the 3D Fighter Jet Web Studio)

Extracted 2026-11-10 from the standalone web studio file
`3d_fighter_jets_endless_looping_terrain_supersonic_vfx_3d_studio.html`
(Three.js viewer) so the C++/DxLib game can reuse the assets directly.
All files were decoded from the studio's embedded file table and verified
(OBJ vertex/face counts, PNG signatures, ZIP contents).

## models/
- `enemy_fighter_jet.obj/.mtl` — Enemy "Argon Interceptor": forward-swept
  wings + canards, 382 vertices / 650 faces. Intended as the M5 enemy mesh.
- `enemy_fighter_palette.png` — palette texture for the enemy jet.
- `enemy_fighter_sample.png` — 4-view reference render of the enemy jet.
- `fighter_jet_textured.obj/.mtl` — textured variant of the player jet
  (387 vertices / 658 faces), with `fighter_jet_palette.png`.
- `fighter_jet_sample.png` — 4-view reference render of the player jet.
- Note: the studio's plain `fighter_jet.obj` is byte-identical to the
  existing `assets/models/fighter_jet.obj`, so it is not duplicated here.

## vfx/
- Thruster/afterburner plumes (420 v / 700 f each; missile prefab 150 v / 256 f):
  `player_thruster_plume`, `enemy_thruster_plume`, `missile_thruster_vfx_prefab`.
- Supersonic wind / wingtip vortex / vapor-cone meshes (1184 v / 1208 f each):
  `player_wind_airflow_vfx`, `enemy_wind_airflow_vfx`, `supersonic_wind_vfx_prefab`.

## terrain_chunks/
- Tileable endless-loop terrain chunks (559 v / 986 f each):
  `terrain_canyon_loop_chunk`, `terrain_alpine_loop_chunk`,
  `terrain_cyber_loop_chunk`.
- The studio's island and volcanic biomes exist only as procedural code in
  the HTML viewer; no OBJ files were embedded for them.

## Usage notes
- OBJ/MTL pairs must stay together; several MTLs reference the palette PNGs
  by filename, so keep the PNGs in the same folder as their OBJ.
- These are reference/asset files. Engine integration (loading, scaling,
  axis conversion for DxLib) is a separate code step.
