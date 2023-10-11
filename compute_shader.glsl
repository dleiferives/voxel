#version 430

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) writeonly uniform image2D destTex;

layout (std430, binding = 1) buffer SVOData {
    uint svo[]; // The actual SVO data
};

layout (std430, binding = 2) buffer IndirectionGrid {
    uint grid[]; // Each element is an index into the SVOData buffer
};

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float cameraFov;

uint querySVO(vec3 position, uint rootIndex) {
    // Query the SVO starting at rootIndex and return some value
    // This is a placeholder; you'll need to implement this based on how your SVO data is stored
    return svo[rootIndex];
}

float calculateStepSize(vec3 rayPos, uint rootIndex) {
    // Calculate the step size based on the current SVO or child SVO
    // This is a placeholder; you'll need to implement this based on how your SVO data is stored
    // For example, you might look up the size of the current node in the SVO and use that as the step size
    return svo[rootIndex].size;
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
        uint rootIndex = grid[int(rayPos.x) + int(rayPos.y) * gridSize + int(rayPos.z) * gridSize * gridSize];
        uint density = querySVO(rayPos + rayDir * t, rootIndex);

        if (density > 0) {
            // We hit something!
            pixel = vec4(1.0, 0.0, 0.0, 1.0); // Set to red for demonstration
            break;
        }

        float stepSize = calculateStepSize(rayPos + rayDir * t, rootIndex);
        t += stepSize;
    }

    imageStore(destTex, storePos, pixel);
}