#ifdef XWAYLAND
#include <trace.hh>

#include <kranewl/context.hh>
#include <kranewl/model.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xwayland-view.hh>
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

XWaylandView::XWaylandView(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    XWayland_ptr xwayland
)
    : View(
          this,
          reinterpret_cast<std::uintptr_t>(wlr_xwayland_surface),
          server,
          model,
          seat,
          wlr_xwayland_surface->surface
      ),
      mp_xwayland(xwayland),
      mp_wlr_xwayland_surface(wlr_xwayland_surface),
      ml_map({ .notify = XWaylandView::handle_map }),
      ml_unmap({ .notify = XWaylandView::handle_unmap }),
      ml_commit({ .notify = XWaylandView::handle_commit }),
      ml_request_activate({ .notify = XWaylandView::handle_request_activate }),
      ml_request_configure({ .notify = XWaylandView::handle_request_configure }),
      ml_request_fullscreen({ .notify = XWaylandView::handle_request_fullscreen }),
      ml_request_minimize({ .notify = XWaylandView::handle_request_minimize }),
      ml_request_maximize({ .notify = XWaylandView::handle_request_maximize }),
      ml_request_move({ .notify = XWaylandView::handle_request_move }),
      ml_request_resize({ .notify = XWaylandView::handle_request_resize }),
      ml_set_override_redirect({ .notify = XWaylandView::handle_set_override_redirect }),
      ml_set_title({ .notify = XWaylandView::handle_set_title }),
      ml_set_class({ .notify = XWaylandView::handle_set_class }),
      ml_set_hints({ .notify = XWaylandView::handle_set_hints }),
      ml_destroy({ .notify = XWaylandView::handle_destroy })
{
    wl_signal_add(&mp_wlr_xwayland_surface->events.map, &ml_map);
    wl_signal_add(&mp_wlr_xwayland_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_activate, &ml_request_activate);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_configure, &ml_request_configure);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_fullscreen, &ml_request_fullscreen);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_minimize, &ml_request_minimize);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_maximize, &ml_request_maximize);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_move, &ml_request_move);
    wl_signal_add(&mp_wlr_xwayland_surface->events.request_resize, &ml_request_resize);
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_override_redirect, &ml_set_override_redirect);
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_title, &ml_set_title);
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_class, &ml_set_class);
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_hints, &ml_set_hints);
    wl_signal_add(&mp_wlr_xwayland_surface->events.destroy, &ml_destroy);
}

XWaylandView::~XWaylandView()
{}

void
XWaylandView::format_uid()
{
    std::stringstream uid_ss;
    uid_ss << "0x" << std::hex << uid() << std::dec;
    uid_ss << " [" << m_title;
    uid_ss << "; " << m_class;
    uid_ss << "; " << m_instance;
    uid_ss << ", " << m_pid << "]";
    uid_ss << " (XM)";
    m_uid_formatted = uid_ss.str();
}

Region
XWaylandView::constraints()
{
    TRACE();

}

pid_t
XWaylandView::pid()
{
    TRACE();
    return mp_wlr_xwayland_surface->pid;
}

bool
XWaylandView::prefers_floating()
{
    TRACE();

    struct wlr_xwayland_surface* xwayland_surface = mp_wlr_xwayland_surface;

    if (xwayland_surface->modal)
        return true;

    for (std::size_t i = 0; i < xwayland_surface->window_type_len; ++i) {
        xcb_atom_t type = xwayland_surface->window_type[i];

        if (type == mp_xwayland->m_atoms[XWayland::NET_WM_WINDOW_TYPE_DIALOG]
            || type == mp_xwayland->m_atoms[XWayland::NET_WM_WINDOW_TYPE_UTILITY]
            || type == mp_xwayland->m_atoms[XWayland::NET_WM_WINDOW_TYPE_TOOLBAR]
            || type == mp_xwayland->m_atoms[XWayland::NET_WM_WINDOW_TYPE_SPLASH])
        {
            return true;
        }
    }

    struct wlr_xwayland_surface_size_hints* size_hints = xwayland_surface->size_hints;
    if (size_hints && size_hints->min_width > 0 && size_hints->min_height > 0
        && (size_hints->max_width == size_hints->min_width
            || size_hints->max_height == size_hints->min_height))
    {
        return true;
    }

    return false;
}

void
XWaylandView::focus(Toggle toggle)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (focused())
            return;

        set_focused(true);
        activate(toggle);
        render_decoration();
        raise();
        break;
    }
    case Toggle::Off:
    {
        if (!focused())
            return;

        set_focused(false);
        activate(toggle);
        render_decoration();
        break;
    }
    case Toggle::Reverse:
    {
        focus(
            focused()
            ? Toggle::Off
            : Toggle::On
        );
        return;
    }
    default: break;
    }
}

void
XWaylandView::activate(Toggle toggle)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (activated())
            return;

        set_activated(true);

        struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(mp_seat->mp_wlr_seat);
        if (keyboard)
            wlr_seat_keyboard_notify_enter(
                mp_seat->mp_wlr_seat,
                mp_wlr_surface,
                keyboard->keycodes,
                keyboard->num_keycodes,
                &keyboard->modifiers
            );
        else
            wlr_seat_keyboard_notify_enter(
                mp_seat->mp_wlr_seat,
                mp_wlr_surface,
                nullptr,
                0,
                nullptr
            );

        wlr_xwayland_surface_activate(mp_wlr_xwayland_surface, true);
		wlr_xwayland_surface_restack(
            mp_wlr_xwayland_surface,
            nullptr,
            XCB_STACK_MODE_ABOVE
        );
        break;
    }
    case Toggle::Off:
    {
        if (!activated())
            return;

        set_activated(false);
        wlr_xwayland_surface_activate(mp_wlr_xwayland_surface, false);
        break;
    }
    case Toggle::Reverse:
    {
        activate(
            activated()
            ? Toggle::Off
            : Toggle::On
        );
        return;
    }
    default: break;
    }
}

void
XWaylandView::set_fullscreen(Toggle)
{
    TRACE();
    // TODO
}

void
XWaylandView::configure(Region const& region, Extents const& extents, bool interactive)
{
    TRACE();

    wlr_scene_node_set_position(mp_scene, region.pos.x, region.pos.y);
    wlr_scene_node_set_position(mp_scene_surface, extents.left, extents.top);
    wlr_scene_rect_set_size(m_protrusions[0], region.dim.w, extents.top);
    wlr_scene_rect_set_size(m_protrusions[1], region.dim.w, extents.bottom);
    wlr_scene_rect_set_size(m_protrusions[2], extents.left, region.dim.h - extents.top - extents.bottom);
    wlr_scene_rect_set_size(m_protrusions[3], extents.right, region.dim.h - extents.top - extents.bottom);
    wlr_scene_node_set_position(&m_protrusions[0]->node, 0, 0);
    wlr_scene_node_set_position(&m_protrusions[1]->node, 0, region.dim.h - extents.bottom);
    wlr_scene_node_set_position(&m_protrusions[2]->node, 0, extents.top);
    wlr_scene_node_set_position(&m_protrusions[3]->node, region.dim.w - extents.right, extents.top);

    wlr_xwayland_surface_configure(
        mp_wlr_xwayland_surface,
        region.pos.x,
        region.pos.y,
        region.dim.w - extents.left - extents.right,
        region.dim.h - extents.top - extents.bottom
    );

    m_resize = 0;
}

void
XWaylandView::close()
{
    TRACE();
    wlr_xwayland_surface_close(mp_wlr_xwayland_surface);
}

void
XWaylandView::close_popups()
{
    TRACE();

}

void
XWaylandView::handle_map(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_map);
    Server_ptr server = view->mp_server;
    Model_ptr model = view->mp_model;

    view->m_pid = view->pid();
    view->format_uid();

    view->set_floating(view->prefers_floating());

    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);
    view->mp_wlr_surface = xwayland_surface->surface;

    Dim preferred_dim = Dim{
        .w = xwayland_surface->width,
        .h = xwayland_surface->height,
    };

    Extents const& extents = view->free_decoration().extents();
    preferred_dim.w += extents.left + extents.right;
    preferred_dim.h += extents.top + extents.bottom;
    view->set_preferred_dim(preferred_dim);

    view->set_free_region(Region{
        .pos = Pos{
            .x = xwayland_surface->x - extents.left,
            .y = xwayland_surface->y - extents.top
        },
        .dim = preferred_dim
    });
    view->set_tile_region(view->free_region());

    view->m_app_id = view->m_class = xwayland_surface->class_
        ? xwayland_surface->class_
        : "N/a";
    view->m_instance = xwayland_surface->instance
        ? xwayland_surface->instance
        : "N/a";
    view->m_title = xwayland_surface->title
        ? xwayland_surface->title
        : "N/a";
    view->m_title_formatted = view->m_title; // TODO: format title

    view->mp_scene = &wlr_scene_tree_create(
        server->m_scene_layers[SCENE_LAYER_TILE]
    )->node;

    view->mp_wlr_surface->data = view->mp_scene_surface = wlr_scene_subsurface_tree_create(
        view->mp_scene,
        view->mp_wlr_surface
    );
    view->mp_scene_surface->data = view;

    view->relayer(
        view->floating()
            ? SCENE_LAYER_FREE
            : SCENE_LAYER_TILE
    );

    for (std::size_t i = 0; i < 4; ++i) {
        view->m_protrusions[i] = wlr_scene_rect_create(
            view->mp_scene,
            0, 0,
            view->active_decoration().colorscheme.unfocused.values
        );
        view->m_protrusions[i]->node.data = view;
        wlr_scene_rect_set_color(
            view->m_protrusions[i],
            view->active_decoration().colorscheme.unfocused.values
        );
        wlr_scene_node_lower_to_bottom(&view->m_protrusions[i]->node);
    }

    wl_signal_add(&xwayland_surface->surface->events.commit, &view->ml_commit);

    Workspace_ptr workspace = model->mp_workspace;

    view->set_mapped(true);
    view->render_decoration();
    model->register_view(view, workspace);
}

void
XWaylandView::handle_unmap(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_unmap);

    wl_list_remove(&view->ml_commit.link);

    view->activate(Toggle::Off);
    view->mp_model->unregister_view(view);

    wlr_scene_node_destroy(view->mp_scene);
    view->mp_wlr_surface = nullptr;
    view->set_managed(false);

    if (view->mp_model->mp_workspace)
        view->mp_model->apply_layout(view->mp_workspace);
}

void
XWaylandView::handle_commit(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandView::handle_request_activate(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_request_activate);
    struct wlr_xwayland_surface* xwayland_surface = view->mp_wlr_xwayland_surface;

    if (!xwayland_surface->mapped)
        return;

    view->mp_model->focus_view(view);
}

void
XWaylandView::handle_request_configure(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_request_configure);
    struct wlr_xwayland_surface_configure_event* event
        = reinterpret_cast<struct wlr_xwayland_surface_configure_event*>(data);
    struct wlr_xwayland_surface* xwayland_surface = view->mp_wlr_xwayland_surface;

    if (!xwayland_surface->mapped) {
        wlr_xwayland_surface_configure(
            xwayland_surface,
            event->x, event->y,
            event->width, event->height
        );

        return;
    }

    if (view->mp_model->is_free(view)) {
        view->set_preferred_dim(Dim{
            .w = event->width,
            .h = event->height,
        });

        view->set_free_region(Region{
            .pos = Pos{
                .x = event->x,
                .y = event->y
            },
            .dim = view->preferred_dim()
        });

        view->configure(
            view->free_region(),
            FREE_DECORATION.extents(),
            false
        );
    } else
        view->configure(
            view->active_region(),
            view->active_decoration().extents(),
            false
        );
}

void
XWaylandView::handle_request_fullscreen(struct wl_listener*, void*)
{
    TRACE();
    // TODO
}

void
XWaylandView::handle_request_minimize(struct wl_listener*, void*)
{
    TRACE();
    // TODO
}

void
XWaylandView::handle_request_maximize(struct wl_listener*, void*)
{
    TRACE();
    // TODO
}

void
XWaylandView::handle_request_move(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_request_move);
    struct wlr_xwayland_surface* xwayland_surface = view->mp_wlr_xwayland_surface;

    if (!xwayland_surface->mapped)
        return;

    if (!view->mp_model->is_free(view) || view->fullscreen())
        return;

    view->mp_model->cursor_interactive(Cursor::Mode::Move, view);
}

void
XWaylandView::handle_request_resize(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_request_resize);
    struct wlr_xwayland_surface* xwayland_surface = view->mp_wlr_xwayland_surface;

    if (!xwayland_surface->mapped)
        return;

    if (!view->mp_model->is_free(view) || view->fullscreen())
        return;

    struct wlr_xwayland_resize_event* event
        = reinterpret_cast<struct wlr_xwayland_resize_event*>(data);

    view->mp_seat->mp_cursor->initiate_cursor_interactive(
        Cursor::Mode::Resize,
        view,
        event->edges
    );
}

void
XWaylandView::handle_set_override_redirect(struct wl_listener* listener, void* data)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_set_override_redirect);
    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);

    if (xwayland_surface->mapped)
        handle_unmap(&view->ml_unmap, nullptr);

    handle_destroy(&view->ml_destroy, view);
    xwayland_surface->data = nullptr;

    XWaylandUnmanaged_ptr unmanaged = view->mp_model->create_xwayland_unmanaged(
        xwayland_surface,
        view->mp_seat,
        view->mp_xwayland
    );

    if (xwayland_surface->mapped)
        XWaylandUnmanaged::handle_map(&unmanaged->ml_map, xwayland_surface);
}

void
XWaylandView::handle_set_title(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_set_title);
    view->m_title = view->mp_wlr_xwayland_surface->title
        ? view->mp_wlr_xwayland_surface->title
        : "N/a";
    view->m_title_formatted = view->m_title; // TODO: format title
    view->format_uid();
}

void
XWaylandView::handle_set_class(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_set_class);
    view->m_class = view->mp_wlr_xwayland_surface->class_
        ? view->mp_wlr_xwayland_surface->class_
        : "N/a";
    view->format_uid();
}

void
XWaylandView::handle_set_hints(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_set_hints);
    struct wlr_xwayland_surface* xwayland_surface = view->mp_wlr_xwayland_surface;

    if (!xwayland_surface->mapped)
        return;

    const bool urgent = xwayland_surface->hints_urgency;

    if (urgent) {
        view->set_urgent(true);
        view->render_decoration();
    }
}

void
XWaylandView::handle_destroy(struct wl_listener* listener, void*)
{
    TRACE();

    XWaylandView_ptr view = wl_container_of(listener, view, ml_destroy);

    wl_list_remove(&view->ml_map.link);
    wl_list_remove(&view->ml_unmap.link);
    wl_list_remove(&view->ml_request_activate.link);
    wl_list_remove(&view->ml_request_configure.link);
    wl_list_remove(&view->ml_request_fullscreen.link);
    wl_list_remove(&view->ml_request_minimize.link);
    wl_list_remove(&view->ml_request_maximize.link);
    wl_list_remove(&view->ml_request_move.link);
    wl_list_remove(&view->ml_request_resize.link);
    wl_list_remove(&view->ml_set_override_redirect.link);
    wl_list_remove(&view->ml_set_title.link);
    wl_list_remove(&view->ml_set_class.link);
    wl_list_remove(&view->ml_set_hints.link);
    wl_list_remove(&view->ml_destroy.link);

    view->mp_wlr_xwayland_surface = nullptr;
    view->mp_model->destroy_view(view);
}


XWaylandUnmanaged::XWaylandUnmanaged(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    XWayland_ptr xwayland
)
    : Node(Type::XWaylandUnmanaged, reinterpret_cast<std::uintptr_t>(wlr_xwayland_surface)),
      mp_wlr_xwayland_surface(wlr_xwayland_surface),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(nullptr),
      mp_xwayland(xwayland),
      m_region({}),
      ml_map({ .notify = handle_map }),
      ml_unmap({ .notify = handle_unmap }),
      ml_commit({ .notify = handle_commit }),
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
    wl_signal_add(&mp_wlr_xwayland_surface->events.set_geometry, &ml_set_geometry);
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
    uid_ss << " (XU)";
    m_uid_formatted = uid_ss.str();
}

void
XWaylandUnmanaged::handle_map(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_unmap(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_commit(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_set_override_redirect(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_set_geometry(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_request_activate(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_request_configure(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_request_fullscreen(struct wl_listener*, void*)
{
    TRACE();

}

void
XWaylandUnmanaged::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}
#endif
