#include <trace.hh>

#include <kranewl/input/cursor.hh>

#include <kranewl/input/seat.hh>
#include <kranewl/model.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
}
#undef static
#undef namespace
#undef class

#include <algorithm>

Cursor::Cursor(
    Server_ptr server,
    Seat_ptr seat,
    struct wlr_cursor* cursor,
    struct wlr_pointer_constraints_v1* pointer_constraints,
    struct wlr_relative_pointer_manager_v1* relative_pointer_manager,
    struct wlr_virtual_pointer_manager_v1* virtual_pointer_manager
)
    : mp_server(server),
      mp_seat(seat),
      mp_wlr_cursor(cursor),
      mp_cursor_manager(wlr_xcursor_manager_create(nullptr, 24)),
      mp_pointer_constraints(pointer_constraints),
      mp_relative_pointer_manager(relative_pointer_manager),
      mp_virtual_pointer_manager(virtual_pointer_manager),
      ml_cursor_motion({ .notify = Cursor::handle_cursor_motion }),
      ml_cursor_motion_absolute({ .notify = Cursor::handle_cursor_motion_absolute }),
      ml_cursor_button({ .notify = Cursor::handle_cursor_button }),
      ml_cursor_axis({ .notify = Cursor::handle_cursor_axis }),
      ml_cursor_frame({ .notify = Cursor::handle_cursor_frame }),
      ml_request_start_drag({ .notify = Cursor::handle_request_start_drag }),
      ml_start_drag({ .notify = Cursor::handle_start_drag }),
      ml_request_set_cursor({ .notify = Cursor::handle_request_set_cursor })
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

Cursor::~Cursor()
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
    static std::vector<SceneLayer> focus_order = {
        SCENE_LAYER_OVERLAY,
        SCENE_LAYER_TOP,
        SCENE_LAYER_FREE,
        SCENE_LAYER_TILE,
        SCENE_LAYER_BOTTOM,
    };

    struct wlr_scene_node* node;
    for (auto const& layer : focus_order) {
        if ((node = wlr_scene_node_at(server->m_scene_layers[layer], lx, ly, sx, sy))) {
            if (node->type != WLR_SCENE_NODE_SURFACE)
                return nullptr;

            *surface = wlr_scene_surface_from_node(node)->surface;

            while (node && !node->data)
                node = node->parent;

            if (node && node->data && static_cast<Node_ptr>(node->data)->is_focusable())
                return reinterpret_cast<View_ptr>(node->data);

            return nullptr;
        }
    }

    return nullptr;
}

View_ptr
Cursor::view_under_cursor() const
{
    double sx, sy;
    struct wlr_surface* surface = nullptr;

    View_ptr view = view_at(
        mp_server,
        mp_wlr_cursor->x,
        mp_wlr_cursor->y,
        &surface,
        &sx, &sy
    );

    return view;
}

void
Cursor::initiate_cursor_interactive(Mode mode, View_ptr view)
{
    TRACE();

    m_grab_state = {
        .view = view,
        .x = mp_wlr_cursor->x,
        .y = mp_wlr_cursor->y,
        .region = view->free_region(),
        .edges = WLR_EDGE_NONE
    };

    switch (mode) {
    case Mode::Move:
    {
        wlr_xcursor_manager_set_cursor_image(
            mp_cursor_manager,
            "fleur",
            mp_wlr_cursor
        );
        break;
    }
    case Mode::Resize:
    {
        Pos center = view->free_region().center();

        if (m_grab_state.x >= center.x)
            m_grab_state.edges |= WLR_EDGE_RIGHT;
        else
            m_grab_state.edges |= WLR_EDGE_LEFT;

        if (m_grab_state.y >= center.y) {
            m_grab_state.edges |= WLR_EDGE_BOTTOM;

            wlr_xcursor_manager_set_cursor_image(
                mp_cursor_manager,
                (m_grab_state.edges & WLR_EDGE_RIGHT)
                    ? "bottom_right_corner"
                    : "bottom_left_corner",
                mp_wlr_cursor
            );
        } else {
            m_grab_state.edges |= WLR_EDGE_TOP;

            wlr_xcursor_manager_set_cursor_image(
                mp_cursor_manager,
                (m_grab_state.edges & WLR_EDGE_RIGHT)
                    ? "top_right_corner"
                    : "top_left_corner",
                mp_wlr_cursor
            );
        }
        break;
    }
    default: break;
    }

    m_cursor_mode = mode;
}

void
Cursor::abort_cursor_interactive()
{
    m_cursor_mode = Mode::Passthrough;
    m_grab_state.view = nullptr;
}

static inline void
process_cursor_move(Cursor_ptr cursor, uint32_t time)
{
    TRACE();

    View_ptr view = cursor->m_grab_state.view;
    Pos const& pos = cursor->m_grab_state.region.pos;

    view->set_free_pos(Pos{
        .x = pos.x + cursor->mp_wlr_cursor->x - cursor->m_grab_state.x,
        .y = pos.y + cursor->mp_wlr_cursor->y - cursor->m_grab_state.y
    });

    view->configure(
        view->free_region(),
        view->free_decoration().extents(),
        true
    );
}

static inline void
process_cursor_resize(Cursor_ptr cursor, uint32_t time)
{
    TRACE();

    View_ptr view = cursor->m_grab_state.view;
    Region const& grab_region = cursor->m_grab_state.region;
    Region region = view->free_region();
    Decoration const& decoration = view->free_decoration();
    Extents const& extents = decoration.extents();

    int dx = cursor->mp_wlr_cursor->x - cursor->m_grab_state.x;
    int dy = cursor->mp_wlr_cursor->y - cursor->m_grab_state.y;

    int dest_w;
    int dest_h;

    if ((cursor->m_grab_state.edges & WLR_EDGE_LEFT))
        dest_w = grab_region.dim.w - dx;
    else
        dest_w = grab_region.dim.w + dx;

    if ((cursor->m_grab_state.edges & WLR_EDGE_TOP))
        dest_h = grab_region.dim.h - dy;
    else
        dest_h = grab_region.dim.h + dy;

    region.dim.w = std::max(0, dest_w);
    region.dim.h = std::max(0, dest_h);

    if ((cursor->m_grab_state.edges & WLR_EDGE_TOP))
        region.pos.y
            = grab_region.pos.y + (grab_region.dim.h - region.dim.h);

    if ((cursor->m_grab_state.edges & WLR_EDGE_LEFT))
        region.pos.x
            = grab_region.pos.x + (grab_region.dim.w - region.dim.w);

    if (region == view->prev_region())
        return;

    view->set_free_region(region);
    view->configure(
        view->free_region(),
        view->free_decoration().extents(),
        true
    );
}

static inline void
cursor_motion_to_client(
    Cursor_ptr cursor,
    View_ptr view,
    struct wlr_surface* surface,
    double sx, double sy,
    uint32_t time
)
{
    static View_ptr prev_view = nullptr;

    if (true /* TODO: focus_follows_cursor */ && time && view && view != prev_view && view->managed())
        cursor->mp_seat->mp_model->focus_view(view);

    if (!surface) {
        wlr_seat_pointer_notify_clear_focus(cursor->mp_seat->mp_wlr_seat);
        return;
    }

    if (!time) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time = now.tv_sec * 1000 + now.tv_nsec / 1000000;
    }

    wlr_seat_pointer_notify_enter(cursor->mp_seat->mp_wlr_seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(cursor->mp_seat->mp_wlr_seat, time, sx, sy);

    prev_view = view;
}

void
Cursor::process_cursor_motion(uint32_t time)
{
    struct wlr_drag_icon* icon;
    if (mp_seat->mp_wlr_seat->drag && (icon = mp_seat->mp_wlr_seat->drag->icon))
        wlr_scene_node_set_position(
            reinterpret_cast<struct wlr_scene_node*>(icon->data),
            mp_wlr_cursor->x + icon->surface->sx,
            mp_wlr_cursor->y + icon->surface->sy
        );

    switch (m_cursor_mode) {
    case Cursor::Mode::Move:   process_cursor_move(this, time);   return;
    case Cursor::Mode::Resize: process_cursor_resize(this, time); return;
    case Cursor::Mode::Passthrough: // fallthrough
    default: break;
    }

    double sx, sy;
    struct wlr_surface* surface = nullptr;

    View_ptr view = view_at(
        mp_server,
        mp_wlr_cursor->x,
        mp_wlr_cursor->y,
        &surface,
        &sx, &sy
    );

    if (!surface && time)
        wlr_xcursor_manager_set_cursor_image(
            mp_cursor_manager,
            "left_ptr",
            mp_wlr_cursor
        );

    cursor_motion_to_client(this, view, surface, sx, sy, time);
}

void
Cursor::handle_cursor_motion(struct wl_listener* listener, void* data)
{
    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_cursor_motion);
    struct wlr_event_pointer_motion* event
        = reinterpret_cast<struct wlr_event_pointer_motion*>(data);

    wlr_cursor_move(cursor->mp_wlr_cursor, event->device, event->delta_x, event->delta_y);
    cursor->process_cursor_motion(event->time_msec);
}

void
Cursor::handle_cursor_motion_absolute(struct wl_listener* listener, void* data)
{
    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute* event
        = reinterpret_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(cursor->mp_wlr_cursor, event->device, event->x, event->y);
    cursor->process_cursor_motion(event->time_msec);
}

bool
process_cursorbinding(Cursor_ptr cursor, uint32_t button, uint32_t modifiers)
{
    TRACE();

    if (!button || !modifiers)
        return false;

    CursorInput input = CursorInput{
        .target = CursorInput::Target::Global,
        .button = static_cast<CursorInput::Button>(button),
        .modifiers = modifiers & ~WLR_MODIFIER_CAPS
    };

    View_ptr view = cursor->view_under_cursor();
    Model_ptr model = cursor->mp_seat->mp_model;
    View_ptr focused_view = model->focused_view();

#define CALL_AND_HANDLE_FOCUS(binding) \
    do { \
        if (((*binding)(*model, view) && view && view != focused_view && view->managed())) \
            model->focus_view(view); \
    } while (false)

    { // global binding
        auto binding = Util::const_retrieve(model->cursor_bindings(), input);

        if (binding) {
            CALL_AND_HANDLE_FOCUS(binding);
            return true;
        }
    }

    if (!view) { // root binding
        input.target = CursorInput::Target::Root;
        auto binding = Util::const_retrieve(model->cursor_bindings(), input);

        if (binding) {
            CALL_AND_HANDLE_FOCUS(binding);
            return true;
        }
    } else { // view binding
        input.target = CursorInput::Target::View;
        auto binding = Util::const_retrieve(model->cursor_bindings(), input);

        if (binding) {
            CALL_AND_HANDLE_FOCUS(binding);
            return true;
        }
    }
#undef CALL_AND_HANDLE_FOCUS

    return false;
}

void
Cursor::handle_cursor_button(struct wl_listener* listener, void* data)
{
    TRACE();

    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_cursor_button);
    struct wlr_event_pointer_button* event
        = reinterpret_cast<struct wlr_event_pointer_button*>(data);

    wlr_idle_notify_activity(
        cursor->mp_seat->mp_idle,
        cursor->mp_seat->mp_wlr_seat
    );

    switch (event->state) {
    case WLR_BUTTON_PRESSED:
    {
        struct wlr_keyboard* keyboard
            = wlr_seat_get_keyboard(cursor->mp_seat->mp_wlr_seat);

        uint32_t button = event->button;
        uint32_t modifiers = keyboard
            ? wlr_keyboard_get_modifiers(keyboard)
            : 0;

        if (process_cursorbinding(cursor, button, modifiers))
            return;

        if (false /* TODO: !focus_follows_cursor */) {
            View_ptr view = cursor->view_under_cursor();

            if (view && !view->focused() && view->managed())
                cursor->mp_seat->mp_model->focus_view(view);
        }

        break;
    }
    case WLR_BUTTON_RELEASED:
    {
        if (cursor->m_cursor_mode != Mode::Passthrough) {
            cursor->m_cursor_mode = Mode::Passthrough;

            wlr_xcursor_manager_set_cursor_image(
                cursor->mp_cursor_manager,
                "left_ptr",
                cursor->mp_wlr_cursor
            );
        }

        break;
    }
    default: break;
    }

    wlr_seat_pointer_notify_button(
        cursor->mp_seat->mp_wlr_seat,
        event->time_msec,
        event->button,
        event->state
    );
}

void
Cursor::handle_cursor_axis(struct wl_listener* listener, void* data)
{
    TRACE();

    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_cursor_axis);
    struct wlr_event_pointer_axis* event
        = reinterpret_cast<struct wlr_event_pointer_axis*>(data);

    struct wlr_keyboard* keyboard
        = wlr_seat_get_keyboard(cursor->mp_seat->mp_wlr_seat);

    uint32_t button = 0;
    uint32_t modifiers = keyboard
        ? wlr_keyboard_get_modifiers(keyboard)
        : 0;

    if (modifiers)
        switch (event->orientation) {
        case WLR_AXIS_ORIENTATION_VERTICAL:
        {
            button = event->delta < 0
                ? CursorInput::Button::ScrollUp
                : CursorInput::Button::ScrollDown;
            break;
        }
        case WLR_AXIS_ORIENTATION_HORIZONTAL:
            button = event->delta < 0
                ? CursorInput::Button::ScrollLeft
                : CursorInput::Button::ScrollRight;
            break;
        default: break;
        }

    if (!process_cursorbinding(cursor, button, modifiers))
        wlr_seat_pointer_notify_axis(
            cursor->mp_seat->mp_wlr_seat,
            event->time_msec,
            event->orientation,
            event->delta,
            event->delta_discrete,
            event->source
        );
}

void
Cursor::handle_cursor_frame(struct wl_listener* listener, void*)
{
    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_cursor_frame);
    wlr_seat_pointer_notify_frame(cursor->mp_seat->mp_wlr_seat);
}

void
Cursor::handle_request_start_drag(struct wl_listener* listener, void* data)
{
    TRACE();

    Cursor_ptr cursor = wl_container_of(listener, cursor, ml_request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event* event
        = reinterpret_cast<struct wlr_seat_pointer_request_set_cursor_event*>(data);

    struct wlr_seat_client* focused_client
        = cursor->mp_seat->mp_wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client)
        wlr_cursor_set_surface(
            cursor->mp_wlr_cursor,
            event->surface,
            event->hotspot_x,
            event->hotspot_y
        );
}

void
Cursor::handle_start_drag(struct wl_listener*, void*)
{
    TRACE();

}

void
Cursor::handle_request_set_cursor(struct wl_listener*, void*)
{
    TRACE();

}
