#ifdef XWAYLAND
#include <trace.hh>

#include <kranewl/context.hh>
#include <kranewl/manager.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xwayland-view.hh>
#include <kranewl/tree/xwayland-unmanaged.hh>
#include <kranewl/workspace.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/xwayland.h>
}
#undef static
#undef namespace
#undef class

XWaylandUnmanaged::XWaylandUnmanaged(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Server_ptr server,
    Manager_ptr manager,
    Seat_ptr seat,
    XWayland_ptr xwayland
)
    : Node(Type::XdgPopup, reinterpret_cast<std::uintptr_t>(wlr_xwayland_surface)),
      mp_wlr_xwayland_surface(wlr_xwayland_surface),
      mp_server(server),
      mp_manager(manager),
      mp_seat(seat),
      mp_output(nullptr),
      mp_xwayland(xwayland),
      m_region({}),
      mp_wlr_surface(wlr_xwayland_surface->surface),
      m_pid(0),
      ml_map({ .notify = handle_map }),
      ml_unmap({ .notify = handle_unmap }),
      ml_set_override_redirect({ .notify = handle_set_override_redirect }),
      ml_set_geometry({ .notify = handle_set_geometry }),
      ml_request_activate({ .notify = handle_request_activate }),
      ml_request_configure({ .notify = handle_request_configure }),
      ml_request_fullscreen({ .notify = handle_request_fullscreen }),
      ml_destroy({ .notify = handle_destroy })
{
    wl_signal_add(&mp_wlr_xwayland_surface->events.map, &ml_map);
    wl_signal_add(&mp_wlr_xwayland_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_override_redirect, &ml_set_override_redirect);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_activate, &ml_request_activate);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_configure, &ml_request_configure);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_fullscreen, &ml_request_fullscreen);
    wl_signal_add(&mp_wlr_xwayland_surface->events.destroy, &ml_destroy);
}

XWaylandUnmanaged::~XWaylandUnmanaged()
{}

void
XWaylandUnmanaged::format_uid()
{
    std::stringstream uid_ss;
    uid_ss << "0x" << std::hex << uid() << std::dec;
    uid_ss << " [" << m_pid << "]";
    uid_ss << " (XU)";
    m_uid_formatted = uid_ss.str();
}

pid_t
XWaylandUnmanaged::retrieve_pid()
{
    TRACE();
    return mp_wlr_xwayland_surface->pid;
}

void
XWaylandUnmanaged::handle_map(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_map);
    Server_ptr server = unmanaged->mp_server;
    Manager_ptr manager = unmanaged->mp_manager;

    unmanaged->set_pid(unmanaged->retrieve_pid());
    unmanaged->format_uid();

    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);
    unmanaged->mp_wlr_surface = xwayland_surface->surface;

    unmanaged->m_region = Region{
        .pos = Pos{
            .x = xwayland_surface->x,
            .y = xwayland_surface->y,
        },
        .dim = Dim{
            .w = xwayland_surface->width,
            .h = xwayland_surface->height,
        }
    };

    unmanaged->set_instance(xwayland_surface->instance
        ? xwayland_surface->instance : "N/a");
    unmanaged->set_class(xwayland_surface->class_
        ? xwayland_surface->class_ : "N/a");
    unmanaged->set_title(xwayland_surface->title
        ? xwayland_surface->title : "N/a");
    unmanaged->set_app_id(unmanaged->class_());
    unmanaged->set_title_formatted(unmanaged->title()); // TODO: format title

    unmanaged->mp_scene = wlr_scene_tree_create(
        server->m_scene_layers[SCENE_LAYER_TILE]
    );

    unmanaged->mp_wlr_surface->data = unmanaged->mp_scene_surface = wlr_scene_subsurface_tree_create(
        unmanaged->mp_scene,
        unmanaged->mp_wlr_surface
    );
    unmanaged->mp_scene_surface->node.data = unmanaged;

    wlr_scene_node_reparent(
        &unmanaged->mp_scene->node,
        unmanaged->mp_server->m_scene_layers[SCENE_LAYER_FREE]
    );

    wlr_scene_node_set_position(
        &unmanaged->mp_scene->node,
        unmanaged->m_region.pos.x,
        unmanaged->m_region.pos.y
    );

    wl_signal_add(&xwayland_surface->events.set_geometry, &unmanaged->ml_set_geometry);

    struct wlr_seat* wlr_seat = unmanaged->mp_seat->mp_wlr_seat;

    if (wlr_xwayland_or_surface_wants_focus(xwayland_surface)) {
        wlr_xwayland_set_seat(unmanaged->mp_xwayland->mp_wlr_xwayland, wlr_seat);

        struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(wlr_seat);
        if (!keyboard) {
            wlr_seat_keyboard_notify_enter(
                wlr_seat,
                xwayland_surface->surface,
                nullptr,
                0,
                nullptr
            );

            return;
        }

        wlr_seat_keyboard_notify_enter(
            wlr_seat,
            xwayland_surface->surface,
            keyboard->keycodes,
            keyboard->num_keycodes,
            &keyboard->modifiers
        );
    }
}

void
XWaylandUnmanaged::handle_unmap(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_unmap);
    struct wlr_xwayland_surface* xwayland_surface = unmanaged->mp_wlr_xwayland_surface;
    struct wlr_seat* wlr_seat = unmanaged->mp_seat->mp_wlr_seat;

    wl_list_remove(&unmanaged->ml_set_geometry.link);

    if (wlr_seat->keyboard_state.focused_surface == xwayland_surface->surface) {
        if (xwayland_surface->parent && xwayland_surface->parent->surface
                && wlr_xwayland_or_surface_wants_focus(xwayland_surface->parent))
        {
            struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(wlr_seat);
            if (!keyboard) {
                wlr_seat_keyboard_notify_enter(
                    wlr_seat,
                    xwayland_surface->parent->surface,
                    nullptr,
                    0,
                    nullptr
                );

                return;
            }

            wlr_seat_keyboard_notify_enter(
                wlr_seat,
                xwayland_surface->parent->surface,
                keyboard->keycodes,
                keyboard->num_keycodes,
                &keyboard->modifiers
            );

            return;
        }

        unmanaged->mp_manager->refocus();
    }
}

void
XWaylandUnmanaged::handle_set_override_redirect(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_set_override_redirect);
    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);

    if (xwayland_surface->mapped)
        XWaylandUnmanaged::handle_unmap(&unmanaged->ml_unmap, nullptr);

    XWaylandUnmanaged::handle_destroy(&unmanaged->ml_destroy, unmanaged);
    xwayland_surface->data = nullptr;

    XWaylandView_ptr view = unmanaged->mp_manager->create_xwayland_view(
        xwayland_surface,
        unmanaged->mp_seat,
        unmanaged->mp_xwayland
    );

    if (xwayland_surface->mapped)
        XWaylandView::handle_map(&view->ml_map, xwayland_surface);
}

void
XWaylandUnmanaged::handle_set_geometry(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_set_geometry);
    struct wlr_xwayland_surface* xwayland_surface = unmanaged->mp_wlr_xwayland_surface;

    Pos new_pos = Pos{
        .x = xwayland_surface->x,
        .y = xwayland_surface->y,
    };


    if (unmanaged->m_region.pos != new_pos)
    {
        unmanaged->m_region.pos = new_pos;
        wlr_scene_node_set_position(
            &unmanaged->mp_scene->node,
            unmanaged->m_region.pos.x,
            unmanaged->m_region.pos.y
        );
    }
}

void
XWaylandUnmanaged::handle_request_activate(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_request_activate);
    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);

    if (!xwayland_surface->mapped)
        return;

    struct wlr_seat* wlr_seat = unmanaged->mp_seat->mp_wlr_seat;

    if (wlr_xwayland_or_surface_wants_focus(xwayland_surface)) {
        wlr_xwayland_set_seat(unmanaged->mp_xwayland->mp_wlr_xwayland, wlr_seat);

        struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(wlr_seat);
        if (!keyboard) {
            wlr_seat_keyboard_notify_enter(
                wlr_seat,
                xwayland_surface->surface,
                nullptr,
                0,
                nullptr
            );

            return;
        }

        wlr_seat_keyboard_notify_enter(
            wlr_seat,
            xwayland_surface->surface,
            keyboard->keycodes,
            keyboard->num_keycodes,
            &keyboard->modifiers
        );
    }
}

void
XWaylandUnmanaged::handle_request_configure(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_request_configure);
    struct wlr_xwayland_surface_configure_event* event
        = reinterpret_cast<struct wlr_xwayland_surface_configure_event*>(data);

    wlr_xwayland_surface_configure(
        unmanaged->mp_wlr_xwayland_surface,
        event->x,
        event->y,
        event->width,
        event->height
    );
}

void
XWaylandUnmanaged::handle_request_fullscreen(struct wl_listener*, void*)
{
    TRACE();
    // TODO
}

void
XWaylandUnmanaged::handle_destroy(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandUnmanaged_ptr unmanaged = wl_container_of(listener, unmanaged, ml_destroy);

    wl_list_remove(&unmanaged->ml_map.link);
    wl_list_remove(&unmanaged->ml_unmap.link);
    wl_list_remove(&unmanaged->ml_set_override_redirect.link);
    wl_list_remove(&unmanaged->ml_request_activate.link);
    wl_list_remove(&unmanaged->ml_request_configure.link);
    wl_list_remove(&unmanaged->ml_request_fullscreen.link);
    wl_list_remove(&unmanaged->ml_destroy.link);

    unmanaged->mp_manager->destroy_unmanaged(unmanaged);
}
#endif
