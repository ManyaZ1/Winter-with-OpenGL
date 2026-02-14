import trimesh
import numpy as np
from PIL import Image

# =========================
# CONFIG
# =========================
OBJ_IN  = "Mesher_LOD2.obj"  # "Mesher.obj"
OBJ_OUT = "Mesher_LOD2_flat_lake_no_blur.obj"  # "Mesher_flat_lake_no_blur.obj"
MASK    = "lake_mask.bmp"

waterHeight = 3.21598/200
mask_threshold = 128   # 0–255, adjust if needed

# =========================
# Load mesh
# =========================
mesh = trimesh.load(OBJ_IN, process=False)
vertices = mesh.vertices.copy()

# =========================
# Load mask
# =========================
mask_img = Image.open(MASK).convert("L")
mask = np.array(mask_img)
mask_h, mask_w = mask.shape

# =========================
# Terrain bounds (XZ)
# =========================
min_x, max_x = vertices[:, 0].min(), vertices[:, 0].max()
min_z, max_z = vertices[:, 2].min(), vertices[:, 2].max()

# =========================
# Helper: Bilinear interpolation for mask sampling
# =========================
def sample_mask_bilinear(u, v, mask):
    h, w = mask.shape
    x = u * (w - 1)
    y = (1.0 - v) * (h - 1)  # flip vertical
    x0 = int(np.floor(x))
    x1 = min(x0 + 1, w - 1)
    y0 = int(np.floor(y))
    y1 = min(y0 + 1, h - 1)
    dx = x - x0
    dy = y - y0
    val = (
        (1 - dx) * (1 - dy) * mask[y0, x0] +
        dx * (1 - dy) * mask[y0, x1] +
        (1 - dx) * dy * mask[y1, x0] +
        dx * dy * mask[y1, x1]
    )
    return val / 255.0  # Normalize to 0-1

# =========================
# Helper: Smoothstep function
# =========================
def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3 - 2 * t)

# =========================
# Flatten lake vertices (no blur)
# =========================
lake_vertex_count = 0
original_vertices = mesh.vertices.copy()
vertices = mesh.vertices.copy()
for i, (x, y, z) in enumerate(vertices):
    if (min_x <= x <= max_x) and (min_z <= z <= max_z):
        u = (x - min_x) / (max_x - min_x)
        v = (z - min_z) / (max_z - min_z)
        px = int(u * (mask_w - 1))
        py = int((1.0 - v) * (mask_h - 1))
        px = np.clip(px, 0, mask_w - 1)
        py = np.clip(py, 0, mask_h - 1)
        if mask[py, px] >= mask_threshold:
            vertices[i][1] = waterHeight
            lake_vertex_count += 1
        else:
            t = sample_mask_bilinear(u, v, mask)
            t = smoothstep(0.0, 1.0, t)
            if t > 0.001:
                vertices[i][1] = (1.0 - t) * y + t * waterHeight
                lake_vertex_count += 1

# =========================
# Save result
# =========================
mesh.vertices = vertices
mesh.export(OBJ_OUT)

print(f"Flattened {lake_vertex_count} lake vertices (no blur)")
print(f"Saved -> {OBJ_OUT}")
