#include <trace.hh>

#include <kranewl/server.hh>

#include <kranewl/exec.hh>
#include <kranewl/input/keyboard.hh>
#include <kranewl/manager.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg-view.hh>
#include <kranewl/xdg-decoration.hh>
#ifdef XWAYLAND
#include <kranewl/tree/xwayland-view.hh>
#include <kranewl/tree/xwayland-unmanaged.hh>
#include <kranewl/xwayland.hh>
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
#include <wlr/backend/headless.h>
#include <wlr/backend/libinput.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/session.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm_lease_v1.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_foreign_registry.h>
#include <wlr/types/wlr_xdg_foreign_registry.h>
#include <wlr/types/wlr_xdg_foreign_v1.h>
#include <wlr/types/wlr_xdg_foreign_v2.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#ifdef XWAYLAND
#include <wlr/xwayland.h>
#endif
}
#undef static
#undef namespace
#undef class

extern "C" {
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdlib>

static inline bool
drop_privileges()
{
    if (getuid() != geteuid() || getgid() != getegid())
        if (setuid(getuid()) || setgid(getgid()))
            return false;

    if (!geteuid() || !getegid()) {
        spdlog::error("Running as root is prohibited");
        return false;
    }

    if (setgid(0) != -1 || setuid(0) != -1) {
        spdlog::error("Unable to drop root privileges");
        return false;
    }

    return true;
}

Server::Server(Manager_ptr manager)
    : mp_manager([this,manager]() {
        manager->register_server(this);
        return manager;
      }()),
      m_pending_output_layout_changes(0),
      ml_new_output({ .notify = Server::handle_new_output }),
      ml_output_layout_change({ .notify = Server::handle_output_layout_change }),
      ml_output_manager_apply({ .notify = Server::handle_output_manager_apply }),
      ml_output_manager_test({ .notify = Server::handle_output_manager_test }),
      ml_new_xdg_surface({ .notify = Server::handle_new_xdg_surface }),
      ml_new_layer_shell_surface({ .notify = Server::handle_new_layer_shell_surface }),
      ml_new_input({ .notify = Server::handle_new_input }),
      ml_new_xdg_toplevel_decoration({ .notify = Server::handle_new_xdg_toplevel_decoration }),
      ml_xdg_request_activate({ .notify = Server::handle_xdg_request_activate }),
      ml_new_virtual_keyboard({ .notify = Server::handle_new_virtual_keyboard }),
      ml_drm_lease_request({ .notify = Server::handle_drm_lease_request })
{}

Server::~Server()
{
    TRACE();

    delete mp_seat;
#ifdef XWAYLAND
    delete mp_xwayland;
#endif

#ifdef XWAYLAND
    wlr_xwayland_destroy(mp_xwayland->mp_wlr_xwayland);
#endif
    wl_display_destroy_clients(mp_display);
    wl_display_destroy(mp_display);
}

static inline void
err(struct wl_display* display, std::string const&& message)
{
    spdlog::critical(message);
    wl_display_destroy_clients(display);
    wl_display_destroy(display);
    std::exit(EXIT_FAILURE);
}

void
Server::initialize()
{
    TRACE();

    mp_display = wl_display_create();
    mp_event_loop = wl_display_get_event_loop(mp_display);

    mp_backend = wlr_backend_autocreate(mp_display);
    if (!mp_backend) {
        err(mp_display, "Could not autocreate backend");
        return;
    }

    if (!drop_privileges()) {
        err(mp_display, "Could not drop privileges");
        return;
    }

    mp_output_layout = wlr_output_layout_create();
    wlr_xdg_output_manager_v1_create(mp_display, mp_output_layout);

    mp_renderer = wlr_renderer_autocreate(mp_backend);
    wlr_renderer_init_wl_display(mp_renderer, mp_display);
    if (!mp_renderer) {
        wlr_backend_destroy(mp_backend);
        err(mp_display, "Could not autocreate renderer");
        return;
    }

    mp_allocator = wlr_allocator_autocreate(mp_backend, mp_renderer);
    if (!mp_allocator) {
        wlr_backend_destroy(mp_backend);
        err(mp_display, "Could not autocreate allocator");
        return;
    }

    mp_compositor = wlr_compositor_create(mp_display, mp_renderer);
    wlr_subcompositor_create(mp_display);
    mp_data_device_manager = wlr_data_device_manager_create(mp_display);

    mp_scene = wlr_scene_create();
    wlr_scene_attach_output_layout(mp_scene, mp_output_layout);
    m_scene_layers = {
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree),
        wlr_scene_tree_create(&mp_scene->tree)
    };

    struct wlr_cursor* cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor, mp_output_layout);
    mp_seat = new Seat(
        this,
        mp_manager,
        wlr_seat_create(mp_display, "seat0"),
        wlr_idle_create(mp_display),
        cursor,
        wlr_input_inhibit_manager_create(mp_display),
        wlr_idle_inhibit_v1_create(mp_display),
        wlr_virtual_keyboard_manager_v1_create(mp_display)
    );

#ifdef XWAYLAND
    mp_wlr_xwayland = wlr_xwayland_create(mp_display, mp_compositor, true);
    mp_xwayland = new XWayland(mp_wlr_xwayland, this, mp_manager, mp_seat);
#endif

    mp_layer_shell = wlr_layer_shell_v1_create(mp_display);
    mp_xdg_activation = wlr_xdg_activation_v1_create(mp_display);
    mp_xdg_shell = wlr_xdg_shell_create(mp_display, XDG_SHELL_VERSION);
    mp_presentation = wlr_presentation_create(mp_display, mp_backend);
    mp_server_decoration_manager = wlr_server_decoration_manager_create(mp_display);
    mp_xdg_decoration_manager = wlr_xdg_decoration_manager_v1_create(mp_display);
    mp_virtual_keyboard_manager = wlr_virtual_keyboard_manager_v1_create(mp_display);
    mp_output_manager = wlr_output_manager_v1_create(mp_display);
    mp_foreign_registry = wlr_xdg_foreign_registry_create(mp_display);
    mp_drm_lease_manager
        = wlr_drm_lease_v1_manager_create(mp_display, mp_backend);

    wlr_server_decoration_manager_set_default_mode(
        mp_server_decoration_manager,
        WLR_SERVER_DECORATION_MANAGER_MODE_SERVER
    );
    wlr_export_dmabuf_manager_v1_create(mp_display);
    wlr_screencopy_manager_v1_create(mp_display);
    wlr_data_control_manager_v1_create(mp_display);
    wlr_gamma_control_manager_v1_create(mp_display);
    wlr_primary_selection_v1_device_manager_create(mp_display);
    wlr_viewporter_create(mp_display);
    wlr_xdg_foreign_v1_create(mp_display, mp_foreign_registry);
    wlr_xdg_foreign_v2_create(mp_display, mp_foreign_registry);

    m_socket = wl_display_add_socket_auto(mp_display);
    if (m_socket.empty()) {
        wlr_backend_destroy(mp_backend);
        err(mp_display, "Could not set up server socket");
        return;
    }

    mp_headless_backend = wlr_headless_backend_create(mp_display);
    if (!mp_headless_backend) {
        wlr_backend_destroy(mp_backend);
        err(mp_display, "Could not create headless backend");
        return;
    } else
        wlr_multi_backend_add(mp_backend, mp_headless_backend);

    mp_fallback_output
        = wlr_headless_add_output(mp_headless_backend, 800, 600);
    wlr_output_set_name(mp_fallback_output, "FALLBACK");

    setenv("WAYLAND_DISPLAY", m_socket.c_str(), true);
    setenv("XDG_CURRENT_DESKTOP", "kranewl", true);

    spdlog::info("Server initialized at WAYLAND_DISPLAY={}", m_socket);
}

void
Server::start()
{
    spdlog::info("Linking signal handlers");
    wl_signal_add(&mp_backend->events.new_output, &ml_new_output);
    wl_signal_add(&mp_output_layout->events.change, &ml_output_layout_change);
    wl_signal_add(&mp_output_manager->events.apply, &ml_output_manager_apply);
    wl_signal_add(&mp_output_manager->events.test, &ml_output_manager_test);

    wl_signal_add(&mp_xdg_shell->events.new_surface, &ml_new_xdg_surface);
    wl_signal_add(&mp_layer_shell->events.new_surface, &ml_new_layer_shell_surface);
    wl_signal_add(&mp_backend->events.new_input, &ml_new_input);
    wl_signal_add(&mp_xdg_decoration_manager->events.new_toplevel_decoration, &ml_new_xdg_toplevel_decoration);
    wl_signal_add(&mp_xdg_activation->events.request_activate, &ml_xdg_request_activate);

    wl_signal_add(&mp_virtual_keyboard_manager->events.new_virtual_keyboard, &ml_new_virtual_keyboard);

    if (mp_drm_lease_manager)
        wl_signal_add(&mp_drm_lease_manager->events.request, &ml_drm_lease_request);
    else {
        spdlog::error("Could not create wlr_drm_lease_device_v1");
        spdlog::warn("VR will not be available");
    }

    spdlog::info("Starting backend");
    if (!wlr_backend_start(mp_backend)) {
        wlr_backend_destroy(mp_backend);
        err(mp_display, "Could not start backend");
        return;
    }
}

void
Server::run()
{
    TRACE();
    spdlog::info("Running compositor");
    wl_display_run(mp_display);
}

void
Server::terminate()
{
    TRACE();
    wl_display_terminate(mp_display);
}

void
Server::relinquish_focus()
{
    struct wlr_surface* focused_surface
        = mp_seat->mp_wlr_seat->keyboard_state.focused_surface;

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
                wlr_xdg_toplevel_set_activated(wlr_xdg_surface->toplevel, false);
            }
        }
    }

    wlr_seat_keyboard_notify_clear_focus(mp_seat->mp_wlr_seat);
}

static inline struct wlr_output_configuration_v1*
create_output_config(Server_ptr server)
{
    struct wlr_output_configuration_v1* config
        = wlr_output_configuration_v1_create();

    if (!config) {
        spdlog::error("Could not create new output configuration");
        return nullptr;
    }

    for (Output_ptr output : server->mp_manager->outputs()) {
        struct wlr_output_configuration_head_v1* config_head
            = wlr_output_configuration_head_v1_create(config, output->mp_wlr_output);

        if (!config_head) {
            spdlog::error("Could not create new output configuration head");
            wlr_output_configuration_v1_destroy(config);
            return nullptr;
        }

        struct wlr_box output_box;
        wlr_output_layout_get_box(
            server->mp_output_layout,
            output->mp_wlr_output,
            &output_box
        );

        if (!wlr_box_empty(&output_box)) {
            config_head->state.x = output_box.x;
            config_head->state.y = output_box.y;
        } else
            spdlog::error("Could not retrieve output layout box");
    }

    return config;
}

static inline void
output_update_for_layout_change(Server_ptr server)
{
    // TODO: clamp views to output
    server->mp_seat->mp_cursor->move_cursor({0, 0});
}

static inline void
update_output_manager_config(Server_ptr server)
{
    if (server->m_pending_output_layout_changes <= 0) {
        struct wlr_output_configuration_v1* config
            = create_output_config(server);

        if (config)
            wlr_output_manager_v1_set_configuration(
                server->mp_output_manager,
                config
            );
        else
            spdlog::error("Could not set output manager configuration");

        output_update_for_layout_change(server);
    }
}

void
Server::handle_new_output(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_output);
    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);

    if (wlr_output == server->mp_fallback_output)
        return;

    if (server->mp_drm_lease_manager)
        wlr_drm_lease_v1_manager_offer_output(
            server->mp_drm_lease_manager,
            wlr_output
        );

    if (wlr_output->non_desktop) {
        spdlog::warn("Detected non-desktop output; not configuring");
        return;
    }

    if (!wlr_output_init_render(wlr_output, server->mp_allocator, server->mp_renderer)) {
        spdlog::error("Could not initialize rendering to output");
        spdlog::warn("Ignoring output {}", std::string(wlr_output->name));
        return;
    }

    wlr_output_enable(wlr_output, true);
    spdlog::info("Enabled output {}", std::string(wlr_output->name));

    struct wlr_output_mode* preferred_mode = nullptr;
    if (!wl_list_empty(&wlr_output->modes)) {
        preferred_mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, preferred_mode);
    }

    if (!wlr_output_test(wlr_output)) {
        spdlog::error("Output rejected preferred mode, falling back to another mode");
        struct wlr_output_mode* mode;
        wl_list_for_each(mode, &wlr_output->modes, link) {
            if (mode == preferred_mode)
                continue;

            wlr_output_set_mode(wlr_output, mode);

            if (wlr_output_test(wlr_output))
                break;
        }
    }

    wlr_output_enable_adaptive_sync(wlr_output, true);
    if (!wlr_output_test(wlr_output)) {
        wlr_output_enable_adaptive_sync(wlr_output, false);
        spdlog::warn("Could not enable adaptive sync for output {}", std::string(wlr_output->name));
    } else
        spdlog::info("Adaptive sync enabled for output {}", std::string(wlr_output->name));

    if (!wlr_output_commit(wlr_output)) {
        spdlog::error("Could not commit to output");
        spdlog::warn("Ignoring output {}", std::string(wlr_output->name));
        return;
    }

    ++server->m_pending_output_layout_changes;

    wlr_output_layout_add_auto(server->mp_output_layout, wlr_output);
    struct wlr_scene_output* wlr_scene_output
        = wlr_scene_get_scene_output(server->mp_scene, wlr_output);

    struct wlr_box output_box;
    wlr_output_layout_get_box(
        server->mp_output_layout,
        wlr_output,
        &output_box
    );

    Output_ptr output = server->mp_manager->create_output(
        wlr_output,
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

    --server->m_pending_output_layout_changes;
    update_output_manager_config(server);
}

void
Server::handle_output_layout_change(struct wl_listener* listener, void* data)
{
    TRACE();
    Server_ptr server = wl_container_of(listener, server, ml_output_layout_change);
    update_output_manager_config(server);
}


static inline Output_ptr
output_from_wlr_output(Manager_ptr manager, struct wlr_output* wlr_output)
{
    for (Output_ptr output : manager->outputs()) {
        if (output->mp_wlr_output == wlr_output)
            return output;
    }

    return nullptr;
}

static inline bool
apply_or_test_output_config(
    Server_ptr server,
    struct wlr_output_configuration_v1* config,
    bool test
)
{
    ++server->m_pending_output_layout_changes;

    struct wlr_output_configuration_head_v1* config_head;
    bool configuration_succeeded = true;

    wl_list_for_each(config_head, &config->heads, link) {
        struct wlr_output* wlr_output = config_head->state.output;
        Output_ptr output = output_from_wlr_output(server->mp_manager, wlr_output);

        wlr_output_enable(wlr_output, config_head->state.enabled);

        if (config_head->state.enabled) {
            if (config_head->state.mode)
                wlr_output_set_mode(wlr_output, config_head->state.mode);
            else {
                wlr_output_set_custom_mode(
                    wlr_output,
                    config_head->state.custom_mode.width,
                    config_head->state.custom_mode.height,
                    config_head->state.custom_mode.refresh
                );
            }

            wlr_output_set_scale(wlr_output, config_head->state.scale);
            wlr_output_set_transform(wlr_output, config_head->state.transform);
            wlr_output_enable_adaptive_sync(wlr_output, config_head->state.adaptive_sync_enabled);
        }

        if (!wlr_output_commit(wlr_output)) {
            spdlog::error("Could not commit to output");
            spdlog::warn("Ignoring reconfiguration of output {}", std::string(wlr_output->name));
            continue;
        }

        // output needs to be added
        if (config_head->state.enabled && output && !output->enabled())
            wlr_output_layout_add_auto(server->mp_output_layout, wlr_output);

        if (config_head->state.enabled) {
            struct wlr_box output_box = {0};
            wlr_output_layout_get_box(
                server->mp_output_layout,
                wlr_output,
                &output_box
            );

            if (output_box.x != config_head->state.x || output_box.y != config_head->state.y)
                wlr_output_layout_move(
                    server->mp_output_layout,
                    wlr_output,
                    config_head->state.x,
                    config_head->state.y
                );
        }

        // output needs to be removed
        if (!config_head->state.enabled && (!output || output->enabled()))
            wlr_output_layout_remove(server->mp_output_layout, wlr_output);

        if (test) {
            configuration_succeeded &= wlr_output_test(wlr_output);
            wlr_output_rollback(wlr_output);
        } else {
            configuration_succeeded &= wlr_output_commit(wlr_output);
        }
    }

    --server->m_pending_output_layout_changes;
    update_output_manager_config(server);

    return configuration_succeeded;
}

static inline void
handle_output_manager_apply_or_test(
    Server_ptr server,
    struct wlr_output_configuration_v1* config,
    bool test
)
{
    TRACE();

    if (apply_or_test_output_config(server, config, test))
        wlr_output_configuration_v1_send_succeeded(config);
    else
        wlr_output_configuration_v1_send_failed(config);

    wlr_output_configuration_v1_destroy(config);

    for (Output_ptr output : server->mp_manager->outputs())
        server->mp_seat->mp_cursor->load_output_cursor(
            output->mp_wlr_output->scale
        );
}

void
Server::handle_output_manager_apply(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_output_manager_apply);
    struct wlr_output_configuration_v1* config
        = reinterpret_cast<struct wlr_output_configuration_v1*>(data);

    handle_output_manager_apply_or_test(server, config, false);
}

void
Server::handle_output_manager_test(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_output_manager_test);
    struct wlr_output_configuration_v1* config
        = reinterpret_cast<struct wlr_output_configuration_v1*>(data);

    handle_output_manager_apply_or_test(server, config, true);
}

static inline XDGView_ptr
xdg_view_from_popup(struct wlr_xdg_popup* popup)
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
        case WLR_XDG_SURFACE_ROLE_TOPLEVEL: return reinterpret_cast<XDGView_ptr>(surface->data);
        case WLR_XDG_SURFACE_ROLE_NONE:     return nullptr;
        }
}

void
Server::handle_new_xdg_surface(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_xdg_surface);
    struct wlr_xdg_surface* xdg_surface
        = reinterpret_cast<struct wlr_xdg_surface*>(data);

    View_ptr view;
    switch (xdg_surface->role) {
    case WLR_XDG_SURFACE_ROLE_POPUP:
    {
        xdg_surface->surface->data = wlr_scene_xdg_surface_create(
            reinterpret_cast<struct wlr_scene_tree*>(xdg_surface->popup->parent->data),
            xdg_surface
        );

        XDGView_ptr view;
        if (!(view = xdg_view_from_popup(xdg_surface->popup)) || !view->mp_output)
            return;

        Region const& active_region = view->active_region();
        struct wlr_box mappable_box = view->mp_output->placeable_region();
        mappable_box.x -= active_region.pos.x;
        mappable_box.y -= active_region.pos.y;

        wlr_xdg_popup_unconstrain_from_box(xdg_surface->popup, &mappable_box);
        return;
    }
    case WLR_XDG_SURFACE_ROLE_NONE: return;
    default: break;
    }

    xdg_surface->data = view = server->mp_manager->create_xdg_shell_view(
        xdg_surface,
        server->mp_seat
    );
}

void
Server::handle_new_layer_shell_surface(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_layer_shell_surface);
    struct wlr_layer_surface_v1* layer_surface
        = reinterpret_cast<struct wlr_layer_surface_v1*>(data);

    if (!layer_surface->output)
        layer_surface->output = server->mp_manager->mp_output->mp_wlr_output;

    SceneLayer scene_layer;
    switch (layer_surface->pending.layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
        scene_layer = SCENE_LAYER_BACKGROUND;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
        scene_layer = SCENE_LAYER_BOTTOM;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
        scene_layer = SCENE_LAYER_TOP;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
        scene_layer = SCENE_LAYER_OVERLAY;
        break;
    default:
        spdlog::error("No applicable scene layer found for layer surface");
        spdlog::warn("Ignoring layer surface");
        scene_layer = SCENE_LAYER_NONE;
        return;
    }

    Output_ptr output = reinterpret_cast<Output_ptr>(layer_surface->output->data);
    Layer_ptr layer = server->mp_manager->create_layer(
        layer_surface,
        output,
        scene_layer
    );

    layer_surface->data = layer;
    layer_surface->surface->data = layer->mp_scene
        = wlr_scene_subsurface_tree_create(
            server->m_scene_layers[scene_layer],
            layer_surface->surface
        );
    layer->mp_scene->node.data = layer;

    server->mp_manager->register_layer(layer);
    struct wlr_layer_surface_v1_state initial_state = layer->mp_layer_surface->current;
    layer->mp_layer_surface->current = layer->mp_layer_surface->pending;
    layer->mp_output->arrange_layers();
    layer->mp_layer_surface->current = initial_state;
}

static inline void
create_keyboard(Server_ptr server, struct wlr_keyboard* wlr_keyboard)
{
    Keyboard_ptr keyboard = server->mp_seat->create_keyboard(wlr_keyboard);

    struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap* keymap
        = xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(keyboard->mp_wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(keyboard->mp_wlr_keyboard, 100, 200);
    wlr_seat_set_keyboard(server->mp_seat->mp_wlr_seat, keyboard->mp_wlr_keyboard);
}

void
Server::configure_libinput(struct wlr_input_device* device)
{
    if (wlr_input_device_is_libinput(device)) {
        struct libinput_device* libinput_device
            = wlr_libinput_get_device_handle(device);

        libinput_device_config_tap_set_enabled(
            libinput_device,
            LIBINPUT_CONFIG_TAP_ENABLED
        );
    }
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
        create_keyboard(server, wlr_keyboard_from_input_device(device));

        break;
    }
    case WLR_INPUT_DEVICE_POINTER:
    {
        wlr_cursor_attach_input_device(
            server->mp_seat->mp_cursor->mp_wlr_cursor,
            device
        );
        break;
    }
    default: break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!server->mp_seat->m_keyboards.empty())
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    wlr_seat_set_capabilities(server->mp_seat->mp_wlr_seat, caps);
    configure_libinput(device);
}

void
Server::handle_new_xdg_toplevel_decoration(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_xdg_toplevel_decoration);
    struct wlr_xdg_toplevel_decoration_v1* xdg_decoration
        = reinterpret_cast<struct wlr_xdg_toplevel_decoration_v1*>(data);
    XDGView_ptr view = reinterpret_cast<XDGView_ptr>(xdg_decoration->surface->data);

    XDGDecoration_ptr decoration
        = new XDGDecoration(server, server->mp_manager, view, xdg_decoration);
    view->mp_decoration = decoration;
    server->m_decorations[decoration->m_uid] = decoration;

    wl_signal_add(&xdg_decoration->events.destroy, &decoration->ml_destroy);
    wl_signal_add(&xdg_decoration->events.request_mode, &decoration->ml_request_mode);

    XDGDecoration::handle_request_mode(&decoration->ml_request_mode, xdg_decoration);
}

void
Server::handle_xdg_request_activate(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_xdg_request_activate);
    struct wlr_xdg_activation_v1_request_activate_event* event
        = reinterpret_cast<struct wlr_xdg_activation_v1_request_activate_event*>(data);

    if (!wlr_surface_is_xdg_surface(event->surface))
        return;

    XDGView_ptr view
        = reinterpret_cast<XDGView_ptr>(wlr_xdg_surface_from_wlr_surface(event->surface)->data);

    if (!view->focused())
        view->set_urgent(true);
}

void
Server::handle_new_virtual_keyboard(struct wl_listener* listener, void* data)
{
    TRACE();

    Server_ptr server = wl_container_of(listener, server, ml_new_virtual_keyboard);
    struct wlr_virtual_keyboard_v1* virtual_keyboard
        = reinterpret_cast<struct wlr_virtual_keyboard_v1*>(data);

    create_keyboard(server, &virtual_keyboard->keyboard);
}

void
Server::handle_drm_lease_request(struct wl_listener*, void* data)
{
    TRACE();

    struct wlr_drm_lease_request_v1* request
        = reinterpret_cast<struct wlr_drm_lease_request_v1*>(data);
    struct wlr_drm_lease_v1* lease = wlr_drm_lease_request_v1_grant(request);

    if (!lease) {
        spdlog::error("Could not grant DRM lease request");
        wlr_drm_lease_request_v1_reject(request);
    }
}
