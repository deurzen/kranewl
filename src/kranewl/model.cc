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

#include <algorithm>
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

const View_ptr
Model::focused_view() const
{
    return mp_focus;
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
Model::map_view(View_ptr)
{
    TRACE();

}

void
Model::unmap_view(View_ptr)
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

    if (!view->sticky()) {
        activate_workspace(view->mp_workspace);
        mp_workspace->activate_view(view);
    }

    if (mp_focus && mp_focus != view)
        unfocus_view(mp_focus);

    view->set_urgent(false);
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

    spdlog::info("Placing view {} at {}", view->m_uid, std::to_string(view->active_region()));

    map_view(view);
    view->configure(
        view->active_region(),
        view->active_decoration().extents(),
        false
    );
}

void
Model::sync_focus()
{
    TRACE();

    View_ptr active = mp_workspace->active();

    if (active && active != mp_focus)
        active->activate(Toggle::On);
    else if (mp_workspace->empty()) {
        mp_server->relinquish_focus();
        mp_focus = nullptr;
    }
}

void
Model::cycle_focus(Direction direction)
{
    TRACE();

    if (mp_workspace->size() <= 1)
        return;

    mp_workspace->cycle(direction);
    sync_focus();
}

void
Model::drag_focus(Direction direction)
{
    TRACE();

    if (mp_workspace->size() <= 1)
        return;

    mp_workspace->drag(direction);
    sync_focus();

    apply_layout(mp_workspace);
}

void
Model::reverse_views()
{
    TRACE();

    if (mp_workspace->size() <= 1)
        return;

    mp_workspace->reverse();
    /* mp_workspace->active()->focus(true); */
    sync_focus();

    apply_layout(mp_workspace);
}

void
Model::rotate_views(Direction direction)
{
    TRACE();

    if (mp_workspace->size() <= 1)
        return;

    mp_workspace->rotate(direction);
    /* mp_workspace->active()->focus(true); */
    sync_focus();

    apply_layout(mp_workspace);
}

void
Model::shuffle_main(Direction direction)
{
    TRACE();

    std::size_t main_count
        = std::min(static_cast<std::size_t>(mp_workspace->main_count()), mp_workspace->size());

    if (main_count <= 1)
        return;

    Index focus_index = *mp_workspace->views().index();
    std::optional<Index> last_touched_index = std::nullopt;

    if (focus_index >= main_count) {
        View_ptr last_touched = *std::max_element(
            mp_workspace->begin(),
            mp_workspace->begin() + main_count,
            [](View_ptr view1, View_ptr view2) -> bool {
                return view1->m_last_touched < view2->m_last_touched;
            }
        );

        last_touched_index
            = mp_workspace->views().index_of_element(last_touched);
    }

    mp_workspace->shuffle_main(direction);
    sync_focus();

    if (last_touched_index)
        (*mp_workspace)[*last_touched_index]->touch();

    apply_layout(mp_workspace);
}

void
Model::shuffle_stack(Direction direction)
{
    TRACE();

    std::size_t main_count
        = std::min(static_cast<std::size_t>(mp_workspace->main_count()), mp_workspace->size());

    if ((mp_workspace->size() - main_count) <= 1)
        return;

    Index focus_index = *mp_workspace->views().index();
    std::optional<Index> last_touched_index = std::nullopt;

    if (focus_index < main_count) {
        View_ptr last_touched = *std::max_element(
            mp_workspace->begin() + main_count,
            mp_workspace->end(),
            [](View_ptr view1, View_ptr view2) -> bool {
                return view1->m_last_touched < view2->m_last_touched;
            }
        );

        last_touched_index
            = mp_workspace->views().index_of_element(last_touched);
    }

    mp_workspace->shuffle_stack(direction);
    sync_focus();

    if (last_touched_index)
        (*mp_workspace)[*last_touched_index]->touch();

    apply_layout(mp_workspace);
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

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        Output_ptr output_from = context_from->output();

        workspace_from->remove_view(view);
        apply_layout(workspace_from);
    }

    Context_ptr context_to = workspace_to->context();
    Output_ptr output_to = context_to->output();

    view->mp_context = context_to;
    view->mp_output = output_to;

    workspace_to->add_view(view);
    apply_layout(workspace_to);

    if (output_to)
        map_view(view);
    else
        unmap_view(view);

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

    Workspace_ptr workspace_from = view->mp_workspace;

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        Output_ptr output_from = context_from->output();

        workspace_from->remove_view(view);
        apply_layout(workspace_from);
    }

    view->mp_context = context_to;

    Workspace_ptr workspace_to = context_to->workspace();
    Output_ptr output_to = context_to->output();

    view->mp_workspace = workspace_to;
    view->mp_output = output_to;

    workspace_to->add_view(view);
    apply_layout(workspace_to);

    if (output_to)
        map_view(view);
    else
        unmap_view(view);

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

    Workspace_ptr workspace_from = view->mp_workspace;

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        Output_ptr output_from = context_from->output();

        workspace_from->remove_view(view);
        apply_layout(workspace_from);
        wlr_surface_send_leave(view->mp_wlr_surface, output_from->mp_wlr_output);
    }

    if (!output_to || !output_to->context()) {
        view->mp_output = nullptr;
        view->mp_context = nullptr;
        view->mp_workspace = nullptr;
        return;
    }

    Context_ptr context_to = output_to->context();
    Workspace_ptr workspace_to = context_to->workspace();

    view->mp_output = output_to;
    view->mp_context = context_to;
    view->mp_workspace = workspace_to;

    workspace_to->add_view(view);
    apply_layout(workspace_to);

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
    TRACE();

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
                    return view->free_region();
                else
                    return view->tile_region();
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
Model::toggle_layout_data()
{
    TRACE();

    mp_workspace->toggle_layout_data();
    apply_layout(mp_workspace);
}

void
Model::cycle_layout_data(Direction direction)
{
    TRACE();

    mp_workspace->cycle_layout_data(direction);
    apply_layout(mp_workspace);
}

void
Model::copy_data_from_prev_layout()
{
    TRACE();

    mp_workspace->copy_data_from_prev_layout();
    apply_layout(mp_workspace);
}

void
Model::change_gap_size(Util::Change<int> change)
{
    TRACE();

    mp_workspace->change_gap_size(change);
    apply_layout(mp_workspace);
}

void
Model::change_main_count(Util::Change<int> change)
{
    TRACE();

    mp_workspace->change_main_count(change);
    apply_layout(mp_workspace);
}

void
Model::change_main_factor(Util::Change<float> change)
{
    TRACE();

    mp_workspace->change_main_factor(change);
    apply_layout(mp_workspace);
}

void
Model::change_margin(Util::Change<int> change)
{
    TRACE();

    mp_workspace->change_margin(change);
    apply_layout(mp_workspace);
}

void
Model::change_margin(Edge edge, Util::Change<int> change)
{
    TRACE();

    mp_workspace->change_margin(edge, change);
    apply_layout(mp_workspace);
}

void
Model::reset_gap_size()
{
    TRACE();

    mp_workspace->reset_gap_size();
    apply_layout(mp_workspace);
}

void
Model::reset_margin()
{
    TRACE();

    mp_workspace->reset_margin();
    apply_layout(mp_workspace);
}

void
Model::reset_layout_data()
{
    TRACE();

    mp_workspace->reset_layout_data();
    apply_layout(mp_workspace);
}

void
Model::save_layout(std::size_t number) const
{
    TRACE();

    mp_workspace->save_layout(number);
}

void
Model::load_layout(std::size_t number)
{
    TRACE();

    mp_workspace->load_layout(number);
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

void
Model::kill_focus()
{
    TRACE();

    if (mp_focus)
        kill_view(mp_focus);
}

void
Model::kill_view(View_ptr view)
{
    TRACE();

    if (!view->invincible())
        view->close();
}

void
Model::set_floating_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_floating_view(toggle, mp_focus);
}

void
Model::set_floating_view(Toggle toggle, View_ptr view)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:      view->set_floating(true);              break;
    case Toggle::Off:     view->set_floating(false);             break;
    case Toggle::Reverse: view->set_floating(!view->floating()); break;
    default: return;
    }

    apply_layout(view->mp_workspace);
}

void
Model::set_fullscreen_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_fullscreen_view(toggle, mp_focus);
}

void
Model::set_fullscreen_view(Toggle toggle, View_ptr view)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (view->fullscreen())
            return;

        view->set_fullscreen(true);

        // TODO: set fullscreen state

        Workspace_ptr workspace = view->mp_workspace;
        apply_layout(workspace);

        m_fullscreen_map[view] = view->free_region();

        return;
    }
    case Toggle::Off:
    {
        if (!view->fullscreen())
            return;

        if (!view->contained())
            view->set_free_region(m_fullscreen_map.at(view));

        view->set_fullscreen(false);

        // TODO: unset fullscreen state

        Workspace_ptr workspace = view->mp_workspace;
        apply_layout(workspace);

        m_fullscreen_map.erase(view);

        return;
    }
    case Toggle::Reverse:
    {
        set_fullscreen_view(
            view->fullscreen()
            ? Toggle::Off
            : Toggle::On,
            view
        );

        return;
    }
    default: return;
    }
}

void
Model::set_sticky_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_sticky_view(toggle, mp_focus);
}

void
Model::set_sticky_view(Toggle toggle, View_ptr view)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (view->sticky())
            return;

        if (view->iconified())
            set_iconify_view(Toggle::Off, view);

        std::for_each(
            m_workspaces.begin(),
            m_workspaces.end(),
            [view](Workspace_ptr workspace) {
                if (workspace != view->mp_workspace)
                    workspace->add_view(view);
            }
        );

        // TODO: set sticky state

        Workspace_ptr workspace = view->mp_workspace;

        // TODO: view->stick();
        apply_layout(workspace);

        return;
    }
    case Toggle::Off:
    {
        if (!view->sticky())
            return;

        std::for_each(
            m_workspaces.begin(),
            m_workspaces.end(),
            [=,this](Workspace_ptr workspace) {
                if (workspace != mp_workspace) {
                    workspace->remove_view(view);
                    workspace->remove_icon(view);
                    workspace->remove_disowned(view);
                } else
                    view->mp_workspace = workspace;
            }
        );

        // TODO: unset sticky state

        // TODO: view->unstick();
        apply_layout(mp_workspace);

        return;
    }
    case Toggle::Reverse:
    {
        set_sticky_view(
            view->sticky()
            ? Toggle::Off
            : Toggle::On,
            view
        );

        return;
    }
    default: return;
    }
}

void
Model::set_contained_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_contained_view(toggle, mp_focus);
}

void
Model::set_contained_view(Toggle toggle, View_ptr view)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        view->set_contained(true);

        Workspace_ptr workspace = view->mp_workspace;

        apply_layout(workspace);
        return;
    }
    case Toggle::Off:
    {
        view->set_contained(false);

        Workspace_ptr workspace = view->mp_workspace;

        apply_layout(workspace);
        return;
    }
    case Toggle::Reverse:
    {
        set_contained_view(
            view->contained()
            ? Toggle::Off
            : Toggle::On,
            view
        );

        return;
    }
    default: return;
    }
}

void
Model::set_invincible_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_invincible_view(toggle, mp_focus);
}

void
Model::set_invincible_view(Toggle toggle, View_ptr view)
{
    TRACE();

    if (toggle == Toggle::Reverse)
        set_invincible_view(
            view->invincible()
            ? Toggle::Off
            : Toggle::On,
            view
        );
    else
        view->set_invincible(
            toggle == Toggle::On
                ? true
                : false
        );
}

void
Model::set_iconifyable_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_iconifyable_view(toggle, mp_focus);
}

void
Model::set_iconifyable_view(Toggle toggle, View_ptr view)
{
    TRACE();

    if (toggle == Toggle::Reverse)
        set_iconifyable_view(
            view->iconifyable()
            ? Toggle::Off
            : Toggle::On,
            view
        );
    else
        view->set_iconifyable(
            toggle == Toggle::On
                ? true
                : false
        );
}

void
Model::set_iconify_focus(Toggle toggle)
{
    TRACE();

    if (mp_focus)
        set_iconify_view(toggle, mp_focus);
}

void
Model::set_iconify_view(Toggle toggle, View_ptr view)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (view->iconified() || view->sticky())
            return;

        Workspace_ptr workspace = view->mp_workspace;
        workspace->view_to_icon(view);

        // TODO: set iconify state

        unmap_view(view);

        apply_layout(workspace);
        sync_focus();

        view->set_iconified(true);

        return;
    }
    case Toggle::Off:
    {
        if (!view->iconified())
            return;

        Workspace_ptr workspace = view->mp_workspace;
        workspace->icon_to_view(view);

        // TODO: unset iconify state

        view->set_iconified(false);

        apply_layout(workspace);
        sync_focus();

        return;
    }
    case Toggle::Reverse:
    {
        set_iconify_view(
            view->iconified()
            ? Toggle::Off
            : Toggle::On,
            view
        );

        return;
    }
    default: return;
    }
}

void
Model::center_focus()
{
    TRACE();

    if (mp_focus)
        center_view(mp_focus);
}

void
Model::center_view(View_ptr view)
{
    TRACE();

    if (!is_free(view))
        return;

    Region region = view->free_region();
    const Region screen_region
        = view->mp_context->output()->placeable_region();

    region.pos.x = screen_region.pos.x
        + (screen_region.dim.w - region.dim.w) / 2;

    region.pos.y = screen_region.pos.y
        + (screen_region.dim.h - region.dim.h) / 2;

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        region
    };

    place_view(placement);
}

void
Model::nudge_focus(Edge edge, Util::Change<std::size_t> change)
{
    TRACE();

    if (mp_focus)
        nudge_view(edge, change, mp_focus);
}

void
Model::nudge_view(Edge edge, Util::Change<std::size_t> change, View_ptr view)
{
    TRACE();

    if (!is_free(view))
        return;

    Region region = view->free_region();

    switch (edge) {
    case Edge::Left:
    {
        region.pos.x -= change;
        break;
    }
    case Edge::Top:
    {
        region.pos.y -= change;
        break;
    }
    case Edge::Right:
    {
        region.pos.x += change;
        break;
    }
    case Edge::Bottom:
    {
        region.pos.y += change;
        break;
    }
    default: return;
    }

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        region
    };

    place_view(placement);
}

void
Model::stretch_focus(Edge edge, Util::Change<int> change)
{
    if (mp_focus)
        stretch_view(edge, change, mp_focus);
}

void
Model::stretch_view(Edge edge, Util::Change<int> change, View_ptr view)
{
    TRACE();

    if (!is_free(view))
        return;

    Decoration decoration = view->free_decoration();
    Extents extents = Extents { 0, 0, 0, 0 };

    if (decoration.frame) {
        extents.left   += decoration.frame->extents.left;
        extents.top    += decoration.frame->extents.top;
        extents.right  += decoration.frame->extents.right;
        extents.bottom += decoration.frame->extents.bottom;
    }

    Region region = view->free_region();
    region.remove_extents(extents);

    switch (edge) {
    case Edge::Left:
    {
        if (!(change < 0 && -change >= region.dim.h)) {
            if (region.dim.w + change <= View::MIN_VIEW_DIM.w) {
                region.pos.x -= View::MIN_VIEW_DIM.w - region.dim.w;
                region.dim.w = View::MIN_VIEW_DIM.w;
            } else {
                region.pos.x -= change;
                region.dim.w += change;
            }
        }

        break;
    }
    case Edge::Top:
    {
        if (!(change < 0 && -change >= region.dim.h)) {
            if (region.dim.h + change <= View::MIN_VIEW_DIM.h) {
                region.pos.y -= View::MIN_VIEW_DIM.h - region.dim.h;
                region.dim.h = View::MIN_VIEW_DIM.h;
            } else {
                region.pos.y -= change;
                region.dim.h += change;
            }
        }

        break;
    }
    case Edge::Right:
    {
        if (!(change < 0 && -change >= region.dim.w)) {
            if (region.dim.w + change <= View::MIN_VIEW_DIM.w)
                region.dim.w = View::MIN_VIEW_DIM.w;
            else
                region.dim.w += change;
        }

        break;
    }
    case Edge::Bottom:
    {
        if (!(change < 0 && -change >= region.dim.h)) {
            if (region.dim.h + change <= View::MIN_VIEW_DIM.h)
                region.dim.h = View::MIN_VIEW_DIM.h;
            else
                region.dim.h += change;
        }

        break;
    }
    default: return;
    }

    region.apply_extents(extents);

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        region
    };

    place_view(placement);
}

void
Model::inflate_focus(Util::Change<int> change)
{
    TRACE();

    if (mp_focus)
        inflate_view(change, mp_focus);
}

void
Model::inflate_view(Util::Change<int> change, View_ptr view)
{
    TRACE();

    if (!is_free(view))
        return;

    Decoration decoration = view->free_decoration();
    Extents extents = Extents { 0, 0, 0, 0 };

    if (decoration.frame) {
        extents.left   += decoration.frame->extents.left;
        extents.top    += decoration.frame->extents.top;
        extents.right  += decoration.frame->extents.right;
        extents.bottom += decoration.frame->extents.bottom;
    }

    Region region = view->free_region();
    region.remove_extents(extents);

    double ratio = static_cast<double>(region.dim.w)
        / static_cast<double>(region.dim.w + region.dim.h);

    double width_inc = ratio * change;
    double height_inc = change - width_inc;

    int dw = std::lround(width_inc);
    int dh = std::lround(height_inc);

    if ((dw < 0 && -dw >= region.dim.w)
        || (dh < 0 && -dh >= region.dim.h)
        || (region.dim.w + dw <= View::MIN_VIEW_DIM.w)
        || (region.dim.h + dh <= View::MIN_VIEW_DIM.h))
    {
        return;
    }

    region.dim.w += dw;
    region.dim.h += dh;

    region.apply_extents(extents);

    int dx = region.dim.w - view->free_region().dim.w;
    int dy = region.dim.h - view->free_region().dim.h;

    dx = std::lround(dx / static_cast<double>(2));
    dy = std::lround(dy / static_cast<double>(2));

    region.pos.x -= dx;
    region.pos.y -= dy;

    view->set_free_region(region);

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        region
    };

    place_view(placement);
}

void
Model::snap_focus(Edge edge)
{
    TRACE();

    if (mp_focus)
        snap_view(edge, mp_focus);
}

void
Model::snap_view(Edge edge, View_ptr view)
{
    TRACE();

    if (!is_free(view))
        return;

    Region region = view->free_region();
    const Region screen_region
        = view->mp_context->output()->placeable_region();

    switch (edge) {
    case Edge::Left:
    {
        region.pos.x = screen_region.pos.x;

        break;
    }
    case Edge::Top:
    {
        region.pos.y = screen_region.pos.y;

        break;
    }
    case Edge::Right:
    {
        region.pos.x = std::max(
            0,
            (screen_region.dim.w + screen_region.pos.x) - region.dim.w
        );

        break;
    }
    case Edge::Bottom:
    {
        region.pos.y = std::max(
            0,
            (screen_region.dim.h + screen_region.pos.y) - region.dim.h
        );

        break;
    }
    }

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        region
    };

    place_view(placement);
}

void
Model::pop_deiconify()
{
    TRACE();

    std::optional<View_ptr> icon = mp_workspace->pop_icon();

    if (icon)
        set_iconify_view(Toggle::Off, *icon);
}

void
Model::deiconify_all()
{
    TRACE();

    for (std::size_t i = 0; i < mp_workspace->size(); ++i)
        pop_deiconify();
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

    m_view_map[view->m_uid] = view;

    return view;
}

#ifdef XWAYLAND
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

    m_view_map[view->m_uid] = view;

    return view;
}
#endif

void
Model::register_view(View_ptr view)
{
    TRACE();

    if (view->mp_workspace)
        view->mp_workspace->add_view(view);

    std::stringstream uid_ss;
    uid_ss << std::hex << view->m_uid;
    spdlog::info(
        "Registered view 0x{} [{}, PID {}]",
        uid_ss.str(),
        view->m_title,
        view->m_pid
    );

    sync_focus();
}

void
Model::unregister_view(View_ptr view)
{
    TRACE();

    if (view->mp_workspace)
        view->mp_workspace->remove_view(view);

    std::stringstream uid_ss;
    uid_ss << std::hex << view->m_uid;
    spdlog::info(
        "Unregistered view 0x{} [{}, PID {}]",
        uid_ss.str(),
        view->m_title,
        view->m_pid
    );

    sync_focus();
}

void
Model::destroy_view(View_ptr view)
{
    TRACE();

    m_view_map.erase(view->m_uid);

    std::stringstream uid_ss;
    uid_ss << std::hex << view->m_uid;
    spdlog::info(
        "Destroyed view 0x{} [{}, PID {}]",
        uid_ss.str(),
        view->m_title,
        view->m_pid
    );

    delete view;
}

bool
Model::is_free(View_ptr view) const
{
    return View::is_free(view)
        || ((!view->fullscreen() || view->contained())
            && (view->sticky()
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
