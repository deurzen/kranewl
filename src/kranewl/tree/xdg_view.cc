#include <trace.hh>

#include <kranewl/layers.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg_view.hh>

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
#include <wlr/types/wlr_xdg_shell.h>
}
#undef static
#undef class
#undef namespace

XDGView::XDGView(
    struct wlr_xdg_surface* wlr_xdg_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat
)
    : View(
          this,
          reinterpret_cast<std::uintptr_t>(wlr_xdg_surface),
          server,
          model,
          seat,
          wlr_xdg_surface->surface,
          XDGView::handle_foreign_activate_request,
          XDGView::handle_foreign_fullscreen_request,
          XDGView::handle_foreign_close_request,
          XDGView::handle_foreign_destroy,
          XDGView::handle_surface_new_subsurface
      ),
      mp_wlr_xdg_surface(wlr_xdg_surface),
      mp_wlr_xdg_toplevel(wlr_xdg_surface->toplevel),
      ml_commit({ .notify = XDGView::handle_commit }),
      ml_request_move({ .notify = XDGView::handle_request_move }),
      ml_request_resize({ .notify = XDGView::handle_request_resize }),
      ml_request_fullscreen({ .notify = XDGView::handle_request_fullscreen }),
      ml_set_title({ .notify = XDGView::handle_set_title }),
      ml_set_app_id({ .notify = XDGView::handle_set_app_id }),
      ml_new_popup({ .notify = XDGView::handle_new_popup }),
      ml_map({ .notify = XDGView::handle_map }),
      ml_unmap({ .notify = XDGView::handle_unmap }),
      ml_destroy({ .notify = XDGView::handle_destroy })
{
    wl_signal_add(&mp_wlr_xdg_surface->events.map, &ml_map);
    wl_signal_add(&mp_wlr_xdg_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_wlr_xdg_surface->events.destroy, &ml_destroy);
}

XDGView::~XDGView()
{}

void
XDGView::focus(bool raise)
{
    TRACE();

    struct wlr_surface* focused_surface
        = mp_seat->mp_wlr_seat->keyboard_state.focused_surface;

    if (raise)
        wlr_scene_node_raise_to_top(mp_scene);

    if (focused_surface == mp_wlr_surface)
        return;

    mp_model->focus_view(this);

    for (std::size_t i = 0; i < 4; ++i)
        wlr_scene_rect_set_color(
            m_protrusions[i],
            m_active_decoration.colorscheme.focused.values
        );

    if (focused_surface && focused_surface != mp_wlr_surface) {
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

            if (focused_surface->role_data && node->data)
                for (std::size_t i = 0; i < 4; ++i)
                    wlr_scene_rect_set_color(
                        m_protrusions[i],
                        m_active_decoration.colorscheme.unfocused.values
                    );

            struct wlr_xdg_surface* wlr_xdg_surface;
            if (wlr_surface_is_xdg_surface(focused_surface)
                    && (wlr_xdg_surface = wlr_xdg_surface_from_wlr_surface(focused_surface))
                    && wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
            {
                wlr_xdg_toplevel_set_activated(wlr_xdg_surface, false);
            }
        }
    }

    wlr_idle_set_enabled(
        mp_seat->mp_idle,
        mp_seat->mp_wlr_seat,
        wl_list_empty(&mp_seat->mp_idle_inhibit_manager->inhibitors)
    );

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

    wlr_xdg_toplevel_set_activated(mp_wlr_xdg_surface, true);
}

void
XDGView::moveresize(Region const& region, Extents const& extents, bool interactive)
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

	m_resize = wlr_xdg_toplevel_set_size(
        mp_wlr_xdg_surface,
        region.dim.w - extents.left - extents.right,
        region.dim.h - extents.top - extents.bottom
    );
}

void
XDGView::handle_foreign_activate_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_fullscreen_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_close_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_surface_new_subsurface(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_commit(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_move(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_resize(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_fullscreen(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_set_title(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_set_app_id(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_new_popup(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_map(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_map);
    struct wlr_xdg_surface* wlr_xdg_surface = view->mp_wlr_xdg_surface;
    struct wlr_xdg_toplevel* wlr_xdg_toplevel = view->mp_wlr_xdg_toplevel;

    view->m_mapped = true;
    view->m_preferred_dim = Dim{
        .w = wlr_xdg_toplevel->base->current.geometry.width,
        .h = wlr_xdg_toplevel->base->current.geometry.height,
    };

    if (!view->m_preferred_dim.w && !view->m_preferred_dim.h) {
        view->m_preferred_dim.w = wlr_xdg_toplevel->base->surface->current.width;
        view->m_preferred_dim.h = wlr_xdg_toplevel->base->surface->current.height;
    }

    Extents const& extents = view->m_free_decoration.extents();
    struct wlr_box geometry;
    wlr_xdg_surface_get_geometry(wlr_xdg_surface, &geometry);
    view->m_preferred_dim.w = extents.left + extents.right + geometry.width;
    view->m_preferred_dim.h = extents.top + extents.bottom + geometry.height;

    view->set_free_region(Region{
        .pos = {0, 0},
        .dim = view->m_preferred_dim
    });

    view->set_tile_region(Region{
        .pos = {0, 0},
        .dim = view->m_preferred_dim
    });

    view->m_app_id = wlr_xdg_toplevel->app_id
        ? std::string(wlr_xdg_toplevel->app_id)
        : "N/a";
    view->m_title = wlr_xdg_toplevel->title
        ? std::string(wlr_xdg_toplevel->title)
        : "N/a";
    view->m_title_formatted = view->m_title; // TODO: format title

    struct wlr_xdg_toplevel_state state = wlr_xdg_toplevel->current;
    view->m_floating = wlr_xdg_toplevel->parent
        || (state.min_width != 0 && state.min_height != 0
                && (state.min_width == state.max_width || state.min_height == state.max_height));

    view->m_minimum_dim = Dim{
        .w = state.min_width,
        .h = state.min_height
    };

    view->map(
        wlr_xdg_toplevel->base->surface,
        wlr_xdg_toplevel->requested.fullscreen,
        wlr_xdg_toplevel->requested.fullscreen_output,
        false // TODO: determine if client has decorations
    );

    wlr_xdg_toplevel_set_tiled(
        wlr_xdg_surface,
        // TODO: determine from view decorations
        view->free_decoration_to_wlr_edges()
    );

    wl_signal_add(&wlr_xdg_toplevel->base->surface->events.commit, &view->ml_commit);
    wl_signal_add(&wlr_xdg_toplevel->base->events.new_popup, &view->ml_new_popup);
    wl_signal_add(&wlr_xdg_toplevel->events.request_fullscreen, &view->ml_request_fullscreen);
    wl_signal_add(&wlr_xdg_toplevel->events.request_move, &view->ml_request_move);
    wl_signal_add(&wlr_xdg_toplevel->events.request_resize, &view->ml_request_resize);
    wl_signal_add(&wlr_xdg_toplevel->events.set_title, &view->ml_set_title);
    wl_signal_add(&wlr_xdg_toplevel->events.set_app_id, &view->ml_set_app_id);
}

void
XDGView::handle_unmap(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_unmap);

    view->unmap();
    view->mp_model->unmap_view(view);
}

void
XDGView::handle_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_destroy);

    view->mp_model->unregister_view(view);

    wl_list_remove(&view->ml_commit.link);
    wl_list_remove(&view->ml_new_popup.link);
    wl_list_remove(&view->ml_request_fullscreen.link);
    wl_list_remove(&view->ml_request_move.link);
    wl_list_remove(&view->ml_request_resize.link);
    wl_list_remove(&view->ml_set_title.link);
    wl_list_remove(&view->ml_set_app_id.link);
}
