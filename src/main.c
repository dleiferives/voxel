/*================================================================
    * Version 0.0.1: implemented fps camera and cubes. based off the gunslinger
    *         examples.
    * Using gunslinger from John Jackson
================================================================*/
#define GS_IMPL
#include "../include/gs/gs.h"
#include "data.c"

typedef struct fps_camera_t {
    float pitch;
    float bob_time;
    gs_camera_t cam;
} fps_camera_t;

fps_camera_t         fps = {0};
void fps_camera_update(fps_camera_t* cam);

#define rand_range(MIN, MAX)\
    (rand() % (MAX - MIN + 1) + MIN)

gs_command_buffer_t                      cb          = {0};
//gs_camera_t                              cam         = {0};
gs_handle(gs_graphics_vertex_buffer_t)   vbo         = {0};
gs_handle(gs_graphics_pipeline_t)        pips[4]     = {0};
gs_handle(gs_graphics_shader_t)          shaders[4]  = {0};
gs_handle(gs_graphics_uniform_t)         u_model     = {0};
gs_handle(gs_graphics_uniform_buffer_t)  u_vp        = {0};

typedef struct vparams_t {
    gs_mat4 projection;
    gs_mat4 view;
} vparams_t;

void app_init()
{
    // Construct new command buffer
    cb = gs_command_buffer_new();

    // Set up camera
//    cam = gs_camera_perspective();
//    cam.transform.position = gs_v3(0.f, 0.f, 3.f);
    fps.cam = gs_camera_perspective();
    fps.cam.transform.position = gs_v3(4.f, 2.f, 4.f);

    // Construct vertex buffer
    vbo = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = v_data,
            .size = sizeof(v_data)
        }
    );

    // Create uniform buffer
    u_vp = gs_graphics_uniform_buffer_create(
        &(gs_graphics_uniform_buffer_desc_t){
            .data = NULL,
            .size = sizeof(vparams_t),
            .name = "u_vp"
        }
    );

    // Upload projection matrix into buffer
    gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());
    gs_mat4 proj = gs_camera_get_proj(&(fps.cam), (int32_t)ws.x, (int32_t)ws.y);

    // Update sub region of uniform buffer data with projection (won't change during runtime)
    gs_graphics_uniform_buffer_request_update(&cb, u_vp,
        &(gs_graphics_uniform_buffer_desc_t){
            .data = &proj,
            .size = sizeof(proj),
            .update = {
                .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
                .offset = 0
            }
        }
    );

    u_model = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_model",
            .layout = (gs_graphics_uniform_layout_desc_t[]){{.type = GS_GRAPHICS_UNIFORM_MAT4}}
        }
    );

    const char* fshaders[4] = {
        f_red_src,
        f_blue_src,
        f_green_src,
        f_yellow_src
    };

    // Create shaders and pipelines
    for (uint32_t i = 0; i < 4; ++i)
    {
        shaders[i] = gs_graphics_shader_create (
            &(gs_graphics_shader_desc_t) {
                .sources = (gs_graphics_shader_source_desc_t[]){
                    {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = v_src},
                    {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = fshaders[i]}
                },
                .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
                .name = "color_shader"
            }
        );

        pips[i] = gs_graphics_pipeline_create (
            &(gs_graphics_pipeline_desc_t) {
                .raster = {
                    .shader = shaders[i]
                },
                .blend = {
                    .func = GS_GRAPHICS_BLEND_EQUATION_ADD,
                    .src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
                    .dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
                },
                .depth = {
                    .func = GS_GRAPHICS_DEPTH_FUNC_LESS
                },
                .layout = {
                    .attrs = (gs_graphics_vertex_attribute_desc_t[]) {
                        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"}        // Position
                    },
                    .size = sizeof(gs_graphics_vertex_attribute_desc_t)
                }
            }
        );
    }


    gs_platform_lock_mouse(gs_platform_main_window(), true);
}

void app_update()
{
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) gs_quit();

    gs_vec2 fs = gs_platform_framebuffer_sizev(gs_platform_main_window());

    if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked()) {
        fps.cam.transform.rotation = gs_quat_default();
        fps.pitch = 0.f;
        gs_platform_lock_mouse(gs_platform_main_window(), true);
    }

    // Action for clearing the screen
    gs_graphics_clear_desc_t clear = {.actions = &(gs_graphics_clear_action_t){.color = {0.1f, 0.1f, 0.1f, 1.f}}};

    // Update camera
    if (gs_platform_mouse_locked()) {
        fps_camera_update(&fps);
    }

    gs_mat4 view = gs_camera_get_view(&fps.cam);

    // Request buffer upate for view once per frame to be shared across pipelines
    gs_graphics_uniform_buffer_request_update(&cb, u_vp,
        &(gs_graphics_uniform_buffer_desc_t){
            .data = &view,
            .size = sizeof(view),
            .update = {
                .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
                .offset = sizeof(gs_mat4)
            }
        }
    );

    const float t = gs_platform_elapsed_time() * 0.001f;

    gs_mat4 models[] = {
        gs_mat4_mul(gs_mat4_translate(-0.75f, 0.75f, 0.0f), gs_mat4_rotatev(t, GS_XAXIS)),
        gs_mat4_mul(gs_mat4_translate(0.75f, 0.75f, 0.0f), gs_mat4_rotatev(t, GS_YAXIS)),
        gs_mat4_mul(gs_mat4_translate(-0.75f, -0.75f, 0.0f), gs_mat4_rotatev(t, GS_ZAXIS)),
        gs_mat4_mul(gs_mat4_translate(0.75f, -0.75f, 0.0f), gs_mat4_rotatev(t, gs_v3(1.f, 0.f, 1.f)))
    };

    /* Render */
    gs_graphics_renderpass_begin(&cb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
        gs_graphics_set_viewport(&cb, 0, 0, (uint32_t)fs.x, (uint32_t)fs.y);
        gs_graphics_clear(&cb, &clear);
        for (uint32_t i = 0; i < 4; ++i) {
            gs_graphics_bind_desc_t binds = {
                .vertex_buffers = {&(gs_graphics_bind_vertex_buffer_desc_t){.buffer = vbo}},
                .uniform_buffers = {&(gs_graphics_bind_uniform_buffer_desc_t){.buffer = u_vp, .binding = 0}},
                .uniforms = {&(gs_graphics_bind_uniform_desc_t){.uniform = u_model, .data = &models[i]}}
            };
            gs_graphics_pipeline_bind(&cb, pips[i]);
            gs_graphics_apply_bindings(&cb, &binds);
            gs_graphics_draw(&cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 36});
        }
    gs_graphics_renderpass_end(&cb);

    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_command_buffer_submit(&cb);
}


void fps_camera_update(fps_camera_t* fps)
{
    gs_platform_t* platform = gs_subsystem(platform);

    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), SENSITIVITY);
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT) ? 2.f : 1.f;
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

    // For a non-flying first person camera, need to lock the y movement velocity
    vel.y = 0.f;

    fps->cam.transform.position = gs_vec3_add(fps->cam.transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * CAM_SPEED * mod));

    // If moved, then we'll "bob" the camera some
    if (gs_vec3_len(vel) != 0.f) {
        fps->bob_time += dt * 8.f;
        float sb = sin(fps->bob_time);
        float bob_amt = (sb * 0.5f + 0.5f) * 0.1f * mod;
        float rot_amt = sb * 0.0004f * mod;
        fps->cam.transform.position.y = 2.f + bob_amt;
        fps->cam.transform.rotation = gs_quat_mul(fps->cam.transform.rotation, gs_quat_angle_axis(rot_amt, GS_ZAXIS));
    }
}
gs_app_desc_t gs_main(int32_t argc, char** argv)
{
    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update
    };
}






