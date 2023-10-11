#version 430

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) writeonly uniform image2D destTex;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float cameraFov;  // Add this line for the FOV uniform

// SVO data would also be passed in some form, either as a texture, SSBO, etc.

void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    vec2 fragCoord = vec2(storePos) / vec2(640, 480); // Assuming a 640x480 resolution

    // Incorporate FOV into the ray direction calculation
    float fovScale = tan(cameraFov * 0.5);
    vec3 rayDir = normalize(cameraDir +
                            (fragCoord.x - 0.5) * cameraRight * fovScale +
                            (fragCoord.y - 0.5) * cameraUp * fovScale);

    // Initialize ray at camera position
    vec3 rayPos = cameraPos;

    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0); // Default to black

    // Perform SVO traversal (simplified)
    // while (not at end of ray) {
    //     if (ray intersects SVO) {
    //         pixel = color of intersected voxel;
    //         break;
    //     }
    //     move ray forward;
    // }

    imageStore(destTex, storePos, pixel);
}