# 3DRenderer

C cpu graphics engine implemented from scratch. 

Implemented using the setup()/run() paradigm, where the 3d context is setup, meshes are loaded, and then the run() step begins. 

The application runs on an infinite loop; in each iteration it transforms the vertices of the mesh from mesh space to clip space, rasterizes them by triangle, performs depth testing using a z buffer, loads in texture data from a corresponding png image using u/v coordinates, and then updates the pixel's corresponding location in the frame buffer. Once completed, we use SDL to perform a system call to update the display.

Functionality:
- Load .obj meshes and corresonding .png textures
- Render vertices, wireframes, untextured objects, and textured objects, along with combinations of these.
- To render the above objects, use keys 1-6, each of which represents different render settings as seen in the gif below

![](drone.gif)