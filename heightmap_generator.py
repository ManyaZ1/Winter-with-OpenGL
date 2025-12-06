#pip install numpy trimesh
import numpy as np
import trimesh
import sys

# --------------------------------------------
# USER SETTINGS
# --------------------------------------------
SCALING_FACTOR = 60.0
TRANSLATION = np.array([0.0, -1.0, -5.0])  # (x, y, z)
HEIGHTMAP_RESOLUTION = 1024  # 1024x1024 grid
OUTPUT_TXT = "heightmap.txt"

# --------------------------------------------
# LOAD OBJ
# --------------------------------------------
if len(sys.argv) < 2:
    print("Usage: python obj_to_heightmap.py model.obj")
    sys.exit(1)

obj_path = sys.argv[1]
mesh = trimesh.load(obj_path, force='mesh')

# --------------------------------------------
# APPLY TRANSFORMATION
# modelMatrix = T * S
# --------------------------------------------
vertices = mesh.vertices.copy()

# SCALE
vertices *= SCALING_FACTOR

# TRANSLATE
vertices += TRANSLATION

# --------------------------------------------
# BUILD HEIGHTMAP (Y as height, XZ as plane)
# --------------------------------------------
x = vertices[:, 0]
y = vertices[:, 1]  # HEIGHT
z = vertices[:, 2]

# Normalize X,Z into grid space
x_min, x_max = x.min(), x.max()
z_min, z_max = z.min(), z.max()

x_norm = (x - x_min) / (x_max - x_min)
z_norm = (z - z_min) / (z_max - z_min)

x_idx = (x_norm * (HEIGHTMAP_RESOLUTION - 1)).astype(int)
z_idx = (z_norm * (HEIGHTMAP_RESOLUTION - 1)).astype(int)

# Initialize heightmap with very low values
heightmap = np.full((HEIGHTMAP_RESOLUTION, HEIGHTMAP_RESOLUTION), -np.inf)

# Fill heightmap using MAX height per cell
for xi, zi, yi in zip(x_idx, z_idx, y):
    if yi > heightmap[zi, xi]:
        heightmap[zi, xi] = yi

# Replace empty cells with minimum height
min_height = np.min(heightmap[heightmap > -np.inf])
heightmap[heightmap == -np.inf] = min_height

# --------------------------------------------
# SAVE AS TXT FILE
# --------------------------------------------
np.savetxt(OUTPUT_TXT, heightmap, fmt="%.6f")

print("===================================")
print("OBJ processed successfully!")
print(f"Resolution: {HEIGHTMAP_RESOLUTION} x {HEIGHTMAP_RESOLUTION}")
print(f"Saved as: {OUTPUT_TXT}")
print("===================================")
