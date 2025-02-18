#include <stdio.h>

#include <inttypes.h>

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

#include <cglm/cglm.h>

#include <cimgui.h>
#define SOKOL_IMGUI_IMPL
#include <sokol_imgui.h>

#include <shaders/grid.h>

// :Application Settings

#define APPLICATION_NAME "Bouncy"
#define WIDTH 1600
#define HEIGHT 1000
#define VSYNC true
#define CLEAR_COLOR 0.11f, 0.11f, 0.11f, 1.0f

#define TILE_SIZE 5

#define DELTA_TIME sapp_frame_duration()

void debug_ui(void);

// :GAME

#define MAX_GRID_COUNT (1920 * 1080)
typedef struct {
    int data[MAX_GRID_COUNT];
    int count;
    int width;
    int height;
    int tile_size;
} grid_t;

// clang-format off
#define RGBA_TO_ABGR(color) (                  \
    ((((uint32_t)color) & 0xff000000) >> 24) | \
    ((((uint32_t)color) & 0x00ff0000) >> 8)  | \
    ((((uint32_t)color) & 0x0000ff00) << 8)  | \
    ((((uint32_t)color) & 0x000000ff) << 24))
// clang-format on

// enum, color
#define PARTICLE_ENUM                \
    X(PARTICLE_NONE = 0, 0x00000000) \
    X(PARTICLE_SAND, 0xf7dba7ff)     \
    X(PARTICLE_AIR, 0x48beffff)      \
    X(PARTICLE_WATER, 0x1ca3ecff)    \
    X(PARTICLE_MAX, 0x00000000)

// X(PARTICLE_SAND, 0xf6d7b0ff)
// X(PARTICLE_SAND, 0xe5be9eff)     \
// X(PARTICLE_AIR, 0x89c2d9ff)
// X(PARTICLE_AIR, 0x87ceebff)

#define X(enum_item, _) enum_item,
typedef enum {
    PARTICLE_ENUM
} particle_t;
#undef X
#define X(_, color) RGBA_TO_ABGR(color),
uint32_t particle_get_color(particle_t e_particle)
{
    static uint32_t particle_color[] = {
        PARTICLE_ENUM
    };
    return particle_color[e_particle];
}
#undef X

struct game_state_t {
    grid_t grid;
} game_state;

void make_grid(grid_t* grid, int tile_size)
{
    grid->width = WIDTH / tile_size;
    grid->height = HEIGHT / tile_size;
    grid->count = grid->width * grid->height;
    grid->tile_size = tile_size;
    assert(grid->count <= MAX_GRID_COUNT && "MAX_GRID_COUNT exceeded");
    memset(grid->data, PARTICLE_NONE, sizeof(grid->data[0]) * MAX_GRID_COUNT);
}

void setup_game(void)
{
    make_grid(&game_state.grid, TILE_SIZE);
    for (int i = 0; i < game_state.grid.count; i++) {
        game_state.grid.data[i] = PARTICLE_AIR;
    }
}

particle_t get_tile(int x, int y)
{
    if (x < 0 || y < 0 || x > game_state.grid.width || y > game_state.grid.height)
        return PARTICLE_NONE;
    return game_state.grid.data[x + y * game_state.grid.width];
}

void set_tile(int x, int y, particle_t particle)
{
    if (x < 0 || y < 0 || x > game_state.grid.width || y > game_state.grid.height)
        return;
    game_state.grid.data[x + y * game_state.grid.width] = particle;
}

// converts from window to grid
void set_tile_from_window(int x, int y, particle_t particle)
{
    // TODO: add out of bounds checks
    x = x / game_state.grid.tile_size;
    y = y / game_state.grid.tile_size;
    set_tile(x, y, particle);
}

bool is_empty(int x, int y)
{
    particle_t tile = get_tile(x, y);
    return tile == PARTICLE_AIR;
}

// :RENDERING

#define MAX_PIXEL_INSTANCE MAX_GRID_COUNT

typedef struct {
    float x, y;
    uint32_t color;
} PixelInstance;

typedef struct {
    sg_pipeline pipeline;
    sg_buffer vertex;
    sg_buffer index;
    sg_buffer instance;
    PixelInstance instance_data[MAX_PIXEL_INSTANCE];
} GridRenderState;
static GridRenderState grid_render_state;

void update_pixels(sg_buffer* buf);

// void make_grid_pipeline(void)
void make_grid_pipeline(GridRenderState* pip)
{
    // clang-format off
    float quad[] = {
        -0.5f, -0.5f, // bot left
        -0.5f,  0.5f, // top left
         0.5f,  0.5,  // top right
         0.5f, -0.5f, // bot right
    };

    uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    // clang-format on

    pip->vertex = sg_make_buffer(&(sg_buffer_desc) {
        .data = SG_RANGE(quad),
    });

    pip->index = sg_make_buffer(&(sg_buffer_desc) {
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
    });

    // grid_state.instance = sg_make_buffer(&(sg_buffer_desc) {
    pip->instance = sg_make_buffer(&(sg_buffer_desc) {
        .size = sizeof(grid_render_state.instance_data),
        .usage = SG_USAGE_STREAM,
    });

    // grid_state.pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
    pip->pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = sg_make_shader(grid_shader_desc(sg_query_backend())),
        .index_type = SG_INDEXTYPE_UINT16,
        .colors[0].blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        },
        .layout = {
            .buffers = {
                [0] = { .stride = sizeof(float) * 2 },
                [1] = { .stride = sizeof(PixelInstance), .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = {
                    .buffer_index = 0,
                    .format = SG_VERTEXFORMAT_FLOAT2,
                },
                [1] = {
                    .buffer_index = 1,
                    .format = SG_VERTEXFORMAT_FLOAT2,
                },
                [2] = {
                    .buffer_index = 1,
                    .format = SG_VERTEXFORMAT_UBYTE4N,
                },
            },
        },
    });
}

void render_init(void)
{
    simgui_setup(&(simgui_desc_t) {
        .logger.func = slog_func,
    });
    printf("ImGui Version: %s\n", igGetVersion());

    make_grid_pipeline(&grid_render_state);
}

void render(void)
{
    debug_ui();

    sg_begin_pass(&(sg_pass) {
        .action.colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { CLEAR_COLOR },
        },
        .swapchain = sglue_swapchain(),
    });

    update_pixels(&grid_render_state.instance);

    sg_apply_pipeline(grid_render_state.pipeline);
    sg_apply_bindings(&(sg_bindings) {
        .vertex_buffers[0] = grid_render_state.vertex,
        .vertex_buffers[1] = grid_render_state.instance,
        .index_buffer = grid_render_state.index,
    });

    struct {
        mat4 model, view, projection;
    } mvp;

    glm_mat4_identity(mvp.model);
    glm_scale(mvp.model, (vec3) { game_state.grid.tile_size, game_state.grid.tile_size, 1.0 });

    glm_mat4_identity(mvp.view);
    glm_translate(mvp.view, (vec3) { game_state.grid.tile_size / 2.0, game_state.grid.tile_size / 2.0, 0.0 });

    glm_mat4_identity(mvp.projection);

    glm_ortho(0.0f, WIDTH, HEIGHT, 0.0f, -1.0f, 1.0f, mvp.projection);
    sg_apply_uniforms(0, &SG_RANGE(mvp));

    sg_draw(0, 6, game_state.grid.count);
    simgui_render();

    sg_end_pass();
    sg_commit();
}

void update_pixels(sg_buffer* buf)
{
    for (int i = 0; i < game_state.grid.count; i++) {
        int x = i % game_state.grid.width;
        int y = i / game_state.grid.width;
        grid_render_state.instance_data[i].x = x;
        grid_render_state.instance_data[i].y = y;
        assert(game_state.grid.data[i] < PARTICLE_MAX && "Unknown particle in grid");
        grid_render_state.instance_data[i].color = particle_get_color(game_state.grid.data[i]);
    }

    sg_update_buffer(*buf, &SG_RANGE(grid_render_state.instance_data));
}

// :EVENT

static enum {
    MOUSE_NONE,
    MOUSE_LEFT,
    MOUSE_RIGHT,

} mouse_held;

static struct {
    float x, y;
} mouse_pos;

void event_mouseup(const sapp_event* e)
{
    // LEFT
    if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        mouse_held = MOUSE_NONE;
    }
    // RIGHT
    if (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
        mouse_held = MOUSE_NONE;
    }
    // MIDDLE
    if (e->mouse_button == SAPP_MOUSEBUTTON_MIDDLE) {
    }
}

void event_mousedown(const sapp_event* e)
{
    // LEFT
    if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        mouse_held = MOUSE_LEFT;
        set_tile_from_window(e->mouse_x, e->mouse_y, PARTICLE_SAND);
    }
    // RIGHT
    if (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
        if (mouse_held == MOUSE_NONE)
            mouse_held = MOUSE_RIGHT;
        set_tile_from_window(e->mouse_x, e->mouse_y, PARTICLE_AIR);
    }
    // MIDDLE
    if (e->mouse_button == SAPP_MOUSEBUTTON_MIDDLE) {
    }
}

// :APPLICATION

void init(void)
{
    sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    render_init();
    setup_game();
}

// TODO: move to another thread, to make consistent
void fixed_update(void)
{
    for (int i = game_state.grid.count - 1; i >= 0; i--) {
        int x = i % game_state.grid.width;
        int y = i / game_state.grid.width;

        if (get_tile(x, y) == PARTICLE_SAND) {
            particle_t below = get_tile(x, y + 1);
            particle_t left = get_tile(x - 1, y + 1);
            particle_t right = get_tile(x + 1, y + 1);

            if (is_empty(x, y + 1)) {
                set_tile(x, y, below);
                set_tile(x, y + 1, PARTICLE_SAND);
            } else if (is_empty(x - 1, y + 1)) {
                set_tile(x, y, left);
                set_tile(x - 1, y + 1, PARTICLE_SAND);
            } else if (is_empty(x + 1, y + 1)) {
                set_tile(x, y, right);
                set_tile(x + 1, y + 1, PARTICLE_SAND);
            }
        }
    }
}

void update(void)
{
    static const double interval = 20.0;
    static double current = 0.0;
    if (current > interval) {
        fixed_update();
        current = 0.0;
    }
    current += DELTA_TIME * 1000.0;
    if (mouse_held == MOUSE_LEFT) {
        set_tile_from_window(mouse_pos.x, mouse_pos.y, PARTICLE_SAND);
    }

    if (mouse_held == MOUSE_RIGHT) {
        set_tile_from_window(mouse_pos.x, mouse_pos.y, PARTICLE_AIR);
    }
    render();
}

void event(const sapp_event* e)
{
    simgui_handle_event(e);
    switch (e->type) {
    case SAPP_EVENTTYPE_KEY_UP:
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        if (e->key_code == SAPP_KEYCODE_Q) {
            sapp_request_quit();
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        mouse_pos.x = e->mouse_x;
        mouse_pos.y = e->mouse_y;
        break;
    case SAPP_EVENTTYPE_MOUSE_ENTER:
        break;
    case SAPP_EVENTTYPE_MOUSE_LEAVE:
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        event_mouseup(e);
        break;
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        event_mousedown(e);
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        break;
    default:
        fprintf(stderr, "Unknown event type: %d\n", e->type);
    }
}

void cleanup(void)
{
    simgui_shutdown();
    sg_shutdown();
}

// :ENTRY

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

// :DEBUG

void debug_ui(void)
{
    simgui_new_frame(&(simgui_frame_desc_t) {
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = DELTA_TIME,
        .dpi_scale = sapp_dpi_scale(),
    });
    igSetNextWindowPos((ImVec2) { 10, 10 }, ImGuiCond_Once, (ImVec2) { 0, 0 });
    igBegin("Debug", 0, ImGuiWindowFlags_AlwaysAutoResize);
    igText("FPS: %.2lf", (1.0 / DELTA_TIME));
    igText("Game State:");
    igText("  Grid (WxH): %dx%d", game_state.grid.width, game_state.grid.height);
    ImVec2 mouse_pos = { .x = 0.0f, .y = 0.0f };
    igGetMousePos(&mouse_pos);
    if (igIsMousePosValid(&mouse_pos)) {
        igText("Mouse: (%.2f, %.2f)", mouse_pos.x, mouse_pos.y);
    } else {
        igText("Mouse: (0.0, 0.0)");
    }
    const char* held = "NONE";
    if (mouse_held == MOUSE_LEFT) {
        held = "LEFT";
    } else if (mouse_held == MOUSE_RIGHT) {
        held = "RIGHT";
    }
    igText("Mouse Held: %s\n", held);
    igEnd();
}
