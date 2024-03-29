#include <trace.hh>

#include <kranewl/context.hh>
#include <kranewl/model.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg-view.hh>
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
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
}
#undef static
#undef namespace
#undef class

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
          wlr_xdg_surface->toplevel->base->surface
      ),
      mp_wlr_xdg_surface(wlr_xdg_surface),
      mp_wlr_xdg_toplevel(wlr_xdg_surface->toplevel),
      mp_decoration(nullptr),
      ml_map({ .notify = XDGView::handle_map }),
      ml_unmap({ .notify = XDGView::handle_unmap }),
      ml_destroy({ .notify = XDGView::handle_destroy }),
      ml_commit({ .notify = XDGView::handle_commit }),
      ml_new_popup({ .notify = XDGView::handle_new_popup }),
      ml_set_title({ .notify = XDGView::handle_set_title }),
      ml_set_app_id({ .notify = XDGView::handle_set_app_id }),
      ml_request_move({ .notify = XDGView::handle_request_move }),
      ml_request_resize({ .notify = XDGView::handle_request_resize }),
      ml_request_maximize({ .notify = XDGView::handle_request_maximize }),
      ml_request_fullscreen({ .notify = XDGView::handle_request_fullscreen })
{
    wl_signal_add(&mp_wlr_xdg_surface->events.map, &ml_map);
    wl_signal_add(&mp_wlr_xdg_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_wlr_xdg_surface->events.destroy, &ml_destroy);
}

XDGView::~XDGView()
{}

Region
XDGView::constraints()
{
    TRACE();

}

pid_t
XDGView::retrieve_pid()
{
    TRACE();

    pid_t pid;
    struct wl_client* client
        = wl_resource_get_client(mp_wlr_surface->resource);

    wl_client_get_credentials(client, &pid, nullptr, nullptr);
    return pid;
}

bool
XDGView::prefers_floating()
{
    TRACE();

    struct wlr_xdg_toplevel_state *state = &mp_wlr_xdg_toplevel->current;
	return mp_wlr_xdg_toplevel->parent || (state->min_width != 0 && state->min_height != 0
		&& (state->min_width == state->max_width || state->min_height == state->max_height));
}

void
XDGView::focus(Toggle toggle)
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

        wlr_idle_set_enabled(
            mp_server->mp_seat->mp_idle,
            mp_server->mp_seat->mp_wlr_seat,
            wl_list_empty(&mp_server->mp_seat->mp_idle_inhibit_manager->inhibitors)
        );

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
XDGView::activate(Toggle toggle)
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

        wlr_xdg_toplevel_set_activated(mp_wlr_xdg_surface, true);
        break;
    }
    case Toggle::Off:
    {
        if (!activated())
            return;

        set_activated(false);
        wlr_xdg_toplevel_set_activated(mp_wlr_xdg_surface, false);
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
XDGView::effectuate_fullscreen(bool fullscreen)
{
    TRACE();

    if (View::fullscreen() != fullscreen) {
        set_fullscreen(fullscreen);
        wlr_xdg_toplevel_set_fullscreen(mp_wlr_xdg_surface, fullscreen);
    }
}

void
XDGView::configure(Region const& region, Extents const& extents, bool interactive)
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
    wlr_scene_rect_set_size(m_next_indicator[0], CYCLE_INDICATOR_SIZE, extents.top);
    wlr_scene_rect_set_size(m_next_indicator[1], extents.left, CYCLE_INDICATOR_SIZE);
    wlr_scene_rect_set_size(m_prev_indicator[0], CYCLE_INDICATOR_SIZE, extents.top);
    wlr_scene_rect_set_size(m_prev_indicator[1], extents.right, CYCLE_INDICATOR_SIZE);
    wlr_scene_node_set_position(&m_next_indicator[0]->node, 0, 0);
    wlr_scene_node_set_position(&m_next_indicator[1]->node, 0, 0);
    wlr_scene_node_set_position(&m_prev_indicator[0]->node, region.dim.w - CYCLE_INDICATOR_SIZE, 0);
    wlr_scene_node_set_position(&m_prev_indicator[1]->node, region.dim.w - extents.right, 0);

	m_resize = wlr_xdg_toplevel_set_size(
        mp_wlr_xdg_surface,
        region.dim.w - extents.left - extents.right,
        region.dim.h - extents.top - extents.bottom
    );
}

void
XDGView::close()
{
    TRACE();
    wlr_xdg_toplevel_send_close(mp_wlr_xdg_surface);
}

void
XDGView::close_popups()
{
    TRACE();

}

void
XDGView::handle_commit(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_commit);

	if (view->m_resize && view->m_resize <= view->mp_wlr_xdg_surface->current.configure_serial)
		view->m_resize = 0;
}

void
XDGView::handle_request_move(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_request_move);
    struct wlr_event_xdg_toplevel_move* event
        = reinterpret_cast<struct wlr_event_xdg_toplevel_move*>(data);

    view->mp_seat->mp_cursor->initiate_cursor_interactive(Cursor::Mode::Move, view);
}

void
XDGView::handle_request_resize(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_request_resize);
    struct wlr_xdg_toplevel_resize_event* event
        = reinterpret_cast<struct wlr_xdg_toplevel_resize_event*>(data);

    view->mp_seat->mp_cursor->initiate_cursor_interactive(Cursor::Mode::Resize, view, event->edges);
}

void
XDGView::handle_request_fullscreen(struct wl_listener* listener, void*)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_request_fullscreen);
    struct wlr_xdg_toplevel* xdg_toplevel = view->mp_wlr_xdg_toplevel;

    view->mp_model->set_fullscreen_view(
        xdg_toplevel->requested.fullscreen
            ? Toggle::On
            : Toggle::Off,
        view
    );

    wlr_xdg_surface_schedule_configure(view->mp_wlr_xdg_surface);
}

void
XDGView::handle_request_maximize(struct wl_listener* listener, void*)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_request_maximize);
    wlr_xdg_surface_schedule_configure(view->mp_wlr_xdg_surface);
}

void
XDGView::handle_set_title(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_set_title);
    view->set_title(view->mp_wlr_xdg_toplevel->title
        ? view->mp_wlr_xdg_toplevel->title : "");
    view->set_title_formatted(view->title());
    view->format_uid();
}

void
XDGView::handle_set_app_id(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_set_app_id);
    view->set_app_id(view->mp_wlr_xdg_toplevel->app_id);
    view->format_uid();
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
    Server_ptr server = view->mp_server;
    Model_ptr model = view->mp_model;

    view->set_pid(view->retrieve_pid());
    view->format_uid();

    view->set_floating(view->prefers_floating());

    struct wlr_xdg_surface* wlr_xdg_surface = view->mp_wlr_xdg_surface;
    struct wlr_xdg_toplevel* wlr_xdg_toplevel = view->mp_wlr_xdg_toplevel;

    Dim preferred_dim = Dim{
        .w = wlr_xdg_toplevel->base->current.geometry.width,
        .h = wlr_xdg_toplevel->base->current.geometry.height,
    };

    if (!preferred_dim.w && !preferred_dim.h) {
        preferred_dim.w = wlr_xdg_toplevel->base->surface->current.width;
        preferred_dim.h = wlr_xdg_toplevel->base->surface->current.height;
    }

    Extents const& extents = view->free_decoration().extents();
    preferred_dim.w += extents.left + extents.right;
    preferred_dim.h += extents.top + extents.bottom;
    view->set_preferred_dim(preferred_dim);

    struct wlr_xdg_toplevel_state* state = &wlr_xdg_toplevel->current;
    view->set_minimum_dim(Dim{
        .w = state->min_width,
        .h = state->min_height
    });

    view->set_app_id(wlr_xdg_toplevel->app_id
        ? wlr_xdg_toplevel->app_id : "N/a");
    view->set_title(wlr_xdg_toplevel->title
        ? wlr_xdg_toplevel->title : "N/a");
    view->set_title_formatted(view->title()); // TODO: format title
    view->set_handle(view->title() + " " + view->app_id());

    wlr_xdg_toplevel_set_tiled(
        wlr_xdg_surface,
        view->free_decoration_to_wlr_edges()
    );

    view->mp_scene = &wlr_scene_tree_create(
        server->m_scene_layers[SCENE_LAYER_TILE]
    )->node;
    view->mp_wlr_surface->data = view->mp_scene_surface = wlr_scene_xdg_surface_create(
        view->mp_scene,
        view->mp_wlr_xdg_surface
    );
    view->mp_scene_surface->data = view;

    for (std::size_t i = 0; i < 2; ++i) {
        view->m_next_indicator[i] = wlr_scene_rect_create(
            view->mp_scene,
            0, 0,
            view->active_decoration().colorscheme.nextfocus.values
        );
        view->m_next_indicator[i]->node.data = view;
        wlr_scene_rect_set_color(
            view->m_next_indicator[i],
            view->active_decoration().colorscheme.nextfocus.values
        );
        wlr_scene_node_lower_to_bottom(&view->m_next_indicator[i]->node);
    }

    for (std::size_t i = 0; i < 2; ++i) {
        view->m_prev_indicator[i] = wlr_scene_rect_create(
            view->mp_scene,
            0, 0,
            view->active_decoration().colorscheme.prevfocus.values
        );
        view->m_prev_indicator[i]->node.data = view;
        wlr_scene_rect_set_color(
            view->m_prev_indicator[i],
            view->active_decoration().colorscheme.prevfocus.values
        );
        wlr_scene_node_lower_to_bottom(&view->m_prev_indicator[i]->node);
    }

    view->unindicate_as_next();
    view->unindicate_as_prev();

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

    wl_signal_add(&wlr_xdg_toplevel->base->surface->events.commit, &view->ml_commit);
    wl_signal_add(&wlr_xdg_toplevel->base->events.new_popup, &view->ml_new_popup);
    wl_signal_add(&wlr_xdg_toplevel->events.request_move, &view->ml_request_move);
    wl_signal_add(&wlr_xdg_toplevel->events.request_resize, &view->ml_request_resize);
    wl_signal_add(&wlr_xdg_toplevel->events.request_maximize, &view->ml_request_maximize);
    wl_signal_add(&wlr_xdg_toplevel->events.request_fullscreen, &view->ml_request_fullscreen);
    wl_signal_add(&wlr_xdg_toplevel->events.set_title, &view->ml_set_title);
    wl_signal_add(&wlr_xdg_toplevel->events.set_app_id, &view->ml_set_app_id);

    Workspace_ptr workspace;

    if (wlr_xdg_toplevel->requested.fullscreen
        && wlr_xdg_toplevel->requested.fullscreen_output->data)
    {
        Output_ptr output = reinterpret_cast<Output_ptr>(
            wlr_xdg_toplevel->requested.fullscreen_output->data
        );

        Context_ptr context = output->context();
        workspace = context->workspace();
    } else
        workspace = model->mp_workspace;

    Region region = Region{
        .pos = Pos{0, 0},
        .dim = preferred_dim
    };

    Output_ptr output = workspace->output();
    if (output)
        output->place_at_center(region);

    view->set_free_region(region);
    view->set_tile_region(region);

    view->set_mapped(true);
    view->render_decoration();
    model->register_view(view, workspace);
}

void
XDGView::handle_unmap(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_unmap);

    wl_list_remove(&view->ml_commit.link);
    wl_list_remove(&view->ml_new_popup.link);
    wl_list_remove(&view->ml_request_fullscreen.link);
    wl_list_remove(&view->ml_request_maximize.link);
    wl_list_remove(&view->ml_request_move.link);
    wl_list_remove(&view->ml_request_resize.link);
    wl_list_remove(&view->ml_set_title.link);
    wl_list_remove(&view->ml_set_app_id.link);

    view->activate(Toggle::Off);
    view->mp_model->unregister_view(view);

    wlr_scene_node_destroy(view->mp_scene);
	view->mp_wlr_surface = nullptr;
    view->set_managed(false);

    if (view->mp_model->mp_workspace)
        view->mp_model->apply_layout(view->mp_workspace);
}

void
XDGView::handle_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_destroy);

    wl_list_remove(&view->ml_map.link);
    wl_list_remove(&view->ml_unmap.link);
    wl_list_remove(&view->ml_destroy.link);
	wl_list_remove(&view->m_events.unmap.listener_list);

    if (view->mp_decoration)
        view->mp_decoration->mp_view = nullptr;

	view->mp_wlr_xdg_toplevel = nullptr;
	view->mp_wlr_xdg_surface = nullptr;
    view->mp_model->destroy_view(view);
}
