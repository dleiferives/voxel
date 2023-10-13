#version 430

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) writeonly uniform image2D destTex;


struct Block{
    uint voxels[64];
};

//layout (std430, binding = 1) buffer blockPages {
//    struct Block pages[]; // The actual SVO data
//};


//layout (std430, binding = 2) buffer IndirectionGrid {
/    uint grid[]; // Each element is an index into the SVOData buffer
//};



int mortonCode(int x, int y, int z)
{
    int result =0;
    result |=  ((z & 0x1) << 2) | ((y & 0x1 ) << 1)| ((x & 0x1)); // insert first order bits
    result |=  ((z & 0x2) << 2) | ((y & 0x2) << 1)| ((x & 0x2)) << 3; // shift up the second order bits
    result = (z | y | x) > 3 ? -result : result; // if z y x out of range, return negative
    return result;
}

int mortonCode(vec3 pos)
{
    return mortonCode(pos.x, pos.y, pos.z);
}

// set gridsize
const int gridSize = 64;

int getBlockIndex(vec3 pos)
{
    int x = int(pos.x);
    int y = int(pos.y);
    int z = int(pos.z);
    if(x == 1 && y == 1 && z == 1)
    {
        return 1;
    }
    return 0;
}

uint getBlockDensity(vec3 pos, int index)
{
    if (index ==0) return 0;
    // find if it is in the block
}

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float cameraFov;

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
        //uint rootIndex = grid[int(rayPos.x) + int(rayPos.y) * gridSize + int(rayPos.z) * gridSize * gridSize];
        //uint density = querySVO(rayPos + rayDir * t, rootIndex);
        uint BlockIndex = getBlockIndex(rayPos);
        uint density =

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