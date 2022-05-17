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

    { // sets up cursor signals
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

    { // sets up keyboard signals
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

    double sx, sy;
    struct wlr_surface* surface = NULL;

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
    struct wlr_event_pointer_axis* event = reinterpret_cast<wlr_event_pointer_axis*>(data);

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
    default: return;
    }

    double sx, sy;
    struct wlr_seat* seat = server->m_seat;
    struct wlr_surface* surface = NULL;
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
{}

void
Server::cursor_process_resize(Server* server, uint32_t time)
{}

void
Server::keyboard_handle_modifiers(struct wl_listener* listener, void* data)
{}

void
Server::keyboard_handle_key(struct wl_listener* listener, void* data)
{}

void
Server::seat_request_cursor(struct wl_listener* listener, void* data)
{}

void
Server::seat_request_set_selection(struct wl_listener* listener, void* data)
{}

void
Server::output_frame(struct wl_listener* listener, void* data)
{}

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

}

void
Server::focus_view(View* view, struct wlr_surface* surface)
{

}

void
Server::xdg_toplevel_map(struct wl_listener* listener, void* data)
{}

void
Server::xdg_toplevel_unmap(struct wl_listener* listener, void* data)
{}

void
Server::xdg_toplevel_destroy(struct wl_listener* listener, void* data)
{}

void
Server::xdg_toplevel_request_move(struct wl_listener* listener, void* data)
{}

void
Server::xdg_toplevel_request_resize(struct wl_listener* listener, void* data)
{}
