#define GS_IMPL
#include "../include/gs/gs.h"

// DATA ////////////////////////////////////////////////////////////////////////

float v_triangle[] ={
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   0.0f,  1.0f, 0.0f,
};

const char* fragment_shader = "";
const char* vertex_shader = "";

gs_command_buffer_t                    command_buffer = {0};
gs_handle(gs_graphics_vertex_buffer_t)   vbo_triangle = {0};
gs_handle(gs_graphics_pipeline_t)            pipeline = {0};
gs_handle(gs_graphics_shader_t)                shader = {0};
gs_camera_t                                       cam = {0};


// FORWARD FUNCTION DECLARATIONS ///////////////////////////////////////////////
void app_init();
void app_update();


// FUNCTIONS ///////////////////////////////////////////////////////////////////

void app_init(){

        // Set up our command buffer to submit to gpu
        command_buffer = gs_command_buffer_new();

        // Set up the camera
        cam = gs_camera_perspective();

        // Set up our vertex buffer 
        vbo_triangle = gs_graphics_vertex_buffer_create(
                &(gs_graphics_vertex_buffer_desc_t) {
                        .data = v_triangle,
                        .size = sizeof(v_triangle)
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

        // Set up pipeline
        pipeline = gs_graphics_pipeline_create(
                &(gs_graphics_pipeline_desc_t) {
                        .raster = {.shader = shader},
                        .depth = {
                                .func = GS_GRAPHICS_DEPTH_FUNC_LESS
                        },
                        .layout = {
                                .attrs = (gs_graphics_vertex_attribute_desc_t[]) {
                                        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"}
                                },
                                .size = sizeof(gs_graphics_vertex_attribute_desc_t)
                        }
                        
                        
                }
        );

        

}

void app_update(){
        if (gs_platform_key_pressed(GS_KEYCODE_ESC)) gs_quit();
        // Set up the fram buffer size
        gs_vec2 fs = gs_platform_framebuffer_sizev(gs_platform_main_window());

        // clear the screen 
        gs_graphics_clear_desc_t clear = {.actions = &(gs_graphics_clear_action_t){.color = {0.1f, 0.1f, 0.1f, 1.f}}};

        // Camera view
        gs_mat4 view = gs_camera_get_view(&cam);

        // Render //
        gs_graphics_renderpass_begin(&command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT);
                gs_graphics_set_viewport(&command_buffer, 0,0, (uint32_t)fs.x, (uint32_t)fs.y );
                // using the clear action
                gs_graphics_clear(&command_buffer, &clear);
                // buffer binds though there is only the vbo_triangle for now
                gs_graphics_bind_desc_t binds = {
                        .vertex_buffers = {
                                &(gs_graphics_bind_vertex_buffer_desc_t){
                                        .buffer = vbo_triangle
                                }
                        },
                };
                gs_graphics_pipeline_bind(&command_buffer,pipeline);
                gs_graphics_apply_bindings(&command_buffer, &binds);
                gs_graphics_draw(&command_buffer, 
                                 &(gs_graphics_draw_desc_t){
                                        .start = 0,
                                        .count = 36
                                 }
                );
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
