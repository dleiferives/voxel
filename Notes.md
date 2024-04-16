# TODO:
 - What is the difference between vertex and fragment shaders
    - Vertex shaders happen first and they find where on the screen the triangles will go
    - The fragment shaders fill in those triangles with texture
 - What are uniforms.
   - Uniforms are a way to send data from the CPU to the GPU, in a manner that the uniform can
     change over time, and this will be computed differently.
 - Where do textures come in and how are they used?

# NOTE:
 -- vec4(x,y,z,w). w==0 then it is a vector, w==1 it is a position
 -- matrix multiplication works from left to right
    - trans * rot * scale * orig
    -> original gets scaled then rotated then translated!
-- To use uniform buffers (like for Model View projection!, use the uniform buffer t, which can take in a struct!)

