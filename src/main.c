///////////////////////////////////////////////////////////////////////////////
/// Copyright: Dylan Leifer-Ives 2024
/// File: main.c
/// Project: Voxel
////////////////////////////////////////////////////////////////////////////////
#include "main.h";

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


// FIXME: load from ./shaders/fragment.glsl
char* fragment_shader = NULL;

// FIXME: load from ./shaders/vertex.glsl
const char* vertex_shader = NULL;


err_t Program_load_shaders(Program_t *program){
	// Load fragment shader
	// FIXME: this assumes that fragment.glsl is showrter than 1024 bytes
	program->fragment_shader = (char *)malloc(1024 * sizeof(char));
	FILE *f = fopen(VOXEL_SHADER_FRAGMENT_FILE, "r");
	if(f == NULL){
		fprintf(stderr, "Failed to open fragment shader\n");
		return 1;
	}
	fread(program->fragment_shader, 1024, 1, f);
	fclose(f);
	// Load vertex shader
	// FIXME: this assumes that vertex.glsl is showrter than 1024 bytes
	program->vertex_shader = (char *)malloc(1024 * sizeof(char));
	f = fopen(VOXEL_SHADER_VERTEX_FILE, "r");
	if(f == NULL){
		fprintf(stderr, "Failed to open vertex shader\n");
		return 2;
	}
	fread(program->vertex_shader, 1024, 1, f);
	fclose(f);

	// overwrite the starts of the strings with what we actually want
	for(int i =0; i<GS_GL_VERSION_STR_LEN; i++){
		program->vertex_shader[i] = GS_GL_VERSION_STR[i];
		program->fragment_shader[i] = GS_GL_VERSION_STR[i];
	}
	return 0;
}


Cube_t Cube_init(gs_vec3 pos) {
	return (Cube_t){pos, CUBE_T_VERTS_COUNT};
}
float *g_verts; 

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

float *CubeMap_remesh(CubeMap_t *cubemap, float *verts){
	for(int i = 0; i < cubemap->num_cubes; i++){
		for(int j = 0; j < CUBE_T_VERTS; j++){
			verts[(i * CUBE_T_VERTS * 3) + j * 3 + 0] = Cube_vertices[j * 3 + 0] + cubemap->cubes[i].pos.x;
			verts[i * CUBE_T_VERTS * 3 + j * 3 + 1] = Cube_vertices[j * 3 + 1] + cubemap->cubes[i].pos.y;
			verts[i * CUBE_T_VERTS * 3 + j * 3 + 2] = Cube_vertices[j * 3 + 2] + cubemap->cubes[i].pos.z;
		}
	}
	return verts;
}

CubeMap_t *CubeMap_create_big_box( int width, int height, int depth) {
	int num_cubes = height * width * depth;
	CubeMap_t *cubemap = (CubeMap_t *)malloc(sizeof(CubeMap_t));
	cubemap->num_cubes = num_cubes;
	cubemap->cubes = (Cube_t *)malloc(num_cubes * sizeof(Cube_t));
	for(int i = 0; i < width; i++){
		for(int j = 0; j < height; j++){
			for(int k = 0; k < depth; k++){
				cubemap->cubes[(i * height * depth) + j * depth + k] = Cube_init(gs_v3(i, j, k));
			}
		}
	}
	return cubemap;
}

CubeMap_t *CubeMap_create_perlin_field(int width, int depth) {
	CubeMap_t *cubemap = CubeMap_create_big_box(1,width,depth);
	for(int i = 0; i < width; i++){
		for(int j = 0; j < depth; j++){
			float y = pnoise3d(i,j,0.5,1, 20, 112313);
			cubemap->cubes[i * width + j] = Cube_init(gs_v3(i, y,j));
		}
	}
	return cubemap;
}

GS_API_DECL gs_handle(gs_graphics_vertex_buffer_t) 
CubeMap_to_vbo(CubeMap_t *cubemap, float* verts){
return gs_graphics_vertex_buffer_create(
	&(gs_graphics_vertex_buffer_desc_t){
.data = verts,
.size = cubemap->num_cubes * CUBE_T_VERTS * 3 * sizeof(float)
}
);
}

CubeMap_t *CubeMap_update_perlin_field(CubeMap_t *cm, int width,int depth,int octaves, double persistance, int z, int scalar, float zoom){
	for(int i = 0; i < width; i++){
		for(int j = 0; j < depth; j++){
			float y = pnoise3d(i*zoom,j*zoom,z,persistance, octaves, 112313) * scalar;
			// floor y to nearest integer
			y = floor(y);
			cm->cubes[i * width + j] = Cube_init(gs_v3(i,y, j));
		}
	}
	return cm;
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
	cubemap = CubeMap_create_perlin_field(200, 200);
	g_verts = Cube_mesh_many(*cubemap);

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
	counter++;
	CubeMap_update_perlin_field(cubemap, 200, 200, 5, 0.5, counter>>8,5,0.1);
	vbo_cubemap = CubeMap_to_vbo(cubemap, g_verts);
	g_verts = CubeMap_remesh(cubemap, g_verts);


	gs_graphics_bind_vertex_buffer_desc_t v_buffers[] = {
		{.buffer = vbo_cubemap},
		// (gs_graphics_bind_vertex_buffer_desc_t){.buffer = vbo_cube, .offset = 0},
		// NOTE: the offset needs to be the index of the color data
		// (gs_graphics_bind_vertex_buffer_desc_t){.buffer = vbo_cube, .offset = 36 * 3 * sizeof(float), .data_type=GS_GRAPHICS_VERTEX_DATA_NONINTERLEAVED},
		// {.buffer = inst_vbo},
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
				  .count = cubemap->num_cubes * CUBE_T_VERTS,
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
	const char vert_str[64];
	float vert_num= cubemap->num_cubes * CUBE_T_VERTS;
	sprintf(vert_str, "Vertexes: %.2f", vert_num);
	gsi_text(&gsi, 20.f, 25.f, "FPS Camera Controls:", NULL, false, 0, 0, 0, 255);
	gsi_text(&gsi, 40.f, 40.f, "- Move: W, A, S, D", NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 55.f, "- Mouse to look", NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 70.f, "- Shift to run", NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 90.f, render_str, NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 110.f, delta_str, NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 130.f, frame_str, NULL, false, 255, 255, 255, 255);
	gsi_text(&gsi, 40.f, 150.f, vert_str, NULL, false, 255, 255, 255, 255);

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
	gs_graphics_vertex_buffer_destroy(vbo_cubemap);
	// free(cubemap->cubes);
	// free(cubemap);
	// cubemap=NULL;
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
