#include <trace.hh>

#include <kranewl/model.hh>

#include <kranewl/common.hh>
#include <kranewl/conf/config.hh>
#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/exec.hh>
#include <kranewl/input/mouse.hh>
#include <kranewl/input/keybindings.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg_view.hh>
#include <kranewl/tree/xwayland_view.hh>
#include <kranewl/workspace.hh>

#include <spdlog/spdlog.h>

#include <iomanip>
#include <optional>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_surface.h>
}
#undef static
#undef class
#undef namespace

Model::Model(
    Config const& config,
    [[maybe_unused]] std::optional<std::string> autostart_path
)
    : m_config{config},
      m_running{true},
      m_outputs{{}, true},
      m_contexts{{}, true},
      m_workspaces{{}, true},
      mp_output{nullptr},
      mp_fallback_output{nullptr},
      mp_context{nullptr},
      mp_workspace{nullptr},
      mp_prev_output{nullptr},
      mp_prev_context{nullptr},
      mp_prev_workspace{nullptr},
      m_view_map{},
      m_pid_map{},
      m_fullscreen_map{},
      m_sticky_views{},
      m_unmanaged_views{},
      mp_focus(nullptr),
      m_key_bindings(Bindings::key_bindings),
      m_mouse_bindings{}
{
    TRACE();

#ifdef NDEBUG
    if (autostart_path) {
        spdlog::info("Executing autostart file at {}", *autostart_path);
        exec_external(*autostart_path);
    }
#endif

    static const std::vector<std::string> context_names{
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j"
    };

    static const std::vector<std::string> workspace_names{
        "main", "web", "term", {}, {}, {}, {}, {}, {}, {}
    };

    for (std::size_t i = 0; i < context_names.size(); ++i) {
        Context_ptr context = new Context(i, context_names[i]);
        m_contexts.insert_at_back(context);

        for (std::size_t j = 0; j < workspace_names.size(); ++j) {
            Workspace_ptr workspace = new Workspace(
                workspace_names.size() * i + j,
                workspace_names[j],
                context
            );

            m_workspaces.insert_at_back(workspace);
            context->register_workspace(workspace);
        }

        context->activate_workspace(Index{0});
    }

    m_contexts.activate_at_index(0);
    m_workspaces.activate_at_index(0);

    mp_context = *m_contexts.active_element();
    mp_workspace = *m_workspaces.active_element();
}

Model::~Model()
{}

void
Model::register_server(Server_ptr server)
{
    TRACE();
    mp_server = server;
}

void
Model::exit()
{
    TRACE();

    m_running = false;
    mp_server->terminate();
}

KeyBindings const&
Model::key_bindings() const
{
    return m_key_bindings;
}

Output_ptr
Model::create_output(
    struct wlr_output* wlr_output,
    struct wlr_scene_output* wlr_scene_output,
    Region const&& output_region
)
{
    TRACE();

    Output_ptr output = new Output(
        mp_server,
        this,
        wlr_output,
        wlr_scene_output,
        std::forward<Region const&&>(output_region)
    );

    std::optional<Context_ptr> context
        = m_contexts.first_element_with_condition([](Context_ptr context) {
            return !context->output();
        });

    if (context) {
        output->set_context(*context);
        (*context)->set_output(output);

        spdlog::info("Assigned context {} to output {}",
            (*context)->index(),
            output->mp_wlr_output->name
        );
    } else
        // TODO: dynamically generate new context
        spdlog::error("Depleted allocatable contexts,"
            " output {} will not house any workspaces",
            output->mp_wlr_output->name
        );

    register_output(output);

    return output;
}

void
Model::register_output(Output_ptr output)
{
    TRACE();

    m_outputs.insert_at_back(output);
    mp_output = *m_outputs.active_element();
    mp_context = mp_output->context();
    mp_workspace = mp_context->workspace();
}

void
Model::unregister_output(Output_ptr output)
{
    TRACE();

    m_outputs.remove_element(output);
    delete output;
}

void
Model::map_view(View_ptr view)
{
    TRACE();
}

void
Model::unmap_view(View_ptr view)
{
    TRACE();
}

void
Model::iconify_view(View_ptr)
{
    TRACE();
}

void
Model::deiconify_view(View_ptr)
{
    TRACE();
}

void
Model::disown_view(View_ptr)
{
    TRACE();
}

void
Model::reclaim_view(View_ptr)
{
    TRACE();
}

void
Model::focus_view(View_ptr view)
{
    TRACE();

    Output_ptr output = view->mp_context->output();

    if (!output)
        return;

    if (!view->m_sticky) {
        activate_workspace(view->mp_workspace);
        mp_workspace->activate_view(view);
    }

    if (mp_focus && mp_focus != view)
        unfocus_view(mp_focus);

    view->m_urgent = false;
    mp_focus = view;

    if (mp_workspace->layout_is_persistent() || mp_workspace->layout_is_single())
        apply_layout(mp_workspace);
}

void
Model::unfocus_view(View_ptr view)
{
    TRACE();

}

void
Model::place_view(Placement& placement)
{
    TRACE();

    View_ptr view = placement.view;

    if (!placement.region) {
        switch (placement.method) {
        case Placement::PlacementMethod::Free:
        {
            view->set_free_decoration(placement.decoration);
            break;
        }
        case Placement::PlacementMethod::Tile:
        {
            view->set_free_decoration(FREE_DECORATION);
            view->set_tile_decoration(placement.decoration);
            break;
        }
        }

        unmap_view(view);
        return;
    }

    switch (placement.method) {
    case Placement::PlacementMethod::Free:
    {
        view->set_free_decoration(placement.decoration);
        view->set_free_region(*placement.region);
        break;
    }
    case Placement::PlacementMethod::Tile:
    {
        view->set_free_decoration(FREE_DECORATION);
        view->set_tile_decoration(placement.decoration);
        view->set_tile_region(*placement.region);
        break;
    }
    }

    spdlog::info("Placing view {} at {}", view->m_uid, std::to_string(view->m_active_region));

    map_view(view);
    view->moveresize(
        view->m_active_region,
        view->m_active_decoration.extents(),
        false
    );
}

void
Model::sync_focus()
{
    View_ptr active = mp_workspace->active();

    if (active && active != mp_focus)
        active->focus(true);
    else if (mp_workspace->empty()) {
        mp_server->relinquish_focus();
        mp_focus = nullptr;
    }
}

void
Model::move_view_to_workspace(View_ptr view, Index index)
{
    TRACE();

    if (index < m_workspaces.size())
        move_view_to_workspace(view, m_workspaces[index]);
}

void
Model::move_view_to_workspace(View_ptr view, Workspace_ptr workspace_to)
{
    TRACE();

    Workspace_ptr workspace_from = view->mp_workspace;

    if (!workspace_to || workspace_to == workspace_from)
        return;

    view->mp_workspace = workspace_to;

    Context_ptr context_from = workspace_from->context();
    Output_ptr output_from = context_from->output();

    Context_ptr context_to = workspace_to->context();
    Output_ptr output_to = context_to->output();

    view->mp_context = context_to;
    if (output_to != output_from)
        view->mp_output = output_to;

    workspace_to->add_view(view);
    workspace_from->remove_view(view);

    apply_layout(workspace_to);
    apply_layout(workspace_from);

    if (!output_to)
        unmap_view(view);
    else
        map_view(view);

    sync_focus();
}

void
Model::move_view_to_context(View_ptr view, Index index)
{
    TRACE();

    if (index < m_workspaces.size())
        move_view_to_context(view, m_contexts[index]);
}

void
Model::move_view_to_context(View_ptr view, Context_ptr context_to)
{
    TRACE();

    Context_ptr context_from = view->mp_context;

    if (!context_to || context_to == context_from)
        return;

    view->mp_context = context_to;

    Workspace_ptr workspace_from = view->mp_workspace;
    Output_ptr output_from = context_from->output();

    Workspace_ptr workspace_to = context_to->workspace();
    Output_ptr output_to = context_to->output();

    view->mp_workspace = workspace_to;
    view->mp_output = output_to;

    workspace_to->add_view(view);
    workspace_from->remove_view(view);

    apply_layout(workspace_to);
    apply_layout(workspace_from);

    if (!output_to)
        unmap_view(view);
    else
        map_view(view);

    sync_focus();
}

void
Model::move_view_to_output(View_ptr view, Index index)
{
    TRACE();

    if (index < m_outputs.size())
        move_view_to_output(view, m_outputs[index]);
}

void
Model::move_view_to_output(View_ptr view, Output_ptr output_to)
{
    TRACE();

    Output_ptr output_from = view->mp_output;

    if (!output_to || output_to == output_from)
        return;

    if (output_from) {
        Workspace_ptr workspace_from = view->mp_workspace;

        if (workspace_from) {
            workspace_from->remove_view(view);
            apply_layout(workspace_from);
        }
    }

    Context_ptr context_to = output_to->context();
    Workspace_ptr workspace_to = context_to->workspace();

    view->mp_output = output_to;
    view->mp_context = context_to;
    view->mp_workspace = workspace_to;

    workspace_to->add_view(view);
    apply_layout(workspace_to);

    if (output_from)
        wlr_surface_send_leave(view->mp_wlr_surface, output_from->mp_wlr_output);

    if (output_to) {
        wlr_surface_send_enter(view->mp_wlr_surface, output_to->mp_wlr_output);
        map_view(view);
    } else
        unmap_view(view);

    sync_focus();
}

void
Model::move_view_to_focused_output(View_ptr view)
{
    TRACE();
    move_view_to_output(view, mp_output);
}

void
Model::activate_workspace(Index)
{
    TRACE();

}

void
Model::activate_workspace(Workspace_ptr)
{
    TRACE();

}

void
Model::activate_context(Index)
{
    TRACE();

}

void
Model::activate_context(Context_ptr)
{
    TRACE();

}

void
Model::activate_output(Index)
{
    TRACE();

}

void
Model::activate_output(Output_ptr)
{
    TRACE();

}

void
Model::toggle_layout()
{
    TRACE();

    mp_workspace->toggle_layout();
    apply_layout(mp_workspace);
}

void
Model::set_layout(LayoutHandler::LayoutKind layout)
{
    TRACE();

    mp_workspace->set_layout(layout);
    apply_layout(mp_workspace);
}

void
Model::set_layout_retain_region(LayoutHandler::LayoutKind layout)
{
    Cycle<View_ptr> const& views = mp_workspace->views();
    std::vector<Region> regions;

    bool was_tiled = !mp_workspace->layout_is_free();

    if (was_tiled) {
        regions.reserve(views.size());

        std::transform(
            views.begin(),
            views.end(),
            std::back_inserter(regions),
            [=,this](View_ptr view) -> Region {
                if (is_free(view))
                    return view->m_free_region;
                else
                    return view->m_tile_region;
            }
        );
    }

    mp_workspace->set_layout(layout);

    if (was_tiled && mp_workspace->layout_is_free())
        for (std::size_t i = 0; i < views.size(); ++i)
            views[i]->set_free_region(regions[i]);

    apply_layout(mp_workspace);
}

void
Model::apply_layout(Index index)
{
    TRACE();

    if (index < m_workspaces.size())
        apply_layout(m_workspaces[index]);
}

void
Model::apply_layout(Workspace_ptr workspace)
{
    TRACE();

    Context_ptr context = workspace->context();
    Output_ptr output = context->output();

    if (!output)
        return;

    for (Placement placement : workspace->arrange(output->placeable_region()))
        place_view(placement);
}

XDGView_ptr
Model::create_xdg_shell_view(
    struct wlr_xdg_surface* wlr_xdg_surface,
    Seat_ptr seat
)
{
    TRACE();

    XDGView_ptr view = new XDGView(
        wlr_xdg_surface,
        mp_server,
        this,
        seat
    );

    register_view(view);

    return view;
}

XWaylandView_ptr
Model::create_xwayland_view(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Seat_ptr seat
)
{
    TRACE();

    XWaylandView_ptr view = new XWaylandView(
        wlr_xwayland_surface,
        mp_server,
        this,
        seat
    );

    register_view(view);

    return view;
}

void
Model::register_view(View_ptr view)
{
    TRACE();

    m_view_map[view->m_uid] = view;

    std::stringstream uid_stream;
    uid_stream << std::hex << view->m_uid;
    spdlog::info("Registered view 0x{}", uid_stream.str());
}

void
Model::unregister_view(View_ptr view)
{
    TRACE();

    std::stringstream uid_stream;
    uid_stream << std::hex << view->m_uid;

    if (view->mp_workspace)
        view->mp_workspace->remove_view(view);

    m_view_map.erase(view->m_uid);
    delete view;

    spdlog::info("Unegistered view 0x{}", uid_stream.str());
    sync_focus();
}

bool
Model::is_free(View_ptr view) const
{
    return View::is_free(view)
        || ((!view->m_fullscreen || view->m_contained)
            && (view->m_sticky
                    ? mp_workspace
                    : view->mp_workspace
               )->layout_is_free());
}

void
Model::spawn_external(std::string&& command) const
{
    TRACE();

    spdlog::info("Calling external command: {}", command);
    exec_external(command);
}
