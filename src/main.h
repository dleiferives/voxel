///////////////////////////////////////////////////////////////////////////////
/// Copyright: Dylan Leifer-Ives 2024
/// File: main.h
/// Project: Voxel
////////////////////////////////////////////////////////////////////////////////
#ifndef VOXEL_MAIN_H
#define VOXEL_MAIN_H

////////////////////////////////////////////////////////////////////////////////
/// Includes
////////////////////////////////////////////////////////////////////////////////
#define GS_IMPL
#include "../include/gs/gs.h"
#include "../include/perlin/perlin.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "../include/gs/util/gs_idraw.h"

////////////////////////////////////////////////////////////////////////////////
/// Defines
////////////////////////////////////////////////////////////////////////////////
#ifdef GS_PLATFORM_WEB
#define GS_GL_VERSION_STR "#version 300 es\n"
#define GS_GL_VERSION_STR_LEN 15
#else
#define GS_GL_VERSION_STR "#version 430 core\n"
#define GS_GL_VERSION_STR_LEN 17
#endif

#define VOXEL_SHADER_FRAGMENT_FILE "./shaders/fragment.glsl"
#define VOXEL_SHADER_VERTEX_FILE "./shaders/vertex.glsl"
#define CUBE_T_VERTS_COUNT 36

////////////////////////////////////////////////////////////////////////////////
/// Type Defines
////////////////////////////////////////////////////////////////////////////////

struct Program_s;
struct v_viewproj_s;
struct Cube_s;
struct CubeMap_s;

typedef struct Program_s Program_t;
typedef struct v_viewproj_s v_viewproj_t;
typedef struct Cube_s Cube_t;
typedef struct CubeMap_s CubeMap_t;

//// PROGRAM ///////////////////////////////////////////////////////////////////
typedef uint32_t err_t;

struct Program_s{
	char *fragment_shader;
	char *vertex_shader;
	gs_command_buffer_t                     command_buffer;
	gs_handle(gs_graphics_pipeline_t)       pipeline;
	gs_handle(gs_graphics_shader_t)         shader;
	gs_handle(gs_graphics_vertex_buffer_t)  vbo_cube;
	gs_handle(gs_graphics_vertex_buffer_t)  vbo_cubemap;
	gs_handle(gs_graphics_vertex_buffer_t)  vbo_color;
	gs_handle(gs_graphics_uniform_buffer_t) ub_vp;
	gs_handle(gs_graphics_vertex_buffer_t)  inst_vbo;
	gs_immediate_draw_t  gsi;
	CubeMap_t *cube_map;
};

err_t Program_load_shaders(Program_t *program);


struct v_viewproj_s{
	gs_mat4 projection;
	gs_mat4 view;
};

//// CUBE //////////////////////////////////////////////////////////////////////
// NOTE: macro CUBE_T_VERTS_COUNT exits
const float CUBE_T_VERTS[] = {
	0.0f, 0.0f, 0.0f, // triangle 1 : begin
	0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f, // triangle 1 : end
	1.0f, 1.0f, 0.0f, // triangle 2 : begin
	0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, // triangle 2 : end
	1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
};


struct Cube_s{
	gs_vec3 pos;
	int num_verts;
}; 

struct CubeMap_s{
 	Cube_t *cubes;
	int num_cubes;
};

#endif // !VOXEL_MAIN_H
