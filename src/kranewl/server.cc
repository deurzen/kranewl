#include <trace.hh>

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
#include <wlr/backend/headless.h>
#include <wlr/backend/multi.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
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
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_screencopy_v1.h>
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

Server::Server(Model_ptr model)
    : mp_model([this,model]() {
        model->register_server(this);
        return model;
      }()),
      mp_display(wl_display_create()),
      mp_event_loop(wl_display_get_event_loop(mp_display)),
      mp_backend(wlr_backend_autocreate(mp_display)),
      mp_headless_backend(wlr_headless_backend_create(mp_display)),
      mp_renderer([](struct wl_display* display, struct wlr_backend* backend) {
          struct wlr_renderer* renderer = wlr_renderer_autocreate(backend);
          wlr_renderer_init_wl_display(renderer, display);

          return renderer;
      }(mp_display, mp_backend)),
      mp_allocator(wlr_allocator_autocreate(mp_backend, mp_renderer)),
      mp_compositor(wlr_compositor_create(mp_display, mp_renderer)),
      mp_data_device_manager(wlr_data_device_manager_create(mp_display)),
      mp_scene(wlr_scene_create()),
      m_root([this](struct wl_display* display) {
          struct wlr_output_layout* wlr_output_layout = wlr_output_layout_create();
          wlr_xdg_output_manager_v1_create(display, wlr_output_layout);

          struct wlr_output* wlr_output =
              wlr_headless_add_output(mp_headless_backend, 800, 600);
          wlr_output_set_name(wlr_output, "FALLBACK");

          return Root(
              this,
              mp_model,
              wlr_output_layout,
              mp_model->create_output(
                  wlr_output,
                  wlr_scene_output_create(mp_scene, wlr_output),
                  true
              )
          );
      }(mp_display)),
      m_seat(this, mp_model, wlr_seat_create(mp_display, "seat0"), wlr_cursor_create()),
#ifdef XWAYLAND
      mp_xwayland(wlr_xwayland_create(mp_display, mp_compositor, 1)),
#endif
      mp_layer_shell(wlr_layer_shell_v1_create(mp_display)),
      mp_xdg_shell(wlr_xdg_shell_create(mp_display)),
      mp_cursor_manager(wlr_xcursor_manager_create(NULL, 24)),
      mp_presentation(wlr_presentation_create(mp_display, mp_backend)),
      mp_idle(wlr_idle_create(mp_display)),
      mp_idle_inhibit_manager(wlr_idle_inhibit_v1_create(mp_display)),
      mp_server_decoration_manager(wlr_server_decoration_manager_create(mp_display)),
      mp_xdg_decoration_manager(wlr_xdg_decoration_manager_v1_create(mp_display)),
      mp_output_manager(wlr_output_manager_v1_create(mp_display)),
      mp_input_inhibit_manager(wlr_input_inhibit_manager_create(mp_display)),
      mp_keyboard_shortcuts_inhibit_manager(wlr_keyboard_shortcuts_inhibit_v1_create(mp_display)),
      mp_pointer_constraints(wlr_pointer_constraints_v1_create(mp_display)),
      mp_relative_pointer_manager(wlr_relative_pointer_manager_v1_create(mp_display)),
      mp_virtual_pointer_manager(wlr_virtual_pointer_manager_v1_create(mp_display)),
      mp_virtual_keyboard_manager(wlr_virtual_keyboard_manager_v1_create(mp_display)),
      ml_new_output({ .notify = Server::handle_new_output }),
      ml_output_manager_apply({ .notify = Server::handle_output_manager_apply }),
      ml_output_manager_test({ .notify = Server::handle_output_manager_test }),
      ml_new_xdg_surface({ .notify = Server::handle_new_xdg_surface }),
      ml_new_layer_shell_surface({ .notify = Server::handle_new_layer_shell_surface }),
      ml_xdg_activation({ .notify = Server::handle_xdg_activation }),
      ml_new_input({ .notify = Server::handle_new_input }),
      ml_inhibit_activate({ .notify = Server::handle_inhibit_activate }),
      ml_inhibit_deactivate({ .notify = Server::handle_inhibit_deactivate }),
      ml_idle_inhibitor_create({ .notify = Server::handle_idle_inhibitor_create }),
      ml_idle_inhibitor_destroy({ .notify = Server::handle_idle_inhibitor_destroy }),
#ifdef XWAYLAND
      ml_xwayland_ready({ .notify = Server::handle_xwayland_ready }),
      ml_new_xwayland_surface({ .notify = Server::handle_new_xwayland_surface }),
#endif
      m_socket(wl_display_add_socket_auto(mp_display))
{
    TRACE();

    if (m_socket.empty()) {
        wlr_backend_destroy(mp_backend);
        wlr_backend_destroy(mp_headless_backend);
        wl_display_destroy(mp_display);
        std::exit(1);
        spdlog::critical("Could not set up server socket");
        return;
    }

    wlr_multi_backend_add(mp_backend, mp_headless_backend);
    wlr_export_dmabuf_manager_v1_create(mp_display);
    wlr_screencopy_manager_v1_create(mp_display);
    wlr_data_control_manager_v1_create(mp_display);
    wlr_gamma_control_manager_v1_create(mp_display);
    wlr_primary_selection_v1_device_manager_create(mp_display);

    wlr_scene_attach_output_layout(mp_scene, m_root.mp_output_layout);
    wlr_cursor_attach_output_layout(m_seat.mp_cursor, m_root.mp_output_layout);
    wlr_xcursor_manager_load(mp_cursor_manager, 1);
    wlr_scene_set_presentation(mp_scene, wlr_presentation_create(mp_display, mp_backend));
    wlr_server_decoration_manager_set_default_mode(
        mp_server_decoration_manager,
        WLR_SERVER_DECORATION_MANAGER_MODE_SERVER
    );

    wl_signal_add(&mp_backend->events.new_output, &ml_new_output);
    wl_signal_add(&mp_layer_shell->events.new_surface, &ml_new_layer_shell_surface);
    wl_signal_add(&mp_xdg_shell->events.new_surface, &ml_new_xdg_surface);
    wl_signal_add(&mp_idle_inhibit_manager->events.new_inhibitor, &ml_idle_inhibitor_create);
    wl_signal_add(&mp_backend->events.new_input, &ml_new_input);
    wl_signal_add(&mp_output_manager->events.apply, &ml_output_manager_apply);
    wl_signal_add(&mp_output_manager->events.test, &ml_output_manager_test);
    wl_signal_add(&mp_input_inhibit_manager->events.activate, &ml_inhibit_activate);
    wl_signal_add(&mp_input_inhibit_manager->events.deactivate, &ml_inhibit_deactivate);

    // TODO: mp_keyboard_shortcuts_inhibit_manager signals
    // TODO: mp_pointer_constraints signals
    // TODO: mp_relative_pointer_manager signals
    // TODO: mp_virtual_pointer_manager signals
    // TODO: mp_virtual_keyboard_manager signals

#ifdef XWAYLAND
    if (mp_xwayland) {
        wl_signal_add(&mp_xwayland->events.ready, &ml_xwayland_ready);
        wl_signal_add(&mp_xwayland->events.new_surface, &ml_new_xwayland_surface);
        setenv("DISPLAY", mp_xwayland->display_name, 1);
    } else {
        spdlog::error("Failed to initiate XWayland");
        spdlog::warn("Continuing without XWayland functionality");
    }
#endif

    if (!wlr_backend_start(mp_backend)) {
        wlr_backend_destroy(mp_backend);
        wlr_backend_destroy(mp_headless_backend);
        wl_display_destroy(mp_display);
        spdlog::critical("Could not start backend");
        std::exit(1);
        return;
    }

    setenv("WAYLAND_DISPLAY", m_socket.c_str(), true);
    setenv("XDG_CURRENT_DESKTOP", "kranewl", true);

    spdlog::info("Server initiated on WAYLAND_DISPLAY=" + m_socket);
}

Server::~Server()
{
    TRACE();

    wl_display_destroy_clients(mp_display);
    wl_display_destroy(mp_display);
}

void
Server::run() noexcept
{
    TRACE();

    wl_display_run(mp_display);
}

void
Server::handle_new_output(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_output);
    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);

    if (wlr_output == server->m_root.mp_fallback_output->mp_wlr_output)
        return;

    if (!wlr_output_init_render(wlr_output, server->mp_allocator, server->mp_renderer)) {
        spdlog::error("Could not initialize rendering to output");
        spdlog::warn("Ignoring output " + std::string(wlr_output->name));
        return;
    }

    if (!wl_list_empty(&wlr_output->modes)) {
        wlr_output_set_mode(wlr_output, wlr_output_preferred_mode(wlr_output));
        wlr_output_enable_adaptive_sync(wlr_output, true);
        wlr_output_enable(wlr_output, true);

        if (!wlr_output_commit(wlr_output))
            return;
    }

    Output_ptr output = server->mp_model->create_output(
        wlr_output,
        wlr_scene_output_create(server->mp_scene, wlr_output)
    );

    wlr_output->data = output;
    wlr_output_layout_add_auto(server->m_root.mp_output_layout, wlr_output);
}

void
Server::handle_output_layout_change(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_output_manager_apply(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_output_manager_test(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_new_xdg_surface(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_new_layer_shell_surface(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_activation(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_new_input(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_inhibit_activate(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_inhibit_deactivate(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_idle_inhibitor_create(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_idle_inhibitor_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_map(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_unmap(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_request_move(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_request_resize(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xdg_toplevel_handle_moveresize(Client_ptr, CursorMode, uint32_t)
{
    TRACE();

}

#ifdef XWAYLAND
void
Server::handle_xwayland_ready(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_new_xwayland_surface(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xwayland_request_activate(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xwayland_request_configure(struct wl_listener*, void*)
{
    TRACE();

}

void
Server::handle_xwayland_set_hints(struct wl_listener*, void*)
{
    TRACE();

}
#endif
