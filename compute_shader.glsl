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
//    uint grid[]; // Each element is an index into the SVOData buffer
//};



int mortonCode(int x, int y, int z)
{
    x = x-2;
    y = y-2;
    z = z-2;

    if (x < 0 || x > 3 || y < 0 || y > 3 || z < 0 || z > 3)
    {
        return -1;
    }
    int result =0;
    result |=  ((z & 0x1) << 2) | ((y & 0x1 ) << 1)| ((x & 0x1)); // insert first order bits
    result |=  ((z & 0x2) << 2) | ((y & 0x2) << 1)| ((x & 0x2)) << 3; // shift up the second order bits
    result = (z | y | x) > 3 ? -result : result; // if z y x out of range, return negative
    return result;
}

int mortonCode(vec3 pos)
{
    return mortonCode(int(pos.x), int(pos.y), int(pos.z));
}

// set gridsize
const int gridSize = 64;
// set up a constant block for testing
struct Block block1;
uint svo[64] = uint[64](0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);

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

uint getBlockDensity(vec3 pos, uint index)
{
    if (index == 0) return 0;
    // find if it is in the block
    int x = int((pos.x));
    int y = int((pos.y));
    int z = int((pos.z));
    int morton = mortonCode(x, y, z);
    if (morton < 0) return 0;
    if (morton > 63) return 0;
    return 1;
    uint res = svo[morton];
    return res;
}

uniform vec3 cameraPos;
uniform vec3 cameraDir;
//uniform vec3 cameraUp;
//uniform vec3 cameraRight;
uniform float cameraFov;
uniform vec3 worldUp;

vec3 cameraRight;
vec3 cameraUp;

void main() {
    //cameraDir = normalize(cameraDir);
    cameraRight = normalize(cross(cameraDir, worldUp));
    cameraUp = normalize(cross(cameraRight, cameraDir));
    block1.voxels = svo;
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

    bool hit = false;
    // Perform ray marching
    for (int i = 0; i < 100; ++i) { // Limit the number of steps to avoid infinite loops
        //uint rootIndex = grid[int(rayPos.x) + int(rayPos.y) * gridSize + int(rayPos.z) * gridSize * gridSize];
        //uint density = querySVO(rayPos + rayDir * t, rootIndex);
        uint BlockIndex = 1;
        uint density = getBlockDensity(rayPos + (rayDir * t), BlockIndex);



        if (density > 0) {
            // We hit something!
            pixel = vec4(1.0, 0.0, 0.0, 1.0); // Set to red for demonstration
            hit = true;
            break;
        }
        if (rayPos.y + rayDir.y * t < 0.0) {
                    // We hit the ground plane!

                    // Determine the square color based on the x and z position
                    bool xEven = int(floor((rayPos.x + rayDir.x * t))) % 2 == 0;
                    bool zEven = int(floor((rayPos.z + rayDir.z * t))) % 2 == 0;

                    if (xEven == zEven) {
                        pixel = vec4(1.0, 1.0, 1.0, 1.0);  // White square
                    } else {
                        pixel = vec4(0.0, 0.0, 0.0, 1.0);  // Black square
                    }

                    hit = true;
                    break;
                }


        t += stepSize;
    }

    if(!hit)
    {
        pixel = vec4(normalize(rayDir), 1.0);
    }


    imageStore(destTex, storePos, pixel);
}