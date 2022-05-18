#include <kranewl/server.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>

#include <spdlog/spdlog.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

// https://github.com/swaywm/wlroots/issues/682
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#ifdef WLR_HAS_XWAYLAND
#include <wlr/xwayland.h>
#endif
}
#undef static
#undef class
#undef namespace

extern "C" {
#include <xkbcommon/xkbcommon.h>
}

#include <cstdlib>

Server::Server()
    : m_display(wl_display_create()),
      m_backend(wlr_backend_autocreate(m_display)),
      m_renderer([](struct wl_display* display, struct wlr_backend* backend) {
        struct wlr_renderer* renderer = wlr_renderer_autocreate(backend);
        wlr_renderer_init_wl_display(renderer, display);

        return renderer;
      }(m_display, m_backend)),
      m_allocator(wlr_allocator_autocreate(m_backend, m_renderer)),
      m_socket(wl_display_add_socket_auto(m_display))
{
    wlr_log_init(WLR_DEBUG, NULL);

    wlr_compositor_create(m_display, m_renderer);
    wlr_data_device_manager_create(m_display);

    m_output_layout = wlr_output_layout_create();

    wl_list_init(&m_outputs);
    m_new_output.notify = Server::new_output;
    wl_signal_add(&m_backend->events.new_output, &m_new_output);

    m_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(m_scene, m_output_layout);

    wl_list_init(&m_views);
    m_xdg_shell = wlr_xdg_shell_create(m_display);
    m_new_xdg_surface.notify = Server::new_xdg_surface;
    wl_signal_add(&m_xdg_shell->events.new_surface, &m_new_xdg_surface);

    m_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(m_cursor, m_output_layout);

    m_cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(m_cursor_mgr, 1);

    { // set up cursor signals
        m_cursor_motion.notify = Server::cursor_motion;
        wl_signal_add(&m_cursor->events.motion, &m_cursor_motion);
        m_cursor_motion_absolute.notify = Server::cursor_motion_absolute;
        wl_signal_add(&m_cursor->events.motion_absolute, &m_cursor_motion_absolute);
        m_cursor_button.notify = Server::cursor_button;
        wl_signal_add(&m_cursor->events.button, &m_cursor_button);
        m_cursor_axis.notify = Server::cursor_axis;
        wl_signal_add(&m_cursor->events.axis, &m_cursor_axis);
        m_cursor_frame.notify = Server::cursor_frame;
        wl_signal_add(&m_cursor->events.frame, &m_cursor_frame);
    }

    { // set up keyboard signals
        wl_list_init(&m_keyboards);
        m_new_input.notify = Server::new_input;
        wl_signal_add(&m_backend->events.new_input, &m_new_input);
        m_seat = wlr_seat_create(m_display, "seat0");
        m_request_cursor.notify = Server::seat_request_cursor;
        wl_signal_add(&m_seat->events.request_set_cursor, &m_request_cursor);
        m_request_set_selection.notify = Server::seat_request_set_selection;
        wl_signal_add(&m_seat->events.request_set_selection, &m_request_set_selection);
    }

    if (m_socket.empty()) {
        wlr_backend_destroy(m_backend);
        exit(1);
        return;
    }

    if (!wlr_backend_start(m_backend)) {
        wlr_backend_destroy(m_backend);
        wl_display_destroy(m_display);
        exit(1);
        return;
    }

    setenv("WAYLAND_DISPLAY", m_socket.c_str(), true);
    spdlog::info("Server initiated on WAYLAND_DISPLAY=" + m_socket);
}

Server::~Server()
{
    wl_display_destroy_clients(m_display);
    wl_display_destroy(m_display);
}

void
Server::start() noexcept
{
    wl_display_run(m_display);
}

void
Server::new_output(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_new_output);

    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);

    wlr_output_init_render(wlr_output, server->m_allocator, server->m_renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            return;
        }
    }

    Output* output = reinterpret_cast<Output*>(calloc(1, sizeof(Output)));
    output->wlr_output = wlr_output;
    output->server = server;
    output->frame.notify = Server::output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
    wl_list_insert(&server->m_outputs, &output->link);

    wlr_output_layout_add_auto(server->m_output_layout, wlr_output);
}

void
Server::new_xdg_surface(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_new_xdg_surface);

    struct wlr_xdg_surface* xdg_surface = reinterpret_cast<struct wlr_xdg_surface*>(data);

    if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface* parent
            = wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);

        struct wlr_scene_node* parent_node
            = reinterpret_cast<struct wlr_scene_node*>(parent->data);

        xdg_surface->data = wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        return;
    }
    assert(xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

    View* view = reinterpret_cast<View*>(calloc(1, sizeof(View)));
    view->server = server;
    view->xdg_surface = xdg_surface;
    view->scene_node = wlr_scene_xdg_surface_create(
        &view->server->m_scene->node,
        view->xdg_surface
    );
    view->scene_node->data = view;
    xdg_surface->data = view->scene_node;

    view->map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_surface->events.map, &view->map);
    view->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
    view->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    struct wlr_xdg_toplevel* toplevel = xdg_surface->toplevel;
    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);
    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);
}

void
Server::new_input(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_new_input);
    struct wlr_input_device* device = reinterpret_cast<struct wlr_input_device*>(data);

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        new_pointer(server, device);
        break;
    default:
        break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->m_keyboards))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    wlr_seat_set_capabilities(server->m_seat, caps);
}

void
Server::new_pointer(Server* server, struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(server->m_cursor, device);
}

void
Server::new_keyboard(Server* server, struct wlr_input_device* device)
{
    Keyboard* keyboard = reinterpret_cast<Keyboard*>(calloc(1, sizeof(Keyboard)));
    keyboard->server = server;
    keyboard->device = device;

    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap = xkb_keymap_new_from_names(
        context,
        NULL,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&device->keyboard->events.key, &keyboard->key);

    wlr_seat_set_keyboard(server->m_seat, device);
    wl_list_insert(&server->m_keyboards, &keyboard->link);
}

void
Server::cursor_motion(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_cursor_motion);
    struct wlr_event_pointer_motion* event
        = reinterpret_cast<wlr_event_pointer_motion*>(data);

    wlr_cursor_move(server->m_cursor, event->device, event->delta_x, event->delta_y);
    cursor_process_motion(server, event->time_msec);
}

void
Server::cursor_motion_absolute(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute* event
        = reinterpret_cast<wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(server->m_cursor, event->device, event->x, event->y);
    cursor_process_motion(server, event->time_msec);
}

void
Server::cursor_button(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_cursor_button);

    struct wlr_event_pointer_button* event
        = reinterpret_cast<wlr_event_pointer_button*>(data);

    wlr_seat_pointer_notify_button(
        server->m_seat,
        event->time_msec,
        event->button,
        event->state
    );

    struct wlr_surface* surface = NULL;

    double sx, sy;
    View* view = desktop_view_at(
        server,
        server->m_cursor->x,
        server->m_cursor->y,
        &surface,
        &sx,
        &sy
    );

    if (event->state == WLR_BUTTON_RELEASED)
        server->m_cursor_mode = Server::CursorMode::Passthrough;
    else
        focus_view(view, surface);
}

void
Server::cursor_axis(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_cursor_axis);

    struct wlr_event_pointer_axis* event
        = reinterpret_cast<wlr_event_pointer_axis*>(data);

    wlr_seat_pointer_notify_axis(
        server->m_seat,
        event->time_msec,
        event->orientation,
        event->delta,
        event->delta_discrete,
        event->source
    );
}

void
Server::cursor_frame(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_cursor_frame);

    wlr_seat_pointer_notify_frame(server->m_seat);
}

void
Server::cursor_process_motion(Server* server, uint32_t time)
{
    switch (server->m_cursor_mode) {
    case Server::CursorMode::Move:   cursor_process_move(server, time);   return;
    case Server::CursorMode::Resize: cursor_process_resize(server, time); return;
    default: break;
    }

    struct wlr_seat* seat = server->m_seat;
    struct wlr_surface* surface = NULL;

    double sx, sy;
    View* view = desktop_view_at(
        server,
        server->m_cursor->x,
        server->m_cursor->y,
        &surface,
        &sx,
        &sy
    );

    if (!view)
        wlr_xcursor_manager_set_cursor_image(
            server->m_cursor_mgr,
            "left_ptr",
            server->m_cursor
        );

    if (surface) {
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    } else
        wlr_seat_pointer_clear_focus(seat);
}

void
Server::cursor_process_move(Server* server, uint32_t time)
{
    View* view = server->m_grabbed_view;

    view->x = server->m_cursor->x - server->m_grab_x;
    view->y = server->m_cursor->y - server->m_grab_y;

    wlr_scene_node_set_position(view->scene_node, view->x, view->y);
}

void
Server::cursor_process_resize(Server* server, uint32_t time)
{
    View* view = server->m_grabbed_view;

    double border_x = server->m_cursor->x - server->m_grab_x;
    double border_y = server->m_cursor->y - server->m_grab_y;

    int new_left = server->m_grab_geobox.x;
    int new_right = server->m_grab_geobox.x + server->m_grab_geobox.width;
    int new_top = server->m_grab_geobox.y;
    int new_bottom = server->m_grab_geobox.y + server->m_grab_geobox.height;

    if (server->m_resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;

        if (new_top >= new_bottom)
            new_top = new_bottom - 1;
    } else if (server->m_resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;

        if (new_bottom <= new_top)
            new_bottom = new_top + 1;
    }

    if (server->m_resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;

        if (new_left >= new_right)
            new_left = new_right - 1;
    } else if (server->m_resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;

        if (new_right <= new_left)
            new_right = new_left + 1;
    }

    struct wlr_box geo_box;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
    view->x = new_left - geo_box.x;
    view->y = new_top - geo_box.y;
    wlr_scene_node_set_position(view->scene_node, view->x, view->y);

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(view->xdg_surface, new_width, new_height);
}

void
Server::keyboard_handle_modifiers(struct wl_listener* listener, void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, modifiers);

    wlr_seat_set_keyboard(keyboard->server->m_seat, keyboard->device);
    wlr_seat_keyboard_notify_modifiers(
        keyboard->server->m_seat,
        &keyboard->device->keyboard->modifiers
    );
}

void
Server::keyboard_handle_key(struct wl_listener* listener, void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, key);
    Server* server = keyboard->server;

    struct wlr_event_keyboard_key* event
        = reinterpret_cast<struct wlr_event_keyboard_key*>(data);
    struct wlr_seat* seat = server->m_seat;

    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t* syms;

    int nsyms = xkb_state_key_get_syms(
        keyboard->device->keyboard->xkb_state,
        keycode,
        &syms
    );

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++)
            handled = keyboard_handle_keybinding(server, syms[i]);
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard->device);
        wlr_seat_keyboard_notify_key(
            seat,
            event->time_msec,
            event->keycode,
            event->state
        );
    }
}

bool
Server::keyboard_handle_keybinding(Server* server, xkb_keysym_t sym)
{
    switch (sym) {
    case XKB_KEY_Escape:
        wl_display_terminate(server->m_display);
        break;
    case XKB_KEY_F1:
        {
            if (wl_list_length(&server->m_views) < 2)
                break;

            View* next_view = wl_container_of(server->m_views.prev, next_view, link);
            focus_view(next_view, next_view->xdg_surface->surface);
        }
        break;
    default: return false;
    }

    return true;
}

void
Server::seat_request_cursor(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_request_cursor);

    struct wlr_seat_pointer_request_set_cursor_event* event
        = reinterpret_cast<struct wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = server->m_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client)
        wlr_cursor_set_surface(
            server->m_cursor,
            event->surface,
            event->hotspot_x,
            event->hotspot_y
        );
}

void
Server::seat_request_set_selection(struct wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, m_request_set_selection);

    struct wlr_seat_request_set_selection_event* event
        = reinterpret_cast<struct wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(server->m_seat, event->source, event->serial);
}

void
Server::output_frame(struct wl_listener* listener, void* data)
{
    Output* output = wl_container_of(listener, output, frame);
    struct wlr_scene* scene = output->server->m_scene;

    struct wlr_scene_output* scene_output
        = wlr_scene_get_scene_output(scene, output->wlr_output);

    wlr_scene_output_commit(scene_output);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

View*
Server::desktop_view_at(
    Server* server,
    double lx,
    double ly,
    struct wlr_surface** surface,
    double* sx,
    double* sy
)
{
    struct wlr_scene_node* node = wlr_scene_node_at(
        &server->m_scene->node,
        lx, ly,
        sx, sy
    );

    if (!node || node->type != WLR_SCENE_NODE_SURFACE)
        return nullptr;

    *surface = wlr_scene_surface_from_node(node)->surface;

    while (node && !node->data)
        node = node->parent;

    return reinterpret_cast<View*>(node->data);
}

void
Server::focus_view(View* view, struct wlr_surface* surface)
{
    if (!view)
        return;

    Server* server = view->server;
    struct wlr_seat* seat = server->m_seat;
    struct wlr_surface* prev_surface = seat->keyboard_state.focused_surface;

    if (prev_surface == surface)
        return;

    if (prev_surface)
        wlr_xdg_toplevel_set_activated(
            wlr_xdg_surface_from_wlr_surface(
                seat->keyboard_state.focused_surface
            ),
            false
        );

    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);

    wlr_scene_node_raise_to_top(view->scene_node);
    wl_list_remove(&view->link);
    wl_list_insert(&server->m_views, &view->link);
    wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
    wlr_seat_keyboard_notify_enter(
        seat,
        view->xdg_surface->surface,
        keyboard->keycodes,
        keyboard->num_keycodes,
        &keyboard->modifiers
    );
}

void
Server::xdg_toplevel_map(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, map);

    wl_list_insert(&view->server->m_views, &view->link);
    focus_view(view, view->xdg_surface->surface);
}

void
Server::xdg_toplevel_unmap(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, unmap);
    wl_list_remove(&view->link);
}

void
Server::xdg_toplevel_destroy(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, destroy);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);

    free(view);
}

void
Server::xdg_toplevel_request_move(struct wl_listener* listener, void* data)
{
    View* view = wl_container_of(listener, view, request_move);
    xdg_toplevel_handle_moveresize(view, Server::CursorMode::Move, 0);
}

void
Server::xdg_toplevel_request_resize(struct wl_listener* listener, void* data)
{
    struct wlr_xdg_toplevel_resize_event* event
        = reinterpret_cast<struct wlr_xdg_toplevel_resize_event*>(data);

    View* view = wl_container_of(listener, view, request_resize);
    xdg_toplevel_handle_moveresize(view, Server::CursorMode::Resize, event->edges);
}

void
Server::xdg_toplevel_handle_moveresize(View* view, Server::CursorMode mode, uint32_t edges)
{
    Server* server = view->server;

    if (view->xdg_surface->surface
        != wlr_surface_get_root_surface(server->m_seat->pointer_state.focused_surface))
    {
        return;
    }

    server->m_grabbed_view = view;
    server->m_cursor_mode = mode;

    switch (mode) {
    case Server::CursorMode::Move:
        server->m_grab_x = server->m_cursor->x - view->x;
        server->m_grab_y = server->m_cursor->y - view->y;
        return;
    case Server::CursorMode::Resize:
        {
            struct wlr_box geo_box;
            wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);

            double border_x = (view->x + geo_box.x) +
                ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);

            double border_y = (view->y + geo_box.y) +
                ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);

            server->m_grab_x = server->m_cursor->x - border_x;
            server->m_grab_y = server->m_cursor->y - border_y;

            server->m_grab_geobox = geo_box;
            server->m_grab_geobox.x += view->x;
            server->m_grab_geobox.y += view->y;

            server->m_resize_edges = edges;
        }
        return;
    default: return;
    }
}
