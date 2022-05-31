#include <trace.hh>

#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/util.hh>
#include <kranewl/workspace.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
}
#undef static
#undef namespace
#undef class

#include <vector>

Output::Output(
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    struct wlr_output* wlr_output,
    struct wlr_scene_output* wlr_scene_output,
    Region const&& output_region
)
    : mp_context(nullptr),
      m_full_region(output_region),
      m_placeable_region(output_region),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      m_dirty(true),
      m_cursor_focus_on_present(false),
      m_layer_map{
          { SCENE_LAYER_BACKGROUND, {} },
          { SCENE_LAYER_BOTTOM, {} },
          { SCENE_LAYER_TOP, {} },
          { SCENE_LAYER_OVERLAY, {} }
      },
      mp_wlr_output(wlr_output),
      mp_wlr_scene_output(wlr_scene_output),
      ml_frame({ .notify = Output::handle_frame }),
      ml_commit({ .notify = Output::handle_commit }),
      ml_present({ .notify = Output::handle_present }),
      ml_mode({ .notify = Output::handle_mode }),
      ml_destroy({ .notify = Output::handle_destroy })
{
    TRACE();

    wl_signal_add(&mp_wlr_output->events.frame, &ml_frame);
    wl_signal_add(&mp_wlr_output->events.commit, &ml_commit);
    wl_signal_add(&mp_wlr_output->events.present, &ml_present);
    wl_signal_add(&mp_wlr_output->events.mode, &ml_mode);
    wl_signal_add(&mp_wlr_output->events.destroy, &ml_destroy);

    wl_signal_init(&m_events.disable);
}

Output::~Output()
{}

void
Output::handle_frame(struct wl_listener* listener, void*)
{
    TRACE();

    Output_ptr output = wl_container_of(listener, output, ml_frame);

    if (!wlr_scene_output_commit(output->mp_wlr_scene_output))
        return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->mp_wlr_scene_output, &now);
}

void
Output::handle_commit(struct wl_listener*, void*)
{
    TRACE();

}

void
Output::handle_present(struct wl_listener* listener, void*)
{
    TRACE();

    Output_ptr output = wl_container_of(listener, output, ml_present);

    if (output->m_cursor_focus_on_present && output == output->mp_model->mp_output) {
        if (true /* TODO: focus_follows_mouse */) {
            View_ptr view_under_cursor
                = output->mp_seat->mp_cursor->view_under_cursor();

            if (view_under_cursor && view_under_cursor->managed())
                output->mp_model->focus_view(view_under_cursor);
        }

        output->m_cursor_focus_on_present = false;
    }
}

void
Output::handle_mode(struct wl_listener*, void*)
{
    TRACE();

}

void
Output::handle_destroy(struct wl_listener*, void* data)
{
    TRACE();

    static const std::vector<SceneLayer> scene_layers = {
        SCENE_LAYER_BACKGROUND,
        SCENE_LAYER_BOTTOM,
        SCENE_LAYER_TOP,
        SCENE_LAYER_OVERLAY,
    };

    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);
    Output_ptr output = reinterpret_cast<Output_ptr>(wlr_output->data);

    wl_list_remove(&output->ml_frame.link);
    wl_list_remove(&output->ml_destroy.link);
    wl_list_remove(&output->ml_present.link);
    wl_list_remove(&output->ml_mode.link);
    wl_list_remove(&output->ml_commit.link);

    wlr_scene_output_destroy(output->mp_wlr_scene_output);
    wlr_output_layout_remove(output->mp_server->mp_output_layout, output->mp_wlr_output);

    for (SceneLayer scene_layer : scene_layers) {
        for (Layer_ptr layer : output->m_layer_map.at(scene_layer)) {
            layer->mp_output = nullptr;
            layer->mp_layer_surface->output = nullptr;
        }

        output->m_layer_map.at(scene_layer).clear();
    }

    output->mp_model->unregister_output(output);
}

void
Output::set_context(Context_ptr context)
{
    TRACE();

    if (!context)
        spdlog::error("Output must contain a valid context");

    if (mp_context)
        mp_context->set_output(nullptr);

    context->set_output(this);
    mp_context = context;
}

Context_ptr
Output::context() const
{
    return mp_context;
}

Region
Output::full_region() const
{
    return m_full_region;
}

Region
Output::placeable_region() const
{
    return m_placeable_region;
}

void
Output::set_placeable_region(Region const& region)
{
    m_placeable_region = region;
}

bool
Output::contains(Pos pos) const
{
    return m_full_region.contains(pos);
}

bool
Output::contains(Region region) const
{
    return m_full_region.contains(region);
}

void
Output::focus_at_cursor()
{
    m_cursor_focus_on_present = true;
}

void
Output::add_layer(Layer_ptr layer)
{
    TRACE();
    m_layer_map[layer->m_scene_layer].push_back(layer);
}

void
Output::remove_layer(Layer_ptr layer)
{
    TRACE();
    Util::erase_remove(m_layer_map.at(layer->m_scene_layer), layer);
}

void
Output::relayer_layer(Layer_ptr layer, SceneLayer old_layer, SceneLayer new_layer)
{
    TRACE();

    Util::erase_remove(m_layer_map.at(old_layer), layer);
    m_layer_map[new_layer].push_back(layer);
    layer->m_scene_layer = new_layer;
}

static inline void
propagate_exclusivity(
    Region& placeable_region,
    uint32_t anchor,
    int32_t exclusive_zone,
    int32_t margin_top,
    int32_t margin_right,
    int32_t margin_bottom,
    int32_t margin_left
)
{
    TRACE();

    struct Edge {
        uint32_t singular_anchor;
        uint32_t anchor_triplet;
        int* positive_axis;
        int* negative_axis;
        int margin;
    };

    // https://github.com/swaywm/sway/blob/master/sway/desktop/layer_shell.c
    const std::vector<Edge> edges = {
        {
            .singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
            .anchor_triplet =
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
            .positive_axis = &placeable_region.pos.y,
            .negative_axis = &placeable_region.dim.h,
            .margin = margin_top,
        },
        {
            .singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .anchor_triplet =
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = NULL,
            .negative_axis = &placeable_region.dim.h,
            .margin = margin_bottom,
        },
        {
            .singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
            .anchor_triplet =
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = &placeable_region.pos.x,
            .negative_axis = &placeable_region.dim.w,
            .margin = margin_left,
        },
        {
            .singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
            .anchor_triplet =
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
            .positive_axis = NULL,
            .negative_axis = &placeable_region.dim.w,
            .margin = margin_right,
        },
    };

    for (auto const& edge : edges) {
        if ((anchor  == edge.singular_anchor || anchor == edge.anchor_triplet)
            && exclusive_zone + edge.margin > 0)
        {
            if (edge.positive_axis)
                *edge.positive_axis += exclusive_zone + edge.margin;

            if (edge.negative_axis)
                *edge.negative_axis -= exclusive_zone + edge.margin;

            break;
        }
    }
}

static inline void
arrange_layer(
    Output_ptr output,
    Region& placeable_region,
    std::vector<Layer_ptr> const& layers,
    bool exclusive
)
{
    TRACE();

    static constexpr uint32_t full_width
        = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    static constexpr uint32_t full_height
        = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;

    for (Layer_ptr layer : layers) {
        struct wlr_layer_surface_v1* layer_surface = layer->mp_layer_surface;
        struct wlr_layer_surface_v1_state* state = &layer_surface->current;

        if (exclusive != (state->exclusive_zone > 0))
            continue;

        Region region = {
            .dim = {
                .w = state->desired_width,
                .h = state->desired_height,
            }
        };

        Region bounds = state->exclusive_zone == -1
            ? output->full_region()
            : placeable_region;

        // horizontal axis
        if ((state->anchor & full_width) && region.dim.w == 0) {
            region.pos.x = bounds.pos.x;
            region.dim.w = bounds.dim.w;
        } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT))
            region.pos.x = bounds.pos.x;
        else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT))
            region.pos.x = bounds.pos.x + (bounds.dim.w - region.dim.w);
        else
            region.pos.x = bounds.pos.x + ((bounds.dim.w / 2) - (region.dim.w / 2));

        // vertical axis
        if ((state->anchor & full_height) && region.dim.h == 0) {
            region.pos.y = bounds.pos.y;
            region.dim.h = bounds.dim.h;
        } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP))
            region.pos.y = bounds.pos.y;
        else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM))
            region.pos.y = bounds.pos.y + (bounds.dim.h - region.dim.h);
        else
            region.pos.y = bounds.pos.y + ((bounds.dim.h / 2) - (region.dim.h / 2));

        { // margin
            if ((state->anchor & full_width) == full_width) {
                region.pos.x += state->margin.left;
                region.dim.w -= state->margin.left + state->margin.right;
            } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT))
                region.pos.x += state->margin.left;
            else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT))
                region.pos.x -= state->margin.right;

            if ((state->anchor & full_height) == full_height) {
                region.pos.y += state->margin.top;
                region.dim.h -= state->margin.top + state->margin.bottom;
            } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP))
                region.pos.y += state->margin.top;
            else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM))
                region.pos.y -= state->margin.bottom;
        }

        if (region.dim.w < 0 || region.dim.h < 0) {
            wlr_layer_surface_v1_destroy(layer_surface);
            continue;
        }

        layer->set_region(region);

        if (state->exclusive_zone > 0)
            propagate_exclusivity(
                placeable_region,
                state->anchor,
                state->exclusive_zone,
                state->margin.top,
                state->margin.right,
                state->margin.bottom,
                state->margin.left
            );

        wlr_scene_node_set_position(layer->mp_scene, region.pos.x, region.pos.y);
        wlr_layer_surface_v1_configure(layer_surface, region.dim.w, region.dim.h);
    }
}

void
Output::arrange_layers()
{
    TRACE();

    static const std::vector<SceneLayer> scene_layers_top_bottom = {
        SCENE_LAYER_OVERLAY,
        SCENE_LAYER_TOP,
        SCENE_LAYER_BOTTOM,
        SCENE_LAYER_BACKGROUND,
    };

    static const std::vector<SceneLayer> scene_layers_super_shell = {
        SCENE_LAYER_OVERLAY,
        SCENE_LAYER_TOP,
    };

    Region placeable_region = m_full_region;
    struct wlr_keyboard* keyboard
        = wlr_seat_get_keyboard(mp_seat->mp_wlr_seat);

    // exclusive surfaces
    for (SceneLayer scene_layer : scene_layers_top_bottom)
        arrange_layer(this, placeable_region, m_layer_map.at(scene_layer), true);

    // TODO: re-apply layout if placeable region changed
    if (mp_context && m_placeable_region != placeable_region) {
        set_placeable_region(placeable_region);
        mp_model->apply_layout(mp_context->workspace());
    }

    // non-exclusive surfaces
    for (SceneLayer scene_layer : scene_layers_top_bottom)
        arrange_layer(this, placeable_region, m_layer_map.at(scene_layer), false);

    for (SceneLayer scene_layer : scene_layers_super_shell)
        for (Layer_ptr layer : Util::reverse(m_layer_map[scene_layer]))
            if (layer->mp_layer_surface->current.keyboard_interactive
                && layer->mp_layer_surface->mapped)
            {
                mp_server->relinquish_focus();

                if (keyboard)
                    wlr_seat_keyboard_notify_enter(
                        mp_seat->mp_wlr_seat,
                        layer->mp_layer_surface->surface,
                        keyboard->keycodes,
                        keyboard->num_keycodes,
                        &keyboard->modifiers
                    );
                else
                    wlr_seat_keyboard_notify_enter(
                        mp_seat->mp_wlr_seat,
                        layer->mp_layer_surface->surface,
                        nullptr,
                        0,
                        nullptr
                    );

                return;
            }
}
