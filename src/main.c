///////////////////////////////////////////////////////////////////////////////
/// Copyright: Dylan Leifer-Ives 2024
/// Using Gunslinger.
/// Based off: 
/// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-4-a-colored-cube/
///////////////////////////////////////////////////////////////////////////////
#define GS_IMPL
#include "../include/gs/gs.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "../include/gs/util/gs_idraw.h"

#ifdef GS_PLATFORM_WEB
    #define GS_GL_VERSION_STR "#version 300 es\n"
#else
    #define GS_GL_VERSION_STR "#version 430 core\n"
#endif


#define voxels 16384
#define voxels_sub 128

// FPS CAMERA //////////////////////////////////////////////////////////////////
#define SENSITIVITY 0.1f
#define CAM_SPEED 10.f

typedef struct fps_camera_t {
    float pitch;
    float bob_time;
    gs_camera_t cam;
} fps_camera_t;

fps_camera_t         fps = {0};
void fps_camera_update(fps_camera_t* cam);

#define rand_range(MIN, MAX)\
    (rand() % (MAX - MIN + 1) + MIN)

// DATA ////////////////////////////////////////////////////////////////////////

float *v_cube;
// float v_cube[] = {
//     -1.0f,-1.0f,-1.0f, // triangle 1 : begin
//     -1.0f,-1.0f, 1.0f,
//     -1.0f, 1.0f, 1.0f, // triangle 1 : end
//     1.0f, 1.0f,-1.0f, // triangle 2 : begin
//     -1.0f,-1.0f,-1.0f,
//     -1.0f, 1.0f,-1.0f, // triangle 2 : end
//     1.0f,-1.0f, 1.0f,
//     -1.0f,-1.0f,-1.0f,
//     1.0f,-1.0f,-1.0f,
//     1.0f, 1.0f,-1.0f,
//     1.0f,-1.0f,-1.0f,
//     -1.0f,-1.0f,-1.0f,
//     -1.0f,-1.0f,-1.0f,
//     -1.0f, 1.0f, 1.0f,
//     -1.0f, 1.0f,-1.0f,
//     1.0f,-1.0f, 1.0f,
//     -1.0f,-1.0f, 1.0f,
//     -1.0f,-1.0f,-1.0f,
//     -1.0f, 1.0f, 1.0f,
//     -1.0f,-1.0f, 1.0f,
//     1.0f,-1.0f, 1.0f,
//     1.0f, 1.0f, 1.0f,
//     1.0f,-1.0f,-1.0f,
//     1.0f, 1.0f,-1.0f,
//     1.0f,-1.0f,-1.0f,
//     1.0f, 1.0f, 1.0f,
//     1.0f,-1.0f, 1.0f,
//     1.0f, 1.0f, 1.0f,
//     1.0f, 1.0f,-1.0f,
//     -1.0f, 1.0f,-1.0f,
//     1.0f, 1.0f, 1.0f,
//     -1.0f, 1.0f,-1.0f,
//     -1.0f, 1.0f, 1.0f,
//     1.0f, 1.0f, 1.0f,
//     -1.0f, 1.0f, 1.0f,
//     1.0f,-1.0f, 1.0f,
    // 0.583f,  0.771f,  0.014f,
    // 0.609f,  0.115f,  0.436f,
    // 0.327f,  0.483f,  0.844f,
    // 0.822f,  0.569f,  0.201f,
    // 0.435f,  0.602f,  0.223f,
    // 0.310f,  0.747f,  0.185f,
    // 0.597f,  0.770f,  0.761f,
    // 0.559f,  0.436f,  0.730f,
    // 0.359f,  0.583f,  0.152f,
    // 0.483f,  0.596f,  0.789f,
    // 0.559f,  0.861f,  0.639f,
    // 0.195f,  0.548f,  0.859f,
    // 0.014f,  0.184f,  0.576f,
    // 0.771f,  0.328f,  0.970f,
    // 0.406f,  0.615f,  0.116f,
    // 0.676f,  0.977f,  0.133f,
    // 0.971f,  0.572f,  0.833f,
    // 0.140f,  0.616f,  0.489f,
    // 0.997f,  0.513f,  0.064f,
    // 0.945f,  0.719f,  0.592f,
    // 0.543f,  0.021f,  0.978f,
    // 0.279f,  0.317f,  0.505f,
    // 0.167f,  0.620f,  0.077f,
    // 0.347f,  0.857f,  0.137f,
    // 0.055f,  0.953f,  0.042f,
    // 0.714f,  0.505f,  0.345f,
    // 0.783f,  0.290f,  0.734f,
    // 0.722f,  0.645f,  0.174f,
    // 0.302f,  0.455f,  0.848f,
    // 0.225f,  0.587f,  0.040f,
    // 0.517f,  0.713f,  0.338f,
    // 0.053f,  0.959f,  0.120f,
    // 0.393f,  0.621f,  0.362f,
    // 0.673f,  0.211f,  0.457f,
    // 0.820f,  0.883f,  0.371f,
    // 0.982f,  0.099f,  0.879f
// };

float g_translations[voxels] = {0};

const char* fragment_shader = 
        GS_GL_VERSION_STR 
        "precision mediump float;\n"
        "in vec3 f_color;\n"
        "out vec3 color;\n"
        "void main(){\n"
        "       color = f_color;\n"
        "}\n";

const char* vertex_shader = 
        GS_GL_VERSION_STR
        "precision mediump float;\n"
        "layout(location = 0) in vec3 a_pos;\n"
        // "layout(location = 1) in vec3 a_color;\n"
        // "layout(location = 2) in float a_offset;\n"
        "out vec3 f_color;\n"
        "layout (std140) uniform ub_vp {\n"
        "   mat4 projection;\n"
        "   mat4 view;\n"
        "};\n"
        "void main(){\n" // Note that the model position is the identity matrix for a mat4
        // "float x = float(gl_InstanceID % 128) * 2.0 - (128.0 / 2.0);\n"  // Modulo by 16 and scale
        // "float z = float(gl_InstanceID / 128) * 2.0 - (128.0 / 2.0);\n"  // Divide by 16 and scale
        // "float distance = sqrt(pow(x, 2.0) + pow(z, 2.0));\n"
        // "vec3 pos = vec3(a_pos.x + x , (a_pos.y + a_offset), a_pos.z + z );\n"
        "   gl_Position = projection * view * mat4(1.0) *  vec4(a_pos, 1.0);\n"
        "   f_color = vec3(0.5,0.5,0.5);\n"
        "}\n";

gs_command_buffer_t                    command_buffer = {0};
gs_handle(gs_graphics_pipeline_t)            pipeline = {0};
gs_handle(gs_graphics_shader_t)                shader = {0};
//gs_camera_t                                       cam = {0};

gs_handle(gs_graphics_vertex_buffer_t)   vbo_cube = {0};
gs_handle(gs_graphics_vertex_buffer_t)   vbo_cubemap = {0};
gs_handle(gs_graphics_vertex_buffer_t)   vbo_color= {0};
gs_handle(gs_graphics_uniform_buffer_t)         ub_vp = {0};
gs_handle(gs_graphics_vertex_buffer_t)  inst_vbo = {0};
gs_immediate_draw_t  gsi = {0};

typedef struct v_viewproj_t{
        gs_mat4 projection;
        gs_mat4 view;
} v_viewproj_t;



float Cube_vertices[] = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
};

typedef struct {
	gs_vec3 pos;
	int num_verts;
} Cube_t;

typedef struct{
	Cube_t *cubes;
	int num_cubes;
}CubeMap_t;
CubeMap_t *cubemap = NULL;

#define CUBE_T_VERTS 36
Cube_t Cube_init(gs_vec3 pos) {
	return (Cube_t){pos, CUBE_T_VERTS};
}

float *Cube_mesh_many(CubeMap_t cubemap){
	float *vertices = (float *)malloc(cubemap.num_cubes * CUBE_T_VERTS * 3 * sizeof(float));
	for(int i = 0; i < cubemap.num_cubes; i++){
		for(int j = 0; j < CUBE_T_VERTS; j++){
			vertices[(i * CUBE_T_VERTS * 3) + j * 3 + 0] = Cube_vertices[j * 3 + 0] + cubemap.cubes[i].pos.x;
			vertices[i * CUBE_T_VERTS * 3 + j * 3 + 1] = Cube_vertices[j * 3 + 1] + cubemap.cubes[i].pos.y;
			vertices[i * CUBE_T_VERTS * 3 + j * 3 + 2] = Cube_vertices[j * 3 + 2] + cubemap.cubes[i].pos.z;
		}
	}
	return vertices;
}

CubeMap_t *Cube_create_3d_sin_of_n(int radius, float height_scalar){
	// figure out how many cubes we need, since each cube is 2x2x2 
	int num_cubes = (radius * 2) * (radius * 2);
	CubeMap_t *cubemap = (CubeMap_t *)malloc(sizeof(CubeMap_t));
	cubemap->num_cubes = num_cubes;
	float offset_x = (float) radius;
	float offset_z = (float) radius;
	cubemap->cubes = (Cube_t *)malloc(num_cubes * sizeof(Cube_t));
	for(int i = 0; i < radius * 2; i++){
		for(int j = 0; j < radius * 2; j++){
			cubemap->cubes[(i * radius * 2) + j] = Cube_init(gs_v3(i - offset_x, sin(i) * height_scalar, j - offset_z));
		}
	}
	return cubemap;
}

GS_API_DECL gs_handle(gs_graphics_vertex_buffer_t) 
CubeMap_to_vbo(CubeMap_t *cubemap){
	float *vertices = Cube_mesh_many(*cubemap);
	return gs_graphics_vertex_buffer_create(
		&(gs_graphics_vertex_buffer_desc_t){
			.data = vertices,
			.size = cubemap->num_cubes * CUBE_T_VERTS * 3 * sizeof(float)
		}
	);
}




// FORWARD FUNCTION DECLARATIONS ///////////////////////////////////////////////
void app_init();
void app_update();


// FUNCTIONS ///////////////////////////////////////////////////////////////////

void app_init(){

        // Set up our command buffer to submit to gpu
        command_buffer = gs_command_buffer_new();


        // set up immediate mode gui
        gsi = gs_immediate_draw_new(gs_platform_main_window());

        // Set up the camera
        fps.cam = gs_camera_perspective();
        // for(int i = 0; i < voxels; ++i) {
        //     g_translations[i] = ((float)i)/((float)voxels) * 100.0f;
        // }
       
        // Set up instancing
        // inst_vbo = gs_graphics_vertex_buffer_create(
        //                 &(gs_graphics_vertex_buffer_desc_t) {
        //                 .data = &g_translations[0],
        //                 .size = sizeof(g_translations)
        //                 }
        // );

		v_cube = malloc(sizeof(float) * 3 * 36);
		// create cube
		Cube_t cube = Cube_init(gs_v3(0, 0, 0));
		for(int i = 0; i < CUBE_T_VERTS; i++){
			v_cube[i * 3 + 0] = Cube_vertices[i * 3 + 0] + cube.pos.x;
			v_cube[i * 3 + 1] = Cube_vertices[i * 3 + 1] + cube.pos.y;
			v_cube[i * 3 + 2] = Cube_vertices[i * 3 + 2] + cube.pos.z;
		}

        // Set up our vertex buffer 
        vbo_cube= gs_graphics_vertex_buffer_create(
                &(gs_graphics_vertex_buffer_desc_t) {
                        .data = v_cube,
                        .size = sizeof(float) * 3 * 36
                }
        );

        // Create uniform buffer
        ub_vp = gs_graphics_uniform_buffer_create(
                &(gs_graphics_uniform_buffer_desc_t){
                        .data = NULL,
                        .size = sizeof(v_viewproj_t),
                        .name = "ub_vp"
                }
        );

        // set up uniform buffer for view projection
        gs_vec2 window_size = gs_platform_window_sizev(gs_platform_main_window());
        gs_mat4 projection = gs_camera_get_proj(
                &(fps.cam),
                (uint32_t)window_size.x,
                (uint32_t)window_size.y
        );

        // load the uniform buffer into the comand buffer
        // note that this will only happen once / maybe once per resize, since
        // this is just setting up the projection, which is only pased on the 
        // window size 
        gs_graphics_uniform_buffer_request_update(

                &command_buffer,
                ub_vp,
                &(gs_graphics_uniform_buffer_desc_t){
                        .data = &projection,
                        .size = sizeof(projection),
                        .update = {
                                .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
                                .offset = 0
                        }
                }
        );

        // set up shaders
        shader = gs_graphics_shader_create(
                &(gs_graphics_shader_desc_t) {
                        .sources = (gs_graphics_shader_source_desc_t[]){
                                {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = vertex_shader},
                                {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = fragment_shader},
                        },
                        .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
                        .name = "triangle_shader"
                }
        );

        gs_graphics_vertex_attribute_desc_t vattrs[] = {
                (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos", .buffer_idx = 0}, // Position
                // (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_color", .buffer_idx = 1}, // Color
                // (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_offset", .stride = sizeof(float), .offset = 0, .divisor = 1, .buffer_idx = 2},    // Offset (stride of total index vertex data, divisor is 1 for instance iteration)
        };

        // Set up pipeline
        pipeline = gs_graphics_pipeline_create(
                &(gs_graphics_pipeline_desc_t) {
                        .raster = {.shader = shader},
                        .depth = {
                                .func = GS_GRAPHICS_DEPTH_FUNC_LESS
                        },
                        .layout = {
                                .attrs = vattrs, 
                                .size = sizeof(vattrs)
                        }
                        
                        
                }
        );
		
		cubemap =  Cube_create_3d_sin_of_n(10, 10.0f);
		vbo_cubemap = CubeMap_to_vbo(cubemap);

}

void app_update(){
        if (gs_platform_key_pressed(GS_KEYCODE_ESC)) gs_quit();
        // Set up the fram buffer size
        gs_vec2 fs = gs_platform_framebuffer_sizev(gs_platform_main_window());

        // clear the screen 
        gs_graphics_clear_desc_t clear = {.actions = &(gs_graphics_clear_action_t){.color = {0.2f, 0.2f, 0.2f, 1.f}}};


        if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked()) {
                gs_platform_lock_mouse(gs_platform_main_window(), true);
        }

        // Update camera
        if (gs_platform_mouse_locked()) {
                fps_camera_update(&fps);
        }

        // Camera view
        // TODO: update with a fps camera
        gs_mat4 view = gs_camera_get_view(&(fps.cam));

        // Request view update once per frame!
        gs_graphics_uniform_buffer_request_update(
                &command_buffer,
                ub_vp,
                &(gs_graphics_uniform_buffer_desc_t){
                        .data = &view,
                        .size = sizeof(view),
                        .update = {
                                .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
                                .offset = sizeof(gs_mat4)
                        }
                }
        );


        static int counter;
                       
		
        gs_graphics_bind_vertex_buffer_desc_t v_buffers[] = {
                (gs_graphics_bind_vertex_buffer_desc_t){.buffer = vbo_cube, .offset = 0},
                // NOTE: the offset needs to be the index of the color data
                // (gs_graphics_bind_vertex_buffer_desc_t){.buffer = vbo_cube, .offset = 36 * 3 * sizeof(float), .data_type=GS_GRAPHICS_VERTEX_DATA_NONINTERLEAVED},
                // {.buffer = inst_vbo},
				// {.buffer = vbo_cubemap},
        };

        // Render //
        gs_graphics_renderpass_begin(&command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT);
                gs_graphics_set_viewport(&command_buffer, 0,0, (uint32_t)fs.x, (uint32_t)fs.y );
                // using the clear action
                gs_graphics_clear(&command_buffer, &clear);

                gs_graphics_bind_desc_t binds = {
                        .vertex_buffers = {
                                .desc = v_buffers ,
                                .size = sizeof(v_buffers) 
                        },
                        .uniform_buffers = {
                                &(gs_graphics_bind_uniform_buffer_desc_t){
                                        .buffer = ub_vp,
                                        .binding = 0
                                }
                        },
                };

                gs_graphics_pipeline_bind(&command_buffer,pipeline);
                gs_graphics_apply_bindings(&command_buffer, &binds);
                gs_graphics_draw(&command_buffer, 
                                 &(gs_graphics_draw_desc_t){
                                        .start = 0,
                                        // note count needs to be for the number of
                                        // vertexes, not the number of floats
                                        .count =  36
									 // .instances = voxels 
                                 }
                );
        gs_graphics_renderpass_end(&command_buffer);

        // Draw text
        gsi_defaults(&gsi);
        gsi_camera2D(&gsi, fs.x, fs.y);
        gsi_rectvd(&gsi, gs_v2(10.f, 10.f), gs_v2(220.f, 70.f), gs_v2(0.f, 0.f), gs_v2(1.f, 1.f), gs_color(10, 50, 150, 255), GS_GRAPHICS_PRIMITIVE_TRIANGLES);
        gsi_rectvd(&gsi, gs_v2(10.f, 10.f), gs_v2(220.f, 70.f), gs_v2(0.f, 0.f), gs_v2(1.f, 1.f), gs_color(10, 50, 220, 255), GS_GRAPHICS_PRIMITIVE_LINES);
        const char render_str[64];
        float render= gs_platform_time()->render;
        sprintf(render_str, "Render: %.2f", render);
        const char delta_str[64];
        float delta= gs_platform_time()->delta;
        const char frame_str[64];
        sprintf(delta_str, "Delta: %.2f", delta);
        float frame= gs_platform_time()->frame;
        sprintf(frame_str, "F: %.2f", frame);
        gsi_text(&gsi, 20.f, 25.f, "FPS Camera Controls:", NULL, false, 0, 0, 0, 255);
        gsi_text(&gsi, 40.f, 40.f, "- Move: W, A, S, D", NULL, false, 255, 255, 255, 255);
        gsi_text(&gsi, 40.f, 55.f, "- Mouse to look", NULL, false, 255, 255, 255, 255);
        gsi_text(&gsi, 40.f, 70.f, "- Shift to run", NULL, false, 255, 255, 255, 255);
        gsi_text(&gsi, 40.f, 90.f, render_str, NULL, false, 255, 255, 255, 255);
        gsi_text(&gsi, 40.f, 110.f, delta_str, NULL, false, 255, 255, 255, 255);
        gsi_text(&gsi, 40.f, 130.f, frame_str, NULL, false, 255, 255, 255, 255);

    /* Render */
    //gsi_renderpass_submit(&gsi, &command_buffer, gs_v4(0.f, 0.f, fs.x, fs.y), gs_color(0,0,0,0));
        // Taken out of renderpass submit
	gs_renderpass_t pass = gs_default_val();
	gs_graphics_renderpass_begin(&command_buffer, pass);
        gs_vec4 viewport = gs_v4(0.f, 0.f, fs.x, fs.y);
	gs_graphics_set_viewport(&command_buffer, (uint32_t)viewport.x, (uint32_t)viewport.y, (uint32_t)viewport.z, (uint32_t)viewport.w);
	gsi_draw(&gsi, &command_buffer);
	gs_graphics_renderpass_end(&command_buffer);

        gs_graphics_command_buffer_submit(&command_buffer);
}


gs_app_desc_t gs_main(int32_t argc, char** argv)
{
    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update
    };
}


void fps_camera_update(fps_camera_t* fps)
{
    gs_platform_t* platform = gs_subsystem(platform);

    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), SENSITIVITY);
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL) ? 2.f : 1.f;
    float dt = platform->time.delta;
    float old_pitch = fps->pitch;

    // Keep track of previous amount to clamp the camera's orientation
    fps->pitch = gs_clamp(old_pitch + dp.y, -90.f, 90.f);

    // Rotate camera
    gs_camera_offset_orientation(&fps->cam, -dp.x, old_pitch - fps->pitch);

    gs_vec3 vel = {0};
    if (gs_platform_key_down(GS_KEYCODE_W)) vel = gs_vec3_add(vel, gs_camera_forward(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_S)) vel = gs_vec3_add(vel, gs_camera_backward(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_A)) vel = gs_vec3_add(vel, gs_camera_left(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_D)) vel = gs_vec3_add(vel, gs_camera_right(&fps->cam));
if (gs_platform_key_down(GS_KEYCODE_SPACE)) vel.y += 1.f;
        if (gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT)) vel.y -= 1.f;

    fps->cam.transform.position = gs_vec3_add(fps->cam.transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * CAM_SPEED * mod));

}
