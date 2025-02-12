#include <stdio.h>

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

#include <cimgui.h>
#define SOKOL_IMGUI_IMPL
#include <sokol_imgui.h>

#include <shaders/square.h>

// :Application Settings

#define APPLICATION_NAME "Bouncy"
#define WIDTH 800
#define HEIGHT 600
#define VSYNC true

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

#define DELTA_TIME sapp_frame_duration()

void debug_ui(void);

// :GAME

#define MAX_GRID_COUNT (1920 * 1080)
typedef struct grid {
    int data[MAX_GRID_COUNT];
    int count;
    int width;
    int height;
} grid;

enum Particle {
    PARTICLE_AIR = 0,
    PARTICLE_SAND,
};

void make_grid(grid* _grid, int tile_size)
{
    _grid->width = WIDTH / tile_size;
    _grid->height = HEIGHT / tile_size;
    _grid->count = _grid->width * _grid->height;
    assert(_grid->count <= MAX_GRID_COUNT && "MAX_GRID_COUNT exceeded");
    memset(_grid->data, PARTICLE_AIR, sizeof(_grid->data[0]) * MAX_GRID_COUNT);
}

struct game_state_t {
    grid grid;
} game_state;

void square_render_pipeline(void)
{
    // clang-format off
    float vertices[] = {
        -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5,  0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    // clang-format on

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
        .data = SG_RANGE(vertices),
    });
    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc) {
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
    });

    state.pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = sg_make_shader(basic_shader_desc(sg_query_backend())),
        .index_type = SG_INDEXTYPE_UINT32,
        .layout = {
            .attrs = {
                [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_basic_color0].format = SG_VERTEXFORMAT_FLOAT4,
            },
        },
    });

    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.11f, 0.11f, 0.11f, 1.0f } },
    };
}

void setup_game(void)
{
    make_grid(&game_state.grid, 25);
    printf("Widht:  %d\n", game_state.grid.width);
    printf("Height: %d\n", game_state.grid.height);
}

// :APPLICATION

void init(void)
{
    sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    simgui_setup(&(simgui_desc_t) {
        .logger.func = slog_func,
    });
    printf("ImGui Version: %s\n", igGetVersion());

    setup_game();
}

void update(void)
{
    debug_ui();

    sg_begin_pass(&(sg_pass) { .action = state.pass_action, .swapchain = sglue_swapchain() });
    //
    //     sg_apply_pipeline(state.pip);
    //     sg_apply_bindings(&state.bind);
    //
    //     sg_draw(0, 6, 1);
    simgui_render();

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e)
{
    simgui_handle_event(e);
    if (e->key_code == SAPP_KEYCODE_Q) {
        sapp_request_quit();
    }
}

void cleanup(void)
{
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    return (sapp_desc) {
        .window_title = APPLICATION_NAME,
        .width = WIDTH,
        .height = HEIGHT,
        .swap_interval = (VSYNC ? 1 : 0), // 0 -> uncapped, (default) 1 -> vsync, 2 -> cap to half display, 3 -> cap to display / 3, etc..
        .init_cb = init,
        .frame_cb = update,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .logger.func = slog_func,
    };
}

void debug_ui(void)
{
    simgui_new_frame(&(simgui_frame_desc_t) {
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = DELTA_TIME,
        .dpi_scale = sapp_dpi_scale(),
    });
    igSetNextWindowPos((ImVec2) { 10, 10 }, ImGuiCond_Once, (ImVec2) { 0, 0 });
    // igSetNextWindowSize((ImVec2) { 200, (float)HEIGHT / 2 }, ImGuiCond_Once);
    igBegin("Debug", 0, ImGuiWindowFlags_None);
    igText("FPS: %.2lf", (1.0 / DELTA_TIME));
    // igText("Mouse Position: (%.1f, %.1f)", );
    igEnd();
}
