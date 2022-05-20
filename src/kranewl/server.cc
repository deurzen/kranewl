#include <kranewl/server.hh>

#include <kranewl/exec.hh>
#include <kranewl/input/keyboard.hh>
#include <kranewl/model.hh>
#include <kranewl/tree/client.hh>
#include <kranewl/tree/output.hh>

#include <spdlog/spdlog.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
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
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#ifdef XWAYLAND
#include <X11/Xlib.h>
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

Server::Server(Model& model)
    : m_model(model),
      mp_display(wl_display_create()),
      mp_event_loop(wl_display_get_event_loop(mp_display)),
      mp_backend(wlr_backend_autocreate(mp_display)),
      mp_renderer([](struct wl_display* display, struct wlr_backend* backend) {
        struct wlr_renderer* renderer = wlr_renderer_autocreate(backend);
        wlr_renderer_init_wl_display(renderer, display);

        return renderer;
      }(mp_display, mp_backend)),
      mp_allocator(wlr_allocator_autocreate(mp_backend, mp_renderer)),
      ml_new_output({ .notify = Server::new_output }),
      ml_output_layout_change({ .notify = Server::output_layout_change }),
      ml_output_manager_apply({ .notify = Server::output_manager_apply }),
      ml_output_manager_test({ .notify = Server::output_manager_test }),
      ml_new_xdg_surface({ .notify = Server::new_xdg_surface }),
      ml_new_layer_shell_surface({ .notify = Server::new_layer_shell_surface }),
      ml_xdg_activation({ .notify = Server::xdg_activation }),
      ml_cursor_motion({ .notify = Server::cursor_motion }),
      ml_cursor_motion_absolute({ .notify = Server::cursor_motion_absolute }),
      ml_cursor_button({ .notify = Server::cursor_button }),
      ml_cursor_axis({ .notify = Server::cursor_axis }),
      ml_cursor_frame({ .notify = Server::cursor_frame }),
      ml_request_set_cursor({ .notify = Server::request_set_cursor }),
      ml_request_start_drag({ .notify = Server::request_start_drag }),
      ml_start_drag({ .notify = Server::start_drag }),
      ml_new_input({ .notify = Server::new_input }),
      ml_request_set_selection({ .notify = Server::request_set_selection }),
      ml_request_set_primary_selection({ .notify = Server::request_set_primary_selection }),
      ml_inhibit_activate({ .notify = Server::inhibit_activate }),
      ml_inhibit_deactivate({ .notify = Server::inhibit_deactivate }),
      ml_idle_inhibitor_create({ .notify = Server::idle_inhibitor_create }),
      ml_idle_inhibitor_destroy({ .notify = Server::idle_inhibitor_destroy }),
#ifdef XWAYLAND
      ml_xwayland_ready({ .notify = Server::xwayland_ready }),
      ml_new_xwayland_surface({ .notify = Server::new_xwayland_surface }),
#endif
      m_socket(wl_display_add_socket_auto(mp_display))
{
    mp_compositor = wlr_compositor_create(mp_display, mp_renderer);
    mp_data_device_manager = wlr_data_device_manager_create(mp_display);

    mp_output_layout = wlr_output_layout_create();

    wl_list_init(&m_outputs);
    wl_signal_add(&mp_backend->events.new_output, &ml_new_output);

    mp_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(mp_scene, mp_output_layout);

    wl_list_init(&m_clients);

    mp_layer_shell = wlr_layer_shell_v1_create(mp_display);
    wl_signal_add(&mp_layer_shell->events.new_surface, &ml_new_layer_shell_surface);

    mp_xdg_shell = wlr_xdg_shell_create(mp_display);
    wl_signal_add(&mp_xdg_shell->events.new_surface, &ml_new_xdg_surface);

    mp_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(mp_cursor, mp_output_layout);

    mp_cursor_manager = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(mp_cursor_manager, 1);

    mp_seat = wlr_seat_create(mp_display, "seat0");
    mp_presentation = wlr_presentation_create(mp_display, mp_backend);
    mp_idle = wlr_idle_create(mp_display);

    mp_idle_inhibit_manager = wlr_idle_inhibit_v1_create(mp_display);
    wl_signal_add(&mp_idle_inhibit_manager->events.new_inhibitor, &ml_idle_inhibitor_create);

    { // set up cursor handling
        wl_signal_add(&mp_cursor->events.motion, &ml_cursor_motion);
        wl_signal_add(&mp_cursor->events.motion_absolute, &ml_cursor_motion_absolute);
        wl_signal_add(&mp_cursor->events.button, &ml_cursor_button);
        wl_signal_add(&mp_cursor->events.axis, &ml_cursor_axis);
        wl_signal_add(&mp_cursor->events.frame, &ml_cursor_frame);
    }

    { // set up keyboard handling
        wl_list_init(&m_keyboards);
        wl_signal_add(&mp_backend->events.new_input, &ml_new_input);
        wl_signal_add(&mp_seat->events.request_set_cursor, &ml_request_set_cursor);
        wl_signal_add(&mp_seat->events.request_set_selection, &ml_request_set_selection);
        wl_signal_add(&mp_seat->events.request_set_primary_selection, &ml_request_set_primary_selection);
    }

    mp_server_decoration_manager = wlr_server_decoration_manager_create(mp_display);
    mp_xdg_decoration_manager = wlr_xdg_decoration_manager_v1_create(mp_display);

    wlr_server_decoration_manager_set_default_mode(
        mp_server_decoration_manager,
        WLR_SERVER_DECORATION_MANAGER_MODE_SERVER
    );

    wlr_xdg_output_manager_v1_create(mp_display, mp_output_layout);
    wl_signal_add(&mp_output_layout->events.change, &ml_output_layout_change);

    mp_output_manager = wlr_output_manager_v1_create(mp_display);
    wl_signal_add(&mp_output_manager->events.apply, &ml_output_manager_apply);
    wl_signal_add(&mp_output_manager->events.test, &ml_output_manager_test);

    wlr_scene_set_presentation(mp_scene, wlr_presentation_create(mp_display, mp_backend));

#ifdef XWAYLAND
    mp_xwayland = wlr_xwayland_create(mp_display, mp_compositor, 1);

    if (mp_xwayland) {
        wl_signal_add(&mp_xwayland->events.ready, &ml_xwayland_ready);
        wl_signal_add(&mp_xwayland->events.new_surface, &ml_new_xwayland_surface);

        setenv("DISPLAY", mp_xwayland->display_name, 1);
    } else
        spdlog::error("Failed to initiate XWayland");
        spdlog::warn("Continuing without XWayland functionality");
#endif

    mp_input_inhibit_manager = wlr_input_inhibit_manager_create(mp_display);
    wl_signal_add(&mp_input_inhibit_manager->events.activate, &ml_inhibit_activate);
    wl_signal_add(&mp_input_inhibit_manager->events.deactivate, &ml_inhibit_deactivate);

    mp_keyboard_shortcuts_inhibit_manager = wlr_keyboard_shortcuts_inhibit_v1_create(mp_display);
    // TODO: mp_keyboard_shortcuts_inhibit_manager signals

    mp_pointer_constraints = wlr_pointer_constraints_v1_create(mp_display);
    // TODO: mp_pointer_constraints signals

    mp_relative_pointer_manager = wlr_relative_pointer_manager_v1_create(mp_display);
    // TODO: mp_relative_pointer_manager signals

    mp_virtual_pointer_manager = wlr_virtual_pointer_manager_v1_create(mp_display);
    // TODO: mp_virtual_pointer_manager signals

    mp_virtual_keyboard_manager = wlr_virtual_keyboard_manager_v1_create(mp_display);
    // TODO: mp_virtual_keyboard_manager signals

    if (m_socket.empty()) {
        wlr_backend_destroy(mp_backend);
        std::exit(1);
        return;
    }

    if (!wlr_backend_start(mp_backend)) {
        wlr_backend_destroy(mp_backend);
        wl_display_destroy(mp_display);
        std::exit(1);
        return;
    }

    setenv("WAYLAND_DISPLAY", m_socket.c_str(), true);
    setenv("XDG_CURRENT_DESKTOP", "kranewl", true);

    setenv("QT_QPA_PLATFORM", "wayland", true);
    setenv("MOZ_ENABLE_WAYLAND", "1", true);

    spdlog::info("Server initiated on WAYLAND_DISPLAY=" + m_socket);
}

Server::~Server()
{
    wl_display_destroy_clients(mp_display);
    wl_display_destroy(mp_display);
}

void
Server::start() noexcept
{
    wl_display_run(mp_display);
}

void
Server::new_output(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_new_output);

    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);
    wlr_output_init_render(wlr_output, server->mp_allocator, server->mp_renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        wlr_output_set_mode(wlr_output, wlr_output_preferred_mode(wlr_output));
        wlr_output_enable_adaptive_sync(wlr_output, true);
        wlr_output_enable(wlr_output, true);

        if (!wlr_output_commit(wlr_output))
            return;
    }

    Output_ptr output = server->m_model.create_output(
        wlr_output,
        wlr_scene_output_create(server->mp_scene, wlr_output)
    );

    output->ml_frame.notify = Server::output_frame;
    output->ml_destroy.notify = Server::output_destroy;
    wl_signal_add(&wlr_output->events.frame, &output->ml_frame);
    wl_signal_add(&wlr_output->events.destroy, &output->ml_destroy);

    wlr_output_layout_add_auto(server->mp_output_layout, wlr_output);
}

void
Server::output_layout_change(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::output_manager_apply(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::output_manager_test(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::output_frame(struct wl_listener* listener, void* data)
{
    Output_ptr output = wl_container_of(listener, output, ml_frame);

    wlr_scene_output_commit(output->mp_wlr_scene_output);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->mp_wlr_scene_output, &now);
}

void
Server::output_destroy(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::new_xdg_surface(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_new_xdg_surface);

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

    Client_ptr client = new Client(
        server,
        Surface{xdg_surface, true},
        nullptr,
        nullptr,
        nullptr
    );

    xdg_surface->data = client->mp_scene;
    client->mp_scene = wlr_scene_xdg_surface_create(
        &client->mp_server->mp_scene->node,
        client->m_surface.xdg
    );

    client->mp_scene->data = client;

    client->ml_map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_surface->events.map, &client->ml_map);
    client->ml_unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_surface->events.unmap, &client->ml_unmap);
    client->ml_destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &client->ml_destroy);

    struct wlr_xdg_toplevel* toplevel = xdg_surface->toplevel;
    client->ml_request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &client->ml_request_move);
    client->ml_request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &client->ml_request_resize);
}

void
Server::new_layer_shell_surface(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::xdg_activation(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::new_input(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_new_input);
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

    wlr_seat_set_capabilities(server->mp_seat, caps);
}

void
Server::new_pointer(Server_ptr server, struct wlr_input_device* device)
{
    wlr_cursor_attach_input_device(server->mp_cursor, device);
}

void
Server::new_keyboard(Server_ptr server, struct wlr_input_device* device)
{
    Keyboard* keyboard = reinterpret_cast<Keyboard*>(calloc(1, sizeof(Keyboard)));
    keyboard->mp_server = server;
    keyboard->mp_device = device;

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

    keyboard->ml_modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&device->keyboard->events.modifiers, &keyboard->ml_modifiers);
    keyboard->ml_key.notify = keyboard_handle_key;
    wl_signal_add(&device->keyboard->events.key, &keyboard->ml_key);

    wlr_seat_set_keyboard(server->mp_seat, device);
    wl_list_insert(&server->m_keyboards, &keyboard->m_link);
}

void
Server::inhibit_activate(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::inhibit_deactivate(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::idle_inhibitor_create(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::idle_inhibitor_destroy(struct wl_listener* listener, void* data)
{
    // TODO
}

void
Server::cursor_motion(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_cursor_motion);
    struct wlr_event_pointer_motion* event
        = reinterpret_cast<wlr_event_pointer_motion*>(data);

    wlr_cursor_move(server->mp_cursor, event->device, event->delta_x, event->delta_y);
    cursor_process_motion(server, event->time_msec);
}

void
Server::cursor_motion_absolute(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute* event
        = reinterpret_cast<wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(server->mp_cursor, event->device, event->x, event->y);
    cursor_process_motion(server, event->time_msec);
}

void
Server::cursor_button(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_cursor_button);

    struct wlr_event_pointer_button* event
        = reinterpret_cast<wlr_event_pointer_button*>(data);

    wlr_seat_pointer_notify_button(
        server->mp_seat,
        event->time_msec,
        event->button,
        event->state
    );

    struct wlr_surface* surface = NULL;

    double sx, sy;
    Client_ptr client = desktop_client_at(
        server,
        server->mp_cursor->x,
        server->mp_cursor->y,
        &surface,
        &sx,
        &sy
    );

    if (event->state == WLR_BUTTON_RELEASED)
        server->m_cursor_mode = Server::CursorMode::Passthrough;
    else
        focus_client(client, surface);
}

void
Server::cursor_axis(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_cursor_axis);

    struct wlr_event_pointer_axis* event
        = reinterpret_cast<wlr_event_pointer_axis*>(data);

    wlr_seat_pointer_notify_axis(
        server->mp_seat,
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
    Server_ptr server = wl_container_of(listener, server, ml_cursor_frame);

    wlr_seat_pointer_notify_frame(server->mp_seat);
}

void
Server::request_set_cursor(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_request_set_cursor);

    struct wlr_seat_pointer_request_set_cursor_event* event
        = reinterpret_cast<struct wlr_seat_pointer_request_set_cursor_event*>(data);
    struct wlr_seat_client* focused_client = server->mp_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client)
        wlr_cursor_set_surface(
            server->mp_cursor,
            event->surface,
            event->hotspot_x,
            event->hotspot_y
        );
}

void
Server::cursor_process_motion(Server_ptr server, uint32_t time)
{
    switch (server->m_cursor_mode) {
    case Server::CursorMode::Move:   cursor_process_move(server, time);   return;
    case Server::CursorMode::Resize: cursor_process_resize(server, time); return;
    default: break;
    }

    struct wlr_seat* seat = server->mp_seat;
    struct wlr_surface* surface = NULL;

    double sx, sy;
    Client_ptr client = desktop_client_at(
        server,
        server->mp_cursor->x,
        server->mp_cursor->y,
        &surface,
        &sx,
        &sy
    );

    if (!client)
        wlr_xcursor_manager_set_cursor_image(
            server->mp_cursor_manager,
            "left_ptr",
            server->mp_cursor
        );

    if (surface) {
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    } else
        wlr_seat_pointer_clear_focus(seat);
}

void
Server::cursor_process_move(Server_ptr server, uint32_t time)
{
    Client_ptr client = server->mp_grabbed_client;

    client->m_active_region.pos.x = server->mp_cursor->x - server->m_grab_x;
    client->m_active_region.pos.y = server->mp_cursor->y - server->m_grab_y;

    wlr_scene_node_set_position(
        client->mp_scene,
        client->m_active_region.pos.x,
        client->m_active_region.pos.y
    );
}

void
Server::cursor_process_resize(Server_ptr server, uint32_t time)
{
    Client_ptr client = server->mp_grabbed_client;

    double border_x = server->mp_cursor->x - server->m_grab_x;
    double border_y = server->mp_cursor->y - server->m_grab_y;

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
    wlr_xdg_surface_get_geometry(client->m_surface.xdg, &geo_box);

    client->m_active_region.pos.x = new_left - geo_box.x;
    client->m_active_region.pos.y = new_top - geo_box.y;

    wlr_scene_node_set_position(
        client->mp_scene,
        client->m_active_region.pos.x,
        client->m_active_region.pos.y
    );

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;

    wlr_xdg_toplevel_set_size(
        client->m_surface.xdg,
        new_width,
        new_height
    );
}

void
Server::request_start_drag(struct wl_listener*, void*)
{

}

void
Server::start_drag(struct wl_listener*, void*)
{

}

void
Server::keyboard_handle_modifiers(struct wl_listener* listener, void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, ml_modifiers);

    wlr_seat_set_keyboard(keyboard->mp_server->mp_seat, keyboard->mp_device);
    wlr_seat_keyboard_notify_modifiers(
        keyboard->mp_server->mp_seat,
        &keyboard->mp_device->keyboard->modifiers
    );
}

void
Server::keyboard_handle_key(struct wl_listener* listener, void* data)
{
    Keyboard* keyboard = wl_container_of(listener, keyboard, ml_key);
    Server_ptr server = keyboard->mp_server;

    struct wlr_event_keyboard_key* event
        = reinterpret_cast<struct wlr_event_keyboard_key*>(data);
    struct wlr_seat* seat = server->mp_seat;

    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t* syms;

    int nsyms = xkb_state_key_get_syms(
        keyboard->mp_device->keyboard->xkb_state,
        keycode,
        &syms
    );

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->mp_device->keyboard);

    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++)
            handled = keyboard_handle_keybinding(server, syms[i]);
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard->mp_device);
        wlr_seat_keyboard_notify_key(
            seat,
            event->time_msec,
            event->keycode,
            event->state
        );
    }
}

bool
Server::keyboard_handle_keybinding(Server_ptr server, xkb_keysym_t sym)
{
    switch (sym) {
    case XKB_KEY_Escape:
        wl_display_terminate(server->mp_display);
        break;
    case XKB_KEY_j:
        {
            if (wl_list_length(&server->m_clients) < 2)
                break;

            /* Client_ptr prev_client = wl_container_of( */
            /*     server->m_clients.prev, */
            /*     prev_client, */
            /*     link */
            /* ); */

            /* focus_client( */
            /*     prev_client, */
            /*     prev_client->get_surface() */
            /* ); */
        }
        break;
    case XKB_KEY_Return:
        {
            std::string foot = "foot";
            exec_external(foot);
        }
        break;
    case XKB_KEY_semicolon:
        {
            std::string st = "st";
            exec_external(st);
        }
        break;
    default: return false;
    }

    return true;
}

void
Server::request_set_selection(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_request_set_selection);

    struct wlr_seat_request_set_selection_event* event
        = reinterpret_cast<struct wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(server->mp_seat, event->source, event->serial);
}

void
Server::request_set_primary_selection(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_request_set_primary_selection);

    struct wlr_seat_request_set_primary_selection_event* event
        = reinterpret_cast<struct wlr_seat_request_set_primary_selection_event*>(data);

    wlr_seat_set_primary_selection(server->mp_seat, event->source, event->serial);
}

Client_ptr
Server::desktop_client_at(
    Server_ptr server,
    double lx,
    double ly,
    struct wlr_surface** surface,
    double* sx,
    double* sy
)
{
    struct wlr_scene_node* node = wlr_scene_node_at(
        &server->mp_scene->node,
        lx, ly,
        sx, sy
    );

    if (!node || node->type != WLR_SCENE_NODE_SURFACE)
        return nullptr;

    *surface = wlr_scene_surface_from_node(node)->surface;

    while (node && !node->data)
        node = node->parent;

    return reinterpret_cast<Client_ptr>(node->data);
}

void
Server::focus_client(Client_ptr client, struct wlr_surface* surface)
{
    if (!client)
        return;

    Server_ptr server = client->mp_server;
    struct wlr_seat* seat = server->mp_seat;
    struct wlr_surface* prev_surface = seat->keyboard_state.focused_surface;

    if (prev_surface == surface)
        return;

    /* if (prev_surface) */
    /*     wlr_xdg_toplevel_set_activated( */
    /*         wlr_xdg_surface_from_wlr_surface( */
    /*             seat->keyboard_state.focused_surface */
    /*         ), */
    /*         false */
    /*     ); */

    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);

    if (client->mp_scene)
        wlr_scene_node_raise_to_top(client->mp_scene);

    /* wl_list_remove(&client->link); */
    /* wl_list_insert(&server->m_clients, &client->link); */

    if (client->m_surface.type == SurfaceType::XDGShell || client->m_surface.type == SurfaceType::LayerShell)
        wlr_xdg_toplevel_set_activated(client->m_surface.xdg, true);

    wlr_seat_keyboard_notify_enter(
        seat,
        client->get_surface(),
        keyboard->keycodes,
        keyboard->num_keycodes,
        &keyboard->modifiers
    );
}

void
Server::xdg_toplevel_map(struct wl_listener* listener, void* data)
{
    Client_ptr client = wl_container_of(listener, client, ml_map);

    /* wl_list_insert(&client->server->m_clients, &client->link); */
    focus_client(client, client->get_surface());
}

void
Server::xdg_toplevel_unmap(struct wl_listener* listener, void* data)
{
    Client_ptr client = wl_container_of(listener, client, ml_unmap);
    /* wl_list_remove(&client->link); */
}

void
Server::xdg_toplevel_destroy(struct wl_listener* listener, void* data)
{
    Client_ptr client = wl_container_of(listener, client, ml_destroy);

    wl_list_remove(&client->ml_map.link);
    wl_list_remove(&client->ml_unmap.link);
    wl_list_remove(&client->ml_destroy.link);
    wl_list_remove(&client->ml_request_move.link);
    wl_list_remove(&client->ml_request_resize.link);

    delete client;
}

void
Server::xdg_toplevel_request_move(struct wl_listener* listener, void* data)
{
    Client_ptr client = wl_container_of(listener, client, ml_request_move);
    xdg_toplevel_handle_moveresize(client, Server::CursorMode::Move, 0);
}

void
Server::xdg_toplevel_request_resize(struct wl_listener* listener, void* data)
{
    struct wlr_xdg_toplevel_resize_event* event
        = reinterpret_cast<struct wlr_xdg_toplevel_resize_event*>(data);

    Client_ptr client = wl_container_of(listener, client, ml_request_resize);
    xdg_toplevel_handle_moveresize(client, Server::CursorMode::Resize, event->edges);
}

void
Server::xdg_toplevel_handle_moveresize(
    Client_ptr client,
    Server::CursorMode mode,
    uint32_t edges
)
{
    Server_ptr server = client->mp_server;

    if (client->get_surface()
        != wlr_surface_get_root_surface(server->mp_seat->pointer_state.focused_surface))
    {
        return;
    }

    server->mp_grabbed_client = client;
    server->m_cursor_mode = mode;

    switch (mode) {
    case Server::CursorMode::Move:
        server->m_grab_x = server->mp_cursor->x - client->m_active_region.pos.x;
        server->m_grab_y = server->mp_cursor->y - client->m_active_region.pos.y;
        return;
    case Server::CursorMode::Resize:
        {
            struct wlr_box geo_box;
            wlr_xdg_surface_get_geometry(client->m_surface.xdg, &geo_box);

            double border_x = (client->m_active_region.pos.x + geo_box.x) +
                ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);

            double border_y = (client->m_active_region.pos.y + geo_box.y) +
                ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);

            server->m_grab_x = server->mp_cursor->x - border_x;
            server->m_grab_y = server->mp_cursor->y - border_y;

            server->m_grab_geobox = geo_box;
            server->m_grab_geobox.x += client->m_active_region.pos.x;
            server->m_grab_geobox.y += client->m_active_region.pos.y;

            server->m_resize_edges = edges;
        }
        return;
    default: return;
    }
}

#ifdef XWAYLAND
void
Server::xwayland_ready(struct wl_listener* listener, void*)
{
    Server_ptr server = wl_container_of(listener, server, ml_xwayland_ready);

	xcb_connection_t* xconn = xcb_connect(server->mp_xwayland->display_name, NULL);
	if (xcb_connection_has_error(xconn)) {
        spdlog::error("Could not establish a connection with the X server");
        spdlog::warn("Continuing with limited XWayland functionality");
		return;
	}

	/* netatom[NetWMWindowTypeDialog]  = getatom(xconn, "_NET_WM_WINDOW_TYPE_DIALOG"); */
	/* netatom[NetWMWindowTypeSplash]  = getatom(xconn, "_NET_WM_WINDOW_TYPE_SPLASH"); */
	/* netatom[NetWMWindowTypeToolbar] = getatom(xconn, "_NET_WM_WINDOW_TYPE_TOOLBAR"); */
	/* netatom[NetWMWindowTypeUtility] = getatom(xconn, "_NET_WM_WINDOW_TYPE_UTILITY"); */

	wlr_xwayland_set_seat(server->mp_xwayland, server->mp_seat);

	struct wlr_xcursor* xcursor;
	if ((xcursor = wlr_xcursor_manager_get_xcursor(server->mp_cursor_manager, "left_ptr", 1)))
		wlr_xwayland_set_cursor(server->mp_xwayland,
				xcursor->images[0]->buffer, xcursor->images[0]->width * 4,
				xcursor->images[0]->width, xcursor->images[0]->height,
				xcursor->images[0]->hotspot_x, xcursor->images[0]->hotspot_y);

	xcb_disconnect(xconn);
}

void
Server::new_xwayland_surface(struct wl_listener* listener, void* data)
{
    Server_ptr server = wl_container_of(listener, server, ml_new_xwayland_surface);

	struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<wlr_xwayland_surface*>(data);

    Client_ptr client = new Client(
        server,
        Surface{xwayland_surface, xwayland_surface->override_redirect},
        nullptr,
        nullptr,
        nullptr
    );

	xwayland_surface->data = client;

    client->ml_map = { .notify = Server::xdg_toplevel_map };
    client->ml_unmap = { .notify = Server::xdg_toplevel_unmap };
    client->ml_destroy = { .notify = Server::xdg_toplevel_destroy };
    // TODO: client->ml_set_title = { .notify = Server::... };
    // TODO: client->ml_fullscreen = { .notify = Server::... };
    client->ml_request_activate = { .notify = Server::xwayland_request_activate };
    client->ml_request_configure = { .notify = Server::xwayland_request_configure };
    client->ml_set_hints = { .notify = Server::xwayland_set_hints };
    // TODO: client->ml_new_xdg_popup = { .notify = Server::... };

    wl_signal_add(&xwayland_surface->events.map, &client->ml_map);
    wl_signal_add(&xwayland_surface->events.unmap, &client->ml_unmap);
    wl_signal_add(&xwayland_surface->events.destroy, &client->ml_destroy);
    wl_signal_add(&xwayland_surface->events.request_activate, &client->ml_request_activate);
    wl_signal_add(&xwayland_surface->events.request_configure, &client->ml_request_configure);
    wl_signal_add(&xwayland_surface->events.set_hints, &client->ml_set_hints);
}

void
Server::xwayland_request_activate(struct wl_listener* listener, void*)
{

}

void
Server::xwayland_request_configure(struct wl_listener* listener, void*)
{

}

void
Server::xwayland_set_hints(struct wl_listener* listener, void*)
{

}
#endif
