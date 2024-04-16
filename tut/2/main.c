#define GS_IMPL
#include "../../include/gs/gs.h"

#ifdef GS_PLATFORM_WEB
    #define GS_GL_VERSION_STR "#version 300 es\n"
#else
    #define GS_GL_VERSION_STR "#version 430 core\n"
#endif

// DATA ////////////////////////////////////////////////////////////////////////

float v_triangle[] ={
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   0.0f,  1.0f, 0.0f,
};

const char* fragment_shader = 
        GS_GL_VERSION_STR 
        "precision mediump float;\n"
        "out vec3 color;\n"
        "void main(){\n"
        "       color = vec3(1,0,0);\n"
        "}\n";

const char* vertex_shader = 
        GS_GL_VERSION_STR
        "layout(location = 0) in vec3 a_pos;\n"
        "layout (std140) uniform ub_vp {\n"
        "   mat4 projection;\n"
        "   mat4 view;\n"
        "};\n"
        "void main(){\n" // Note that the model position is the identity matrix for a mat4
        "   gl_Position = projection * view * mat4(1.0) *  vec4(a_pos, 1.0);\n"
        "}\n";

gs_command_buffer_t                    command_buffer = {0};
gs_handle(gs_graphics_pipeline_t)            pipeline = {0};
gs_handle(gs_graphics_shader_t)                shader = {0};
gs_camera_t                                       cam = {0};

gs_handle(gs_graphics_vertex_buffer_t)   vbo_triangle = {0};
gs_handle(gs_graphics_uniform_buffer_t)         ub_vp = {0};

typedef struct v_viewproj_t{
        gs_mat4 projection;
        gs_mat4 view;
} v_viewproj_t;


// FORWARD FUNCTION DECLARATIONS ///////////////////////////////////////////////
void app_init();
void app_update();


// FUNCTIONS ///////////////////////////////////////////////////////////////////

void app_init(){

        // Set up our command buffer to submit to gpu
        command_buffer = gs_command_buffer_new();

        // Set up the camera
        cam = gs_camera_perspective();
        cam.transform.position = (gs_vec3){4.0,3.0,3.0};
        cam.transform.scale = (gs_vec3){1.0,1.0,1.0};
        cam.transform.rotation = (gs_vec3){0.0,1.0,0.0};
        // Set up our vertex buffer 
        vbo_triangle = gs_graphics_vertex_buffer_create(
                &(gs_graphics_vertex_buffer_desc_t) {
                        .data = v_triangle,
                        .size = sizeof(v_triangle)
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
                &cam,
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
        // TODO: update with a fps camera
        gs_mat4 view = gs_camera_get_view(&cam);

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
                                        .count = sizeof(v_triangle) / (3 * sizeof(float))
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
