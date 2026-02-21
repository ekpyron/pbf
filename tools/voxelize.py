#!/usr/bin/env python3
import pyvista as pv
from pathlib import Path
import numpy as np

reader = pv.get_reader("bunny.obj")
mesh = reader.read()

vox = mesh.voxelize()
# vox.bounds.x_min
# vox.bounds.x_max

bounds_min = np.array([vox.bounds.x_min, vox.bounds.y_min, vox.bounds.z_min])
bounds_max = np.array([vox.bounds.x_max, vox.bounds.y_max, vox.bounds.z_max])
bounds_extent = bounds_max - bounds_min

#points = np.array(mesh.voxelize().points)
points = np.array(mesh.points)
points = ((points - bounds_min) / bounds_extent)

print("#include <glm/glm.hpp>")
print("// Based on the stanford bunny; see https://graphics.stanford.edu/data/3Dscanrep/#bunny")
print("")
print("auto constexpr voxels = std::array{")
for point in points:
    print("\tglm::vec3({},{},{}),".format(*point))
print("};")
