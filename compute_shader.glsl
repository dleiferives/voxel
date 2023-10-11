#version 430

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) writeonly uniform image2D destTex;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float cameraFov;

// SVO data would also be passed in some form, either as a texture, SSBO, etc.

float querySVO(vec3 position) {
    // Query the SVO and return some value (e.g., density)
    // This is a placeholder; you'll need to implement this based on how your SVO data is stored
    return 0.0;
}

void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    vec2 fragCoord = vec2(storePos) / vec2(640, 480); // Assuming a 640x480 resolution

    float fovScale = tan(cameraFov * 0.5);
    vec3 rayDir = normalize(cameraDir +
                            (fragCoord.x - 0.5) * cameraRight * fovScale +
                            (fragCoord.y - 0.5) * cameraUp * fovScale);

    vec3 rayPos = cameraPos;
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0); // Default to black

    float stepSize = 0.1; // Size of each step along the ray
    float t = 0.0; // Current distance along the ray

    // Perform ray marching
    for (int i = 0; i < 100; ++i) { // Limit the number of steps to avoid infinite loops
        float density = querySVO(rayPos + rayDir * t);

        if (density > 0.5) {
            // We hit something!
            pixel = vec4(1.0, 0.0, 0.0, 1.0); // Set to red for demonstration
            break;
        }

        t += stepSize;
    }

    imageStore(destTex, storePos, pixel);
}