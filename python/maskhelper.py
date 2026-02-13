import trimesh
import numpy as np
from PIL import Image
from scipy.ndimage import binary_fill_holes, gaussian_filter

# =========================
# CONFIG
# =========================
OBJ_FILE = "Mesher_flat_lake.obj"
OUT_MASK = "lake_mask_from_mesh0.bmp"

waterHeight = 3.21598       # search for 3.2 after scaling waterHeight = 3.21598f;
height_epsilon = 0.01        # increased tolerance for height comparison

MASK_WIDTH  = 1024          # match your heightmap!
MASK_HEIGHT = 1024

SMOOTH_SIGMA = 2.0          # set 0 to disable smoothing

# =========================
# Load mesh
# =========================
mesh = trimesh.load(OBJ_FILE, process=False)
verts = mesh.vertices * 200  # Scale terrain by 200

# =========================
# Terrain bounds (XZ)
# =========================
min_x, max_x = verts[:, 0].min(), verts[:, 0].max()
min_z, max_z = verts[:, 2].min(), verts[:, 2].max()

# =========================
# Create empty mask
# =========================
mask = np.zeros((MASK_HEIGHT, MASK_WIDTH), dtype=np.uint8)

# =========================
# World → mask coords
# =========================
def world_to_mask(x, z):
    u = (x - min_x) / (max_x - min_x)
    v = (z - min_z) / (max_z - min_z)

    px = int(u * (MASK_WIDTH - 1))
    py = int((1.0 - v) * (MASK_HEIGHT - 1))

    return np.clip(px, 0, MASK_WIDTH - 1), np.clip(py, 0, MASK_HEIGHT - 1)

# =========================
# Mark lake vertices
# =========================
lake_count = 0
print("Mean Y:", np.mean(verts[:,1]))
print("WaterHeight:", waterHeight)
#==δεβθγ
rounded = np.round(verts[:,1], 3)
values, counts = np.unique(rounded, return_counts=True)

top = sorted(zip(values, counts), key=lambda x: -x[1])[:10]
print("Most frequent heights:")
print(top)
for x, y, z in verts:
    if abs(y - waterHeight) <= height_epsilon:
        px, py = world_to_mask(x, z)
        mask[py, px] = 255
        # # Mark a 5x5 area around each lake vertex
        # for dx in range(-2, 3):
        #     for dy in range(-2, 3):
        #         nx, ny = px + dx, py + dy
        #         if 0 <= nx < MASK_WIDTH and 0 <= ny < MASK_HEIGHT:
        #             mask[ny, nx] = 255
        lake_count += 1

print(f"Lake vertices detected: {lake_count}")

# Debug: Save mask after marking lake vertices
Image.fromarray(mask, mode="L").save("lake_mask_debug_raw.bmp")
print("Saved debug mask lake_mask_debug_raw.bmp")

# =========================
# Fill interior holes
# =========================
#mask = binary_fill_holes(mask > 0).astype(np.uint8) * 255
#mask = binary_closing(mask > 0, structure=np.ones((3,3))).astype(np.uint8) * 255
# =========================
# Optional smoothing
# =========================
if SMOOTH_SIGMA > 0:
    mask = gaussian_filter(mask.astype(float), sigma=SMOOTH_SIGMA)
    mask = np.clip(mask, 0, 255)
    mask = (mask > 127).astype(np.uint8) * 255  # Binarize after smoothing

# =========================
# Save mask
# =========================
Image.fromarray(mask, mode="L").save(OUT_MASK)
print(f"Saved lake mask {OUT_MASK}")


