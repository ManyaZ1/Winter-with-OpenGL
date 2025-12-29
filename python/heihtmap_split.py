import numpy as np
from PIL import Image
import json
import os
import struct
# ================= CONFIG =================
OBJ_PATH       = "C:/Users/USER/Documents/GitHub/Winter-with-OpenGL/source/source/assets/Mesher.obj"
LAKE_MASK_PATH = "C:/Users/USER/Documents/GitHub/Winter-with-OpenGL/source/source/assets/Lake_Mask.bmp"

OUT_DIR = "C:/Users/USER/Documents/GitHub/Winter-with-OpenGL/source/source/assets/heightmap/"

GRID_RESOLUTION = 1024          # 1024x1024 heightmap
SCALING_FACTOR  = 200.0         # same as C++
MASK_PATH = "C:/Users/USER/Documents/GitHub/Winter-with-OpenGL/source/source/assets/tree_density.png"
# ==========================================


def load_vertices(obj_path):
    verts = []
    with open(obj_path, "r") as f:
        for line in f:
            if line.startswith("v "):
                _, x, y, z = line.split()
                verts.append([float(x), float(y), float(z)])
    return np.array(verts, dtype=np.float32)


def world_to_grid(x, z, bounds, res):
    min_x, max_x, min_z, max_z = bounds
    u = (x - min_x) / (max_x - min_x)
    v = (z - min_z) / (max_z - min_z)
    ix = int(np.clip(u * (res - 1), 0, res - 1))
    iz = int(np.clip(v * (res - 1), 0, res - 1))
    return ix, iz


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    print("Loading OBJ vertices...")
    verts = load_vertices(OBJ_PATH)

    # Apply same scaling as C++
    verts *= SCALING_FACTOR

    # Compute bounds in XZ
    min_x, max_x = verts[:,0].min(), verts[:,0].max()
    min_z, max_z = verts[:,2].min(), verts[:,2].max()
    bounds = (min_x, max_x, min_z, max_z)

    print("Generating heightfield...")
    heightmap = np.full((GRID_RESOLUTION, GRID_RESOLUTION), -np.inf, dtype=np.float32)



    # Rasterize vertices into grid (max height wins)
    for x, y, z in verts:
        ix, iz = world_to_grid(x, z, bounds, GRID_RESOLUTION)
        heightmap[iz, ix] = max(heightmap[iz, ix], y)

    # Fill empty cells (simple nearest-neighbor fill)
    mask = heightmap == -np.inf
    heightmap[mask] = np.interp(
        np.flatnonzero(mask),
        np.flatnonzero(~mask),
        heightmap[~mask]
    )

    # Normalize height for image preview
    h_min, h_max = heightmap.min(), heightmap.max()
    height_norm = (heightmap - h_min) / (h_max - h_min + 1e-6)

    # Save heightmap
    np.save(os.path.join(OUT_DIR, "terrain_height.npy"), heightmap)
    Image.fromarray((height_norm * 255).astype(np.uint8)) \
         .save(os.path.join(OUT_DIR, "terrain_height.png"))

    print("Loading lake mask...")
    lake_img = Image.open(LAKE_MASK_PATH).convert("L").resize(
        (GRID_RESOLUTION, GRID_RESOLUTION),
        Image.BILINEAR
    )
    lake_mask = np.array(lake_img, dtype=np.float32) / 255.0

    # Mountain mask = inverse lake
    mountain_mask = 1.0 - lake_mask

    np.save(os.path.join(OUT_DIR, "lake_mask.npy"), lake_mask)
    np.save(os.path.join(OUT_DIR, "mountain_mask.npy"), mountain_mask)

    # Save metadata
    meta = {
        "grid_resolution": GRID_RESOLUTION,
        "scaling_factor": SCALING_FACTOR,
        "bounds": {
            "min_x": float(min_x),
            "max_x": float(max_x),
            "min_z": float(min_z),
            "max_z": float(max_z),
            "min_y": float(h_min),
            "max_y": float(h_max)
        }
    }
    #FOREST MASK
    forest_img = Image.open(MASK_PATH).convert("L").resize(
        (GRID_RESOLUTION, GRID_RESOLUTION),
        Image.BILINEAR
    )

    forest_mask = np.array(forest_img, dtype=np.float32) / 255.0

    # Optional: hard threshold (recommended)
    forest_mask = (forest_mask > 0.5).astype(np.float32)

    with open(os.path.join(OUT_DIR, "terrain_meta.json"), "w") as f:
        json.dump(meta, f, indent=4)

    print("Done.")
    print("Heightmap + lake/mountain masks generated.")

    output_file = os.path.join(OUT_DIR, "terrain_data.bin")
    
    with open(output_file, "wb") as f:
        # 1. Header (8 values)
        header = struct.pack("ifffffff", 
            GRID_RESOLUTION, 
            SCALING_FACTOR,
            float(min_x), float(max_x), 
            float(min_z), float(max_z), 
            float(h_min), float(h_max)
        )
        f.write(header)

        # 2. Body: Raw float32 buffers in a clean, specific order
        # HEIGHTMAP (Block 1)
        f.write(heightmap.astype(np.float32).tobytes())
        # FOREST MASK (Block 2)
        f.write(forest_mask.astype(np.float32).tobytes())
        # LAKE MASK (Block 3)
        f.write(lake_mask.astype(np.float32).tobytes())

    print(f"Exported all terrain data to {output_file}")

if __name__ == "__main__":
    main()
