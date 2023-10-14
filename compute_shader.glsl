#version 430
#define VOX_PER_BLOCK 64
#define INDIRECTION_GRID_SIZE 16
#define SVO_ROOT_SIZE 32
#define NUMBER_OF_NODES 16
#define NUMBER_OF_VOXELS 2
#define MAX_STEPS 100
#define MAX_DEPTH 6
#define STACK_SIZE 16

vec4 debugDataV = vec4(0.0, 0.0, 0.0, 0.0);



layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) writeonly uniform image2D destTex;



struct Block{
    int voxels[64];
};

struct Node{
    bool hasData;
    uint index;
    bool isLeaf;
    uint children[8];
};

struct StackElement {
    Node node;
    vec3 rayPos;
    float voxelSize;
    int steps;
    ivec2 returnValue;
    bool hasReturnValue;
};

//layout (std430, binding = 1) buffer blockPages {
//    struct Block pages[]; // The actual SVO data
//};

layout (std430, binding = 1) buffer svoData {
    uint blocksData[];
};

//layout (std430, binding = 2) buffer IndirectionGrid {
//    uint grid[]; // Each element is an index into the SVOData buffer
//};

layout(std430, binding = 3) buffer DebugDataBuffer {
    float debugData[];
};

int IndirectionGrid[INDIRECTION_GRID_SIZE];
vec4 NodesLocationsBuffer[NUMBER_OF_NODES];
Node NodesBuffer[NUMBER_OF_NODES];
vec4 VoxelTypes[NUMBER_OF_VOXELS];

int mortonCode(int x, int y, int z)
{
    x = x-2;
    z = z-2;

    if (x < 0 || x > 3 || y < 0 || y > 3 || z < 0 || z > 3)
    {
        return -1;
    }
    int result =0;
    result |=  ((z & 0x1) << 2) | ((y & 0x1 ) << 1)| ((x & 0x1)); // insert first order bits
    result |=  ((z & 0x2) << 2) | ((y & 0x2) << 1)| ((x & 0x2)) << 3; // shift up the second order bits
//    result = (z | y | x) > 3 ? -result : result; // if z y x out of range, return negative
    return result;
}

int getBlockDensity(vec3 pos, uint index)
{
    if (index == 0) return 0;
    // find if it is in the block
    int x = int((pos.x));
    int y = int((pos.y));
    int z = int((pos.z));
    int morton = mortonCode(x, y, z);
    return -morton;
    if (morton < 0) return 0;
    if (morton > 63) return 0;
    if (blocksData[(index * VOX_PER_BLOCK) + morton] == 1) return 1;
    return 0;
}

float getStep(vec3 rayPos, vec3 rayDir, float voxelSize, int steps)
{
    if(steps > MAX_STEPS)
    {
        return -1;
    }
    int wall_x = int(floor(rayPos.x / voxelSize) * voxelSize);
    int wall_y = int(floor(rayPos.y / voxelSize) * voxelSize);
    int wall_z = int(floor(rayPos.z / voxelSize) * voxelSize);
    if (rayDir.x > 0) wall_x += int(voxelSize);
    if (rayDir.y > 0) wall_y += int(voxelSize);
    if (rayDir.z > 0) wall_z += int(voxelSize);
    //TODO: optimize this division
    if(rayDir.x == 0) rayDir.x = 0.0000001;
    if(rayDir.y == 0) rayDir.y = 0.0000001;
    if(rayDir.z == 0) rayDir.z = 0.0000001;
    float nx = (wall_x - rayPos.x) / rayDir.x;
    float ny = (wall_y - rayPos.y) / rayDir.y;
    float nz = (wall_z - rayPos.z) / rayDir.z;

    // find if x y or z is min
    float min = 100000000;
    if(nx < ny && nx < nz)
    {
        min= nx;
    }
    else if(ny < nx && ny < nz)
    {
        min= ny;
    }
    else
    {
        min= nz;
    }

    min = min + 0.0000001;
    return min;
}


int findChildIndex(vec3 rayPos, float voxelSize) {
    int child_index = 0;
    // are we in the first half of x axis?
    float x_min = floor(rayPos.x / voxelSize) * voxelSize;
    float y_min = floor(rayPos.y / voxelSize) * voxelSize;
    float z_min = floor(rayPos.z / voxelSize) * voxelSize;

    float x_max = x_min + voxelSize;
    float y_max = y_min + voxelSize;
    float z_max = z_min + voxelSize;

    float x_mid = x_min + voxelSize / 2;
    float y_mid = y_min + voxelSize / 2;
    float z_mid = z_min + voxelSize / 2;

    if (rayPos.x > x_mid)
    {
        child_index += 1;
    }
    if (rayPos.y > y_mid)
    {
        child_index += 2;
    }
    if (rayPos.z > z_mid)
    {
        child_index += 4;
    }
    return child_index;
}

ivec2 marchThroughSvo(Node root, vec3 rayPos, vec3 rayDir, float voxelSize, int steps) {
    //return ivec2(-1, 10);
    StackElement stack[MAX_DEPTH];
    int top = 0;

    stack[top].node = root;
    stack[top].rayPos = rayPos;
    stack[top].voxelSize = voxelSize;
    stack[top].steps = steps;
    stack[top].hasReturnValue = false;
    top++;

    while (top > 0) {
        // Pop an element from the stack
        top--;
        StackElement elem = stack[top];

        if (elem.hasReturnValue) {
            if (elem.returnValue.x != -1) {
                return elem.returnValue;
            }
            continue;
        }

        Node node = elem.node;
        rayPos = elem.rayPos;
        voxelSize = elem.voxelSize;
        steps = elem.steps;

        if (node.isLeaf) {
            debugData[2] = 1;
            int density = getBlockDensity(rayPos, node.index);
            if (density != 0u) {
                return ivec2(density, steps);
            }

            float step_block = getStep(rayPos, rayDir, voxelSize / 4.0, steps);
            if (step_block == -1.0) {
                continue;
            }

            while(true) {
                density = getBlockDensity(rayPos + rayDir * step_block, node.index);
                if (density != 0u) {
                    return ivec2(int(density), steps);
                }

                float temp_step = getStep(rayPos + (rayDir * step_block), rayDir, voxelSize / 4.0, steps);
                if (temp_step == -1.0) {
                    break;
                }
                step_block += temp_step;

                if (step_block > voxelSize) {
                    break;
                }
            }
        } else {
            int child_index = findChildIndex(rayPos, voxelSize);
            if (node.children[child_index] == -1) {
                continue;
            }

            if (top >= MAX_DEPTH) {
                // Stack overflow, handle it as per your requirements
                break;
            }

            // Re-push the current node with a flag indicating we should handle its return value next
            stack[top] = elem;
            stack[top].hasReturnValue = true;
            top++;

            // Then push the child node to visit
            stack[top].node = NodesBuffer[node.children[child_index]];
            stack[top].rayPos = rayPos;
            stack[top].voxelSize = voxelSize / 2.0;
            stack[top].steps = steps;
            stack[top].hasReturnValue = false;
            top++;
        }
    }

    return ivec2(-6, steps);
}

ivec2 marchThroughSpace(vec3 rayPos, vec3 rayDir)
{
//    return ivec2(-1, 0);
    vec3 pos = rayPos;
    int steps = 0;
    int depth = 0;
    while(++depth < 10)
    {
        // find which svo we are in and march through it
        for(int i =0; i < INDIRECTION_GRID_SIZE; i++)
        {
            if(IndirectionGrid[i] == -1)
            {
                continue;
            }
            int index = IndirectionGrid[i];
            if(NodesLocationsBuffer[index].x < pos.x && NodesLocationsBuffer[index].x + NodesLocationsBuffer[index].w  > pos.x &&
            NodesLocationsBuffer[index].y < pos.y && NodesLocationsBuffer[index].y + NodesLocationsBuffer[index].w  > pos.y &&
            NodesLocationsBuffer[index].z < pos.z && NodesLocationsBuffer[index].z + NodesLocationsBuffer[index].w  > pos.z)
            {
                ivec2 march_result= marchThroughSvo(NodesBuffer[index], pos, rayDir, NodesLocationsBuffer[index].w ,steps);
                if(march_result.x == -1)
                {
                    debugData[2] = 2;
                    steps += march_result.y;
                    float step = getStep(pos, rayDir, NodesLocationsBuffer[index].w, steps);
                    if (step == -1)
                    {
                        return ivec2(-3, steps);
                    }
                    steps += 1;
                    pos += rayDir * step;
                    break;
                }
                return march_result;
            }
//            if(i == INDIRECTION_GRID_SIZE -1)
//            {
//                return ivec2(-5, steps);
//            }
        }
//        return ivec2(-4, steps);
    }
    return ivec2(-2, steps);
}

int mortonCode(vec3 pos)
{
    return mortonCode(int(pos.x), int(pos.y), int(pos.z));
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
    for(int i =0; i < INDIRECTION_GRID_SIZE; i++)
    {
        IndirectionGrid[i] = -1;
    }

    IndirectionGrid[0] = 0;
    NodesLocationsBuffer[0] = vec4(2, 2, 2, 4);
    Node node;
    node.hasData = true;
    node.index = 0;
    node.isLeaf = true;

    NodesBuffer[0] = node;
    VoxelTypes[0] = vec4(1.0, 0, 0, 1.0); // red

    cameraRight = normalize(cross(cameraDir, worldUp));
    cameraUp = normalize(cross(cameraRight, cameraDir));
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    vec2 fragCoord = vec2(storePos) / vec2(640, 480); // Assuming a 640x480 resolution

    float fovScale = tan(cameraFov * 0.5);
    vec3 rayDir = normalize(cameraDir +
                            (fragCoord.x - 0.5) * cameraRight * fovScale +
                            (fragCoord.y - 0.5) * cameraUp * fovScale);

    vec3 rayPos = cameraPos;
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0); // Default to black

    bool hit = false;
    // Perform ray marching

    ivec2 march_result = marchThroughSpace(rayPos, rayDir);
    debugDataV = vec4(float(march_result.x), float(march_result.y), 0.0, 1.0);
    int steps = march_result.y;
    if(march_result.x == -1)
    {
        // pixel color to green
        pixel = vec4(0.0, 1.0, 0.0, 1.0);
//        pixel = VoxelTypes[march_result.x];
//        hit = true;
    }
            if(march_result.x == -2)#version 430

layout (local_size_x = 16, local_size_y = 16) in;

    layout (rgba32f, binding = 0) writeonly uniform image2D destTex;


    struct Block{
        uint voxels[64];
    };

    //layout (std430, binding = 1) buffer blockPages {
    //    struct Block pages[]; // The actual SVO data
    //};

    layout (std430, binding = 1) buffer svoData {
        uint data[];
    };


    //layout (std430, binding = 2) buffer IndirectionGrid {
    //    uint grid[]; // Each element is an index into the SVOData buffer
    //};



    int mortonCode(int x, int y, int z)
    {
        x = x-2;
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
    //struct Block block1;
    //uint svo[64] = uint[](0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);

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
        if (data[morton] == 1) return 1;
        return 0;
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
        //block1.voxels = svo;
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

        // mix the pixel color with black based on depth (t)
        pixel = mix(pixel, vec4(0.0, 0.0, 0.0, 1.0), t * 0.1);


        imageStore(destTex, storePos, pixel);
    }
    {
        // pixel color to blue
        pixel = vec4(0.0, 0.0, 1.0, 1.0);
    }
    else
    {
        hit = false;
    }
//    pixel = vec4(abs(march_result.x) / 64.0, 0.0, 0.0, 1.0);

    if(!hit)
    {
        pixel = vec4(normalize(rayDir), 1.0);
    }

    // mix the pixel color with black based on depth (t)
//    pixel = mix(pixel, vec4(0.0, 0.0, 0.0, 1.0), steps * 0.1);

    debugData[0] = float(march_result.x);
    debugData[1] = float(march_result.y);
    debugData[3] = 1.0;
    imageStore(destTex, storePos, pixel);
}