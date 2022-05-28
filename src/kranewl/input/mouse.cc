#include <trace.hh>

#include <kranewl/input/mouse.hh>

#include <kranewl/input/seat.hh>
#include <kranewl/layers.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/view.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
}
#undef static
#undef class
#undef namespace

Mouse::Mouse(
    Server_ptr server,
    Seat_ptr seat,
    struct wlr_cursor* cursor,
    struct wlr_pointer_constraints_v1* pointer_constraints,
    struct wlr_relative_pointer_manager_v1* relative_pointer_manager,
    struct wlr_virtual_pointer_manager_v1* virtual_pointer_manager
)
    : mp_server(server),
      mp_seat(seat),
      mp_cursor(cursor),
      mp_cursor_manager(wlr_xcursor_manager_create(nullptr, 24)),
      mp_pointer_constraints(pointer_constraints),
      mp_relative_pointer_manager(relative_pointer_manager),
      mp_virtual_pointer_manager(virtual_pointer_manager),
      ml_cursor_motion({ .notify = Mouse::handle_cursor_motion }),
      ml_cursor_motion_absolute({ .notify = Mouse::handle_cursor_motion_absolute }),
      ml_cursor_button({ .notify = Mouse::handle_cursor_button }),
      ml_cursor_axis({ .notify = Mouse::handle_cursor_axis }),
      ml_cursor_frame({ .notify = Mouse::handle_cursor_frame }),
      ml_request_start_drag({ .notify = Mouse::handle_request_start_drag }),
      ml_start_drag({ .notify = Mouse::handle_start_drag }),
      ml_request_set_cursor({ .notify = Mouse::handle_request_set_cursor })
{
    TRACE();

    wlr_xcursor_manager_load(mp_cursor_manager, 1);

    wl_signal_add(&cursor->events.motion, &ml_cursor_motion);
    wl_signal_add(&cursor->events.motion_absolute, &ml_cursor_motion_absolute);
    wl_signal_add(&cursor->events.button, &ml_cursor_button);
    wl_signal_add(&cursor->events.axis, &ml_cursor_axis);
    wl_signal_add(&cursor->events.frame, &ml_cursor_frame);
    wl_signal_add(&mp_seat->mp_wlr_seat->events.request_start_drag, &ml_request_start_drag);
    wl_signal_add(&mp_seat->mp_wlr_seat->events.start_drag, &ml_start_drag);
    wl_signal_add(&mp_seat->mp_wlr_seat->events.request_set_cursor, &ml_request_set_cursor);
}

Mouse::~Mouse()
{
    TRACE();

}

static inline void
process_cursor_move(Mouse_ptr mouse, uint32_t time)
{
    TRACE();

}

static inline void
process_cursor_resize(Mouse_ptr mouse, uint32_t time)
{
    TRACE();

}

static inline View_ptr
view_at(
    Server_ptr server,
    double lx, double ly,
    struct wlr_surface** surface,
    double* sx, double* sy
)
{
    static std::vector<Layer::type> focus_order = {
        Layer::Overlay,
        Layer::Top,
        Layer::Free,
        Layer::Tile,
        Layer::Bottom,
    };

    struct wlr_scene_node** layers = server->m_layers;
    struct wlr_scene_node* node;

    for (auto const& layer : focus_order) {
        if ((node = wlr_scene_node_at(layers[layer], lx, ly, sx, sy))) {
            if (node->type != WLR_SCENE_NODE_SURFACE)
                return nullptr;

            *surface = wlr_scene_surface_from_node(node)->surface;

            while (node && !node->data)
                node = node->parent;

            return reinterpret_cast<View_ptr>(node->data);
        }
    }

    return nullptr;
}

static inline void
cursor_set_focus(
    Mouse_ptr mouse,
    View_ptr view,
    struct wlr_surface* surface,
    double sx, double sy,
    uint32_t time
)
{
    if (true /* TODO: focus_follows_mouse */ && time && view && view->managed())
        mouse->mp_seat->mp_model->focus_view(view);

    if (!surface) {
        wlr_seat_pointer_notify_clear_focus(mouse->mp_seat->mp_wlr_seat);
        return;
    }

    if (!time) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time = now.tv_sec * 1000 + now.tv_nsec / 1000000;
    }

    wlr_seat_pointer_notify_enter(mouse->mp_seat->mp_wlr_seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(mouse->mp_seat->mp_wlr_seat, time, sx, sy);
}

static inline void
process_cursor_motion(Mouse_ptr mouse, uint32_t time)
{
    TRACE();

    struct wlr_drag_icon* icon;
    if (mouse->mp_seat->mp_wlr_seat->drag && (icon = mouse->mp_seat->mp_wlr_seat->drag->icon))
        wlr_scene_node_set_position(
            reinterpret_cast<struct wlr_scene_node*>(icon->data),
            mouse->mp_cursor->x + icon->surface->sx,
            mouse->mp_cursor->y + icon->surface->sy
        );

    switch (mouse->m_cursor_mode) {
    case Mouse::CursorMode::Move:   process_cursor_move(mouse, time);   return;
    case Mouse::CursorMode::Resize: process_cursor_resize(mouse, time); return;
    case Mouse::CursorMode::Passthrough: // fallthrough
    default: break;
    }

    double sx, sy;
    struct wlr_surface* surface = nullptr;
    View_ptr view = view_at(
        mouse->mp_server,
        mouse->mp_cursor->x,
        mouse->mp_cursor->y,
        &surface,
        &sx, &sy
    );

    if (!view && time) {
        wlr_xcursor_manager_set_cursor_image(
            mouse->mp_cursor_manager,
            "left_ptr",
            mouse->mp_cursor
        );
    }

    cursor_set_focus(mouse, view, surface, sx, sy, time);
}

void
Mouse::handle_cursor_motion(struct wl_listener* listener, void* data)
{
    TRACE();

    Mouse_ptr mouse = wl_container_of(listener, mouse, ml_cursor_motion);
    struct wlr_event_pointer_motion* event
        = reinterpret_cast<struct wlr_event_pointer_motion*>(data);

    wlr_cursor_move(mouse->mp_cursor, event->device, event->delta_x, event->delta_y);
    process_cursor_motion(mouse, event->time_msec);
}

void
Mouse::handle_cursor_motion_absolute(struct wl_listener* listener, void* data)
{
    TRACE();

    Mouse_ptr mouse = wl_container_of(listener, mouse, ml_cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute* event
        = reinterpret_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(mouse->mp_cursor, event->device, event->x, event->y);
    process_cursor_motion(mouse, event->time_msec);
}

void
Mouse::handle_cursor_button(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
Mouse::handle_cursor_axis(struct wl_listener*, void*)
{
    TRACE();

}

void
Mouse::handle_cursor_frame(struct wl_listener* listener, void*)
{
    TRACE();

    Mouse_ptr mouse = wl_container_of(listener, mouse, ml_cursor_frame);
    wlr_seat_pointer_notify_frame(mouse->mp_seat->mp_wlr_seat);
}

void
Mouse::handle_request_start_drag(struct wl_listener*, void*)
{
    TRACE();

}

void
Mouse::handle_start_drag(struct wl_listener*, void*)
{
    TRACE();

}

void
Mouse::handle_request_set_cursor(struct wl_listener*, void*)
{
    TRACE();

}
