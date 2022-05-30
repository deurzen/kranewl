#include <trace.hh>

#include <kranewl/server.hh>

#include <kranewl/exec.hh>
#include <kranewl/input/keyboard.hh>
#include <kranewl/model.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg_view.hh>
#ifdef XWAYLAND
#include <kranewl/tree/xwayland_view.hh>
#endif

#include <spdlog/spdlog.h>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
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
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#ifdef XWAYLAND
#define Cursor Cursor_
#include <X11/Xlib.h>
#undef Cursor
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
      mp_renderer([](struct wl_display* display, struct wlr_backend* backend) {
          struct wlr_renderer* renderer = wlr_renderer_autocreate(backend);
          wlr_renderer_init_wl_display(renderer, display);
          return renderer;
      }(mp_display, mp_backend)),
      mp_allocator(wlr_allocator_autocreate(mp_backend, mp_renderer)),
      mp_compositor(wlr_compositor_create(mp_display, mp_renderer)),
      mp_data_device_manager(wlr_data_device_manager_create(mp_display)),
      mp_output_layout([this]() {
          struct wlr_output_layout* output_layout = wlr_output_layout_create();
          wlr_xdg_output_manager_v1_create(mp_display, output_layout);
          return output_layout;
      }()),
      mp_scene([this]() {
          struct wlr_scene* scene = wlr_scene_create();
          wlr_scene_attach_output_layout(scene, mp_output_layout);
          return scene;
      }()),
      m_layers{
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node,
          &wlr_scene_tree_create(&mp_scene->node)->node
      },
      m_seat([this]() {
          struct wlr_cursor* cursor = wlr_cursor_create();
          wlr_cursor_attach_output_layout(cursor, mp_output_layout);
          return Seat{
              this,
              mp_model,
              wlr_seat_create(mp_display, "seat0"),
              wlr_idle_create(mp_display),
              cursor,
              wlr_input_inhibit_manager_create(mp_display),
              wlr_idle_inhibit_v1_create(mp_display),
              wlr_pointer_constraints_v1_create(mp_display),
              wlr_relative_pointer_manager_v1_create(mp_display),
              wlr_virtual_pointer_manager_v1_create(mp_display),
              wlr_virtual_keyboard_manager_v1_create(mp_display),
              wlr_keyboard_shortcuts_inhibit_v1_create(mp_display)
          };
      }()),
#ifdef XWAYLAND
      mp_xwayland(wlr_xwayland_create(mp_display, mp_compositor, 1)),
#endif
      mp_layer_shell(wlr_layer_shell_v1_create(mp_display)),
      mp_xdg_shell(wlr_xdg_shell_create(mp_display)),
      mp_presentation(wlr_presentation_create(mp_display, mp_backend)),
      mp_server_decoration_manager(wlr_server_decoration_manager_create(mp_display)),
      mp_xdg_decoration_manager(wlr_xdg_decoration_manager_v1_create(mp_display)),
      mp_output_manager(wlr_output_manager_v1_create(mp_display)),
      ml_new_output({ .notify = Server::handle_new_output }),
      ml_output_layout_change({ .notify = Server::handle_output_layout_change }),
      ml_output_manager_apply({ .notify = Server::handle_output_manager_apply }),
      ml_output_manager_test({ .notify = Server::handle_output_manager_test }),
      ml_new_xdg_surface({ .notify = Server::handle_new_xdg_surface }),
      ml_new_layer_shell_surface({ .notify = Server::handle_new_layer_shell_surface }),
      ml_xdg_activation({ .notify = Server::handle_xdg_activation }),
      ml_new_input({ .notify = Server::handle_new_input }),
      ml_xdg_new_toplevel_decoration({ .notify = Server::handle_xdg_new_toplevel_decoration }),
#ifdef XWAYLAND
      ml_xwayland_ready({ .notify = Server::handle_xwayland_ready }),
      ml_new_xwayland_surface({ .notify = Server::handle_new_xwayland_surface }),
#endif
      m_socket(wl_display_add_socket_auto(mp_display))
{
    TRACE();

    if (m_socket.empty()) {
        wlr_backend_destroy(mp_backend);
        wl_display_destroy(mp_display);
        std::exit(1);
        spdlog::critical("Could not set up server socket");
        return;
    }

    wlr_export_dmabuf_manager_v1_create(mp_display);
    wlr_screencopy_manager_v1_create(mp_display);
    wlr_data_control_manager_v1_create(mp_display);
    wlr_gamma_control_manager_v1_create(mp_display);
    wlr_primary_selection_v1_device_manager_create(mp_display);

    wlr_server_decoration_manager_set_default_mode(
        mp_server_decoration_manager,
        WLR_SERVER_DECORATION_MANAGER_MODE_SERVER
    );

    wl_signal_add(&mp_backend->events.new_output, &ml_new_output);
    wl_signal_add(&mp_output_layout->events.change, &ml_output_layout_change);
    wl_signal_add(&mp_layer_shell->events.new_surface, &ml_new_layer_shell_surface);
    wl_signal_add(&mp_xdg_shell->events.new_surface, &ml_new_xdg_surface);
    wl_signal_add(&mp_xdg_decoration_manager->events.new_toplevel_decoration, &ml_xdg_new_toplevel_decoration);
    wl_signal_add(&mp_backend->events.new_input, &ml_new_input);
    wl_signal_add(&mp_output_manager->events.apply, &ml_output_manager_apply);
    wl_signal_add(&mp_output_manager->events.test, &ml_output_manager_test);

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
        wl_display_destroy(mp_display);
        spdlog::critical("Could not start backend");
        std::exit(1);
        return;
    }

    setenv("WAYLAND_DISPLAY", m_socket.c_str(), true);
    setenv("XDG_CURRENT_DESKTOP", "kranewl", true);

    spdlog::info("Server initiated on WAYLAND_DISPLAY={}", m_socket);
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
Server::terminate() noexcept
{
    TRACE();

    wl_display_terminate(mp_display);
}

void
Server::relinquish_focus()
{
    struct wlr_surface* focused_surface
        = m_seat.mp_wlr_seat->keyboard_state.focused_surface;

    if (focused_surface) {
        if (wlr_surface_is_layer_surface(focused_surface)) {
            struct wlr_layer_surface_v1* wlr_layer_surface
                = wlr_layer_surface_v1_from_wlr_surface(focused_surface);

            if (wlr_layer_surface->mapped && (
                wlr_layer_surface->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP ||
                wlr_layer_surface->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY))
            {
                return;
            }
        } else {
            struct wlr_scene_node* node
                = reinterpret_cast<struct wlr_scene_node*>(focused_surface->data);

            struct wlr_xdg_surface* wlr_xdg_surface;
            if (wlr_surface_is_xdg_surface(focused_surface)
                    && (wlr_xdg_surface = wlr_xdg_surface_from_wlr_surface(focused_surface))
                    && wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
            {
                wlr_xdg_toplevel_set_activated(wlr_xdg_surface, false);
            }
        }
    }

    wlr_seat_keyboard_notify_clear_focus(m_seat.mp_wlr_seat);
}

void
Server::handle_new_output(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_output);
    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);

    if (!wlr_output_init_render(wlr_output, server->mp_allocator, server->mp_renderer)) {
        spdlog::error("Could not initialize rendering to output");
        spdlog::warn("Ignoring output {}", std::string(wlr_output->name));
        return;
    }

    if (!wl_list_empty(&wlr_output->modes)) {
        wlr_output_set_mode(wlr_output, wlr_output_preferred_mode(wlr_output));
        wlr_output_enable_adaptive_sync(wlr_output, true);
        wlr_output_enable(wlr_output, true);

        if (!wlr_output_commit(wlr_output))
            return;
    }

    wlr_output_layout_add_auto(server->mp_output_layout, wlr_output);
    struct wlr_box output_box
        = *wlr_output_layout_get_box(server->mp_output_layout, wlr_output);

    Output_ptr output = server->mp_model->create_output(
        wlr_output,
        wlr_scene_output_create(server->mp_scene, wlr_output),
        Region{
            .pos = Pos{
                .x = output_box.x,
                .y = output_box.y
            },
            .dim = Dim{
                .w = output_box.width,
                .h = output_box.height
            }
        }
    );

    wlr_output->data = output;
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

static inline View_ptr
view_from_popup(struct wlr_xdg_popup* popup)
{
    struct wlr_xdg_surface* surface = popup->base;

    for (;;)
        switch (surface->role) {
        case WLR_XDG_SURFACE_ROLE_POPUP:
        {
            if (!wlr_surface_is_xdg_surface(surface->popup->parent))
                return nullptr;

            surface = wlr_xdg_surface_from_wlr_surface(surface->popup->parent);
            break;
        }
        case WLR_XDG_SURFACE_ROLE_TOPLEVEL: return reinterpret_cast<View_ptr>(surface->data);
        case WLR_XDG_SURFACE_ROLE_NONE:     return nullptr;
        }
}

void
Server::handle_new_xdg_surface(struct wl_listener* listener, void* data)
{
    TRACE();

    View_ptr view;
    Server_ptr server = wl_container_of(listener, server, ml_new_xdg_surface);
    struct wlr_xdg_surface* xdg_surface
        = reinterpret_cast<struct wlr_xdg_surface*>(data);

    switch (xdg_surface->role) {
    case WLR_XDG_SURFACE_ROLE_POPUP:
    {
        struct wlr_box mappable_box;
        struct wlr_scene_node* parent_node
            = reinterpret_cast<struct wlr_scene_node*>(xdg_surface->popup->parent->data);

        xdg_surface->data
            = wlr_scene_xdg_surface_create(parent_node, xdg_surface);

        if (!(view = view_from_popup(xdg_surface->popup)) || !view->mp_output)
            return;

        Region const& active_region = view->active_region();
        mappable_box = view->mp_output->placeable_region();
        mappable_box.x -= active_region.pos.x;
        mappable_box.y -= active_region.pos.y;

        wlr_xdg_popup_unconstrain_from_box(xdg_surface->popup, &mappable_box);
        return;
    }
    case WLR_XDG_SURFACE_ROLE_NONE: return;
    default: break;
    }

    xdg_surface->data = view = server->mp_model->create_xdg_shell_view(
        xdg_surface,
        &server->m_seat
    );
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
Server::handle_new_input(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_input);
    struct wlr_input_device* device
        = reinterpret_cast<struct wlr_input_device*>(data);

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
    {
        Keyboard_ptr keyboard = server->m_seat.create_keyboard(device);

        struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        struct xkb_keymap* keymap
            = xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);

        wlr_keyboard_set_keymap(device->keyboard, keymap);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        wlr_keyboard_set_repeat_info(device->keyboard, 200, 100);
        wlr_seat_set_keyboard(server->m_seat.mp_wlr_seat, device);

        break;
    }
    case WLR_INPUT_DEVICE_POINTER:
    {
        wlr_cursor_attach_input_device(
            server->m_seat.mp_cursor->mp_wlr_cursor,
            device
        );
        break;
    }
    default: break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!server->m_seat.m_keyboards.empty())
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    wlr_seat_set_capabilities(server->m_seat.mp_wlr_seat, caps);
}

void
Server::handle_xdg_new_toplevel_decoration(struct wl_listener*, void* data)
{
    TRACE();

    struct wlr_xdg_toplevel_decoration_v1* decoration
        = reinterpret_cast<struct wlr_xdg_toplevel_decoration_v1*>(data);

    wlr_xdg_toplevel_decoration_v1_set_mode(
        decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
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
