#include <trace.hh>

#include <kranewl/model.hh>

#include <kranewl/common.hh>
#include <kranewl/conf/config.hh>
#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/env.hh>
#include <kranewl/exec.hh>
#include <kranewl/input/cursor-bindings.hh>
#include <kranewl/input/cursor.hh>
#include <kranewl/input/key-bindings.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg-view.hh>
#include <kranewl/tree/xwayland-view.hh>
#include <kranewl/workspace.hh>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iomanip>
#include <optional>
#include <set>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_surface.h>
}
#undef static
#undef namespace
#undef class

Model::Model(Config const& config)
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
      m_unmanaged_map{},
      m_pid_map{},
      m_fullscreen_map{},
      m_sticky_views{},
      mp_focus(nullptr),
      mp_jumped_from(nullptr),
      m_key_bindings(Bindings::key_bindings),
      m_cursor_bindings(Bindings::cursor_bindings)
{
    TRACE();

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
Model::evaluate_user_env_vars(std::string const& env_path)
{
    TRACE();
    parse_and_set_env_vars(env_path);
}

void
Model::run_user_autostart(
    [[maybe_unused]] std::optional<std::string> autostart_path
)
{
    TRACE();
#ifdef NDEBUG
    if (autostart_path && file_exists(*autostart_path)) {
        spdlog::info("Executing autostart file at {}", *autostart_path);
        exec_external(*autostart_path);
    }
#endif
}

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

View_ptr
Model::focused_view() const
{
    return mp_focus;
}

Workspace_ptr
Model::workspace(Index index) const
{
    if (index < m_workspaces.size())
        return m_workspaces[index];

    return nullptr;
}

Context_ptr
Model::context(Index index) const
{
    if (index < m_contexts.size())
        return m_contexts[index];

    return nullptr;
}

Output_ptr
Model::output(Index index) const
{
    if (index < m_outputs.size())
        return m_outputs[index];

    return nullptr;
}

KeyBindings const&
Model::key_bindings() const
{
    return m_key_bindings;
}

CursorBindings const&
Model::cursor_bindings() const
{
    return m_cursor_bindings;
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
        &mp_server->m_seat,
        wlr_output,
        wlr_scene_output,
        std::forward<Region const&&>(output_region)
    );

    output_reserve_context(output);
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

    mp_output = m_outputs.active_element().value_or(nullptr);
}

void
Model::output_reserve_context(Output_ptr output)
{
    TRACE();

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
}

void
Model::focus_view(View_ptr view)
{
    TRACE();

    if (!view || !view->mp_context)
        return;

    Output_ptr output = view->mp_context->output();

    if (!output || mp_focus == view)
        return;

    if (!view->sticky() || view->mp_context != mp_context) {
        activate_workspace(view->mp_workspace);
        mp_workspace->activate_view(view);
    }

    if (mp_focus)
        mp_focus->focus(Toggle::Off);

    view->focus(Toggle::On);
    view->set_urgent(false);
    mp_focus = view;

    if (mp_workspace->layout_is_persistent() || mp_workspace->layout_is_single())
        apply_layout(mp_workspace);
}

void
Model::refocus()
{
    TRACE();

    Output_ptr output;
    if (!mp_focus || !(output = mp_focus->mp_context->output()))
        return;

    if (!mp_focus->sticky() || mp_focus->mp_context != mp_context) {
        activate_workspace(mp_focus->mp_workspace);
        mp_workspace->activate_view(mp_focus);
    }

    mp_focus->focus(Toggle::Off);
    mp_focus->focus(Toggle::On);
    mp_focus->set_urgent(false);

    if (mp_workspace->layout_is_persistent() || mp_workspace->layout_is_single())
        apply_layout(mp_workspace);
}

void
Model::focus_output(Output_ptr output)
{
    TRACE();

    if (output && output != mp_output)
        focus_view(output->context()->workspace()->active());
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

        view->unmap();
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

    spdlog::info(
        "Placing view {} at {}",
        view->uid_formatted(),
        std::to_string(view->active_region())
    );

    view->map();
    view->configure(
        view->active_region(),
        view->active_decoration().extents(),
        false
    );
}

bool
Model::view_matches_search(View_ptr view, SearchSelector const& selector) const
{
    TRACE();

    switch (selector.criterium()) {
    case SearchSelector::SelectionCriterium::OnWorkspaceBySelector:
    {
        auto const& [index,selector_] = selector.workspace_selector();

        if (index <= m_workspaces.size()) {
            Workspace_ptr workspace = m_workspaces[index];
            std::optional<View_ptr> view_ = workspace->find_view(selector_);

            return view_ && view_ == view;
        }

        return false;
    }
    case SearchSelector::SelectionCriterium::ByTitleEquals:
        return view->m_title == selector.string_value();
    case SearchSelector::SelectionCriterium::ByAppIdEquals:
        return view->m_app_id == selector.string_value();
    case SearchSelector::SelectionCriterium::ByTitleContains:
        return view->m_title.find(selector.string_value()) != std::string::npos;
    case SearchSelector::SelectionCriterium::ByAppIdContains:
        return view->m_app_id.find(selector.string_value()) != std::string::npos;
    case SearchSelector::SelectionCriterium::ForCondition:
        return selector.filter()(view);
    default: break;
    }

    return false;
}

View_ptr
Model::search_view(SearchSelector const& selector)
{
    TRACE();

    static constexpr struct LastFocusedComparer final {
        bool
        operator()(const View_ptr lhs, const View_ptr rhs) const
        {
            return lhs->last_focused() < rhs->last_focused();
        }
    } last_focused_comparer{};

    std::set<View_ptr, LastFocusedComparer> views{{}, last_focused_comparer};

    switch (selector.criterium()) {
    case SearchSelector::SelectionCriterium::OnWorkspaceBySelector:
    {
        auto const& [index,selector_] = selector.workspace_selector();

        if (index <= m_workspaces.size()) {
            Workspace_ptr workspace = m_workspaces[index];
            std::optional<View_ptr> view = workspace->find_view(selector_);

            if (view && (*view)->managed())
                views.insert(*view);
        }

        break;
    }
    default:
    {
        for (auto const&[_,view] : m_view_map)
            if (view->managed() && view_matches_search(view, selector))
                views.insert(view);

        break;
    }
    }

    return views.empty()
        ? nullptr
        : *views.rbegin();
}

void
Model::jump_view(SearchSelector const& selector)
{
    TRACE();

    View_ptr view = search_view(selector);

    if (view) {
        if (view == mp_focus) {
            if (mp_jumped_from && view != mp_jumped_from)
                view = mp_jumped_from;
        }

        if (mp_focus)
            mp_jumped_from = mp_focus;

        focus_view(view);
    }
}

void
Model::cursor_interactive(Cursor::Mode mode, View_ptr view)
{
    TRACE();

    if (is_free(view))
        mp_server->m_seat.mp_cursor->initiate_cursor_interactive(mode, view);
}

void
Model::abort_cursor_interactive()
{
    TRACE();
    mp_server->m_seat.mp_cursor->abort_cursor_interactive();
}

void
Model::sync_focus()
{
    TRACE();

    View_ptr active = mp_workspace->active();

    if (active == mp_focus)
        return;

    if (mp_focus)
        mp_focus->focus(Toggle::Off);

    if (active) {
        focus_view(active);
        mp_focus = active;
    } else if (mp_workspace->empty()) {
        mp_server->relinquish_focus();
        mp_focus = nullptr;
    }
}

void
Model::relayer_views(Workspace_ptr workspace)
{
    for (View_ptr view : *workspace) {
        if (is_free(view)) {
            if (view->scene_layer() != SceneLayer::SCENE_LAYER_FREE)
                view->relayer(SceneLayer::SCENE_LAYER_FREE);
                view->lower();
        } else {
            if (view->scene_layer() != SceneLayer::SCENE_LAYER_TILE) {
                view->relayer(SceneLayer::SCENE_LAYER_TILE);
            }
        }
    }

    if (mp_focus)
        mp_focus->raise();
}

void
Model::relayer_views(Context_ptr context)
{
    relayer_views(context->workspace());
}

void
Model::relayer_views(Output_ptr output)
{
    relayer_views(output->context());
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
                return view1->last_touched() < view2->last_touched();
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
                return view1->last_touched() < view2->last_touched();
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
Model::move_focus_to_workspace(Index index)
{
    TRACE();

    if (mp_focus)
        move_view_to_workspace(mp_focus, index);
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
    Output_ptr output_from = nullptr;

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        output_from = context_from->output();

        workspace_from->remove_view(view);
        apply_layout(workspace_from);
    }

    Context_ptr context_to = workspace_to->context();
    Output_ptr output_to = context_to->output();

    view->mp_context = context_to;
    view->mp_output = output_to;

    workspace_to->add_view(view);
    apply_layout(workspace_to);

    if (output_to && output_to != output_from)
        view->map();
    else
        view->unmap();

    sync_focus();
}

void
Model::move_focus_to_next_workspace(Direction direction)
{
    TRACE();

    if (mp_focus)
        move_view_to_next_workspace(mp_focus, direction);
}

void
Model::move_view_to_next_workspace(View_ptr view, Direction direction)
{
    TRACE();

    Workspace_ptr next_workspace = *m_workspaces.next_element(direction);
    move_view_to_workspace(view, next_workspace);
}

void
Model::move_focus_to_context(Index index)
{
    TRACE();

    if (mp_focus)
        move_view_to_context(mp_focus, index);
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
    Output_ptr output_from = nullptr;

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        output_from = context_from->output();

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

    if (output_to && output_to != output_from)
        view->map();
    else
        view->unmap();

    sync_focus();
}

void
Model::move_focus_to_next_context(Direction direction)
{
    TRACE();

    if (mp_focus)
        move_view_to_next_context(mp_focus, direction);
}

void
Model::move_view_to_next_context(View_ptr view, Direction direction)
{
    TRACE();

    Context_ptr next_context = *m_contexts.next_element(direction);
    move_view_to_context(view, next_context);
}

void
Model::move_focus_to_output(Index index)
{
    TRACE();

    if (mp_focus)
        move_view_to_output(mp_focus, index);
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
    Output_ptr output_from = nullptr;

    if (workspace_from) {
        Context_ptr context_from = workspace_from->context();
        output_from = context_from->output();

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

    if (output_to && output_to != output_from) {
        wlr_surface_send_enter(view->mp_wlr_surface, output_to->mp_wlr_output);
        view->map();
    } else
        view->unmap();

    sync_focus();
}

void
Model::move_focus_to_next_output(Direction direction)
{
    TRACE();

    if (mp_focus)
        move_view_to_next_output(mp_focus, direction);
}

void
Model::move_view_to_next_output(View_ptr view, Direction direction)
{
    TRACE();

    Output_ptr next_output = *m_outputs.next_element(direction);
    move_view_to_output(view, next_output);
}

void
Model::move_view_to_focused_output(View_ptr view)
{
    TRACE();
    move_view_to_output(view, mp_output);
}

void
Model::toggle_workspace()
{
    TRACE();

    if (mp_prev_workspace)
        activate_workspace(mp_prev_workspace);
}

void
Model::toggle_workspace_current_context()
{
    TRACE();

    Workspace_ptr prev_workspace = mp_context->prev_workspace();

    if (prev_workspace)
        activate_workspace(prev_workspace);
}

void
Model::activate_next_workspace(Direction direction)
{
    TRACE();
    activate_workspace(m_workspaces.next_index(direction));
}

void
Model::activate_next_workspace_current_context(Direction direction)
{
    TRACE();
    activate_workspace(*mp_context->workspaces().next_element(direction));
}

void
Model::activate_workspace(Index index)
{
    TRACE();

    if (index < m_workspaces.size())
        activate_workspace(workspace(index));
}

void
Model::activate_workspace_current_context(Index index)
{
    TRACE();

    if (index < mp_context->size())
        activate_workspace((*mp_context)[index]);
}

void
Model::activate_workspace(Workspace_ptr next_workspace)
{
    TRACE();

    if (next_workspace == mp_workspace)
        return;

    abort_cursor_interactive();

    Workspace_ptr prev_workspace = mp_workspace;
    mp_prev_workspace = prev_workspace;

    Context_ptr next_context = next_workspace->context();
    Context_ptr prev_context = prev_workspace->context();

    if (prev_workspace->focus_follows_cursor()) {
        View_ptr view_under_cursor
            = mp_server->m_seat.mp_cursor->view_under_cursor();

        if (view_under_cursor)
            view_under_cursor->set_last_cursor_pos(
                mp_server->m_seat.mp_cursor->cursor_pos()
            );
    }

    if (next_context == prev_context) {
        for (View_ptr view : *prev_workspace)
            if (!view->sticky())
                view->unmap();

        for (View_ptr view : m_sticky_views)
            view->mp_workspace = next_workspace;
    }

    next_context->activate_workspace(next_workspace);
    m_workspaces.activate_element(next_workspace);
    mp_workspace = next_workspace;

    apply_layout(next_workspace);
    sync_focus();

    if (mp_workspace->focus_follows_cursor()) {
        std::optional<Pos> const& last_cursor_pos
            = mp_focus ? mp_focus->last_cursor_pos() : std::nullopt;

        if (last_cursor_pos) {
            mp_server->m_seat.mp_cursor->set_cursor_pos(*last_cursor_pos);
            mp_focus->set_last_cursor_pos(std::nullopt);
        } else
            mp_output->focus_at_cursor();
    }
}

void
Model::toggle_context()
{
    TRACE();

    if (mp_prev_context)
        activate_context(mp_prev_context);
}

void
Model::activate_next_context(Direction direction)
{
    TRACE();
    activate_context(m_contexts.next_index(direction));
}

void
Model::activate_context(Index index)
{
    TRACE();

    if (index < m_contexts.size())
        activate_context(context(index));
}

void
Model::activate_context(Context_ptr next_context)
{
    TRACE();

    if (next_context == mp_context)
        return;

    abort_cursor_interactive();

    for (View_ptr view : *mp_workspace)
        view->unmap();

    Context_ptr prev_context = mp_context;
    mp_prev_context = prev_context;

    Output_ptr next_output = mp_output;
    Output_ptr prev_output = next_context->output();

    if (prev_output && next_output != prev_output)
        prev_output->set_context(prev_context);

    next_output->set_context(next_context);
    m_contexts.activate_element(next_context);
    mp_context = next_context;

    activate_workspace(next_context->workspace());
}

void
Model::toggle_output()
{
    TRACE();

    if (mp_prev_output)
        activate_output(mp_prev_output);
}

void
Model::activate_output(Index index)
{
    TRACE();

    if (index < m_outputs.size())
        activate_output(output(index));
}

void
Model::activate_output(Output_ptr next_output)
{
    TRACE();

    if (next_output == mp_output)
        return;

    abort_cursor_interactive();

    Output_ptr prev_output = mp_output;
    mp_prev_output = prev_output;
    mp_output = next_output;
    m_outputs.activate_element(next_output);

    Context_ptr next_context = next_output->context();
    Context_ptr prev_context = prev_output->context();
    mp_prev_context = prev_context;
    mp_context = next_context;
    m_contexts.activate_element(next_context);

    activate_workspace(next_context->workspace());
}

void
Model::toggle_layout()
{
    TRACE();

    mp_workspace->toggle_layout();
    relayer_views(mp_workspace);
    apply_layout(mp_workspace);
}

void
Model::set_layout(LayoutHandler::LayoutKind layout)
{
    TRACE();

    mp_workspace->set_layout(layout);
    relayer_views(mp_workspace);
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

    relayer_views(mp_workspace);
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

    Output_ptr output = workspace->context()->output();
    if (!output || workspace != output->context()->workspace())
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
    case Toggle::On:      view->tile(Toggle::Off);     break;
    case Toggle::Off:     view->tile(Toggle::On);      break;
    case Toggle::Reverse: view->tile(Toggle::Reverse); break;
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
        m_fullscreen_map[view] = view->free_region();

        break;
    }
    case Toggle::Off:
    {
        if (!view->fullscreen())
            return;

        if (!view->contained())
            view->set_free_region(m_fullscreen_map.at(view));

        view->set_fullscreen(false);
        // TODO: unset fullscreen state
        m_fullscreen_map.erase(view);

        break;
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

    Workspace_ptr workspace = view->mp_workspace;
    apply_layout(workspace);
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
        break;
    }
    case Toggle::Off:
    {
        view->set_contained(false);
        break;
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

    Workspace_ptr workspace = view->mp_workspace;
    apply_layout(workspace);
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

    Workspace_ptr workspace = view->mp_workspace;

    switch (toggle) {
    case Toggle::On:
    {
        if (view->iconified() || view->sticky())
            return;

        workspace->view_to_icon(view);
        view->unmap();
        view->set_iconified(true);
        break;
    }
    case Toggle::Off:
    {
        if (!view->iconified())
            return;

        workspace->icon_to_view(view);
        view->set_iconified(false);
        break;
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

    apply_layout(workspace);
    sync_focus();
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

    if (!is_free(view) || !view->mp_output)
        return;

    view->center();

    Placement placement = Placement {
        Placement::PlacementMethod::Free,
        view,
        view->free_decoration(),
        view->free_region()
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

void
Model::set_focus_follows_cursor(Toggle toggle, Workspace_ptr workspace)
{
    bool focus_follows_cursor;

    switch (toggle) {
    case Toggle::On:      focus_follows_cursor = true;                               break;
    case Toggle::Off:     focus_follows_cursor = false;                              break;
    case Toggle::Reverse: focus_follows_cursor = !workspace->focus_follows_cursor(); break;
    default: return;
    }

    workspace->set_focus_follows_cursor(focus_follows_cursor);
}

void
Model::set_focus_follows_cursor(Toggle toggle, Context_ptr context)
{
    bool focus_follows_cursor;

    switch (toggle) {
    case Toggle::On:      focus_follows_cursor = true;                             break;
    case Toggle::Off:     focus_follows_cursor = false;                            break;
    case Toggle::Reverse: focus_follows_cursor = !context->focus_follows_cursor(); break;
    default: return;
    }

    for (Workspace_ptr workspace : *context)
        set_focus_follows_cursor(
            focus_follows_cursor
                ? Toggle::On
                : Toggle::Off,
            workspace
        );

    context->set_focus_follows_cursor(focus_follows_cursor);
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

    m_view_map[view->uid()] = view;

    return view;
}

#ifdef XWAYLAND
XWaylandView_ptr
Model::create_xwayland_view(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Seat_ptr seat,
    XWayland_ptr xwayland
)
{
    TRACE();

    XWaylandView_ptr view = new XWaylandView(
        wlr_xwayland_surface,
        mp_server,
        this,
        seat,
        xwayland
    );

    m_view_map[view->uid()] = view;

    return view;
}

XWaylandUnmanaged_ptr
Model::create_xwayland_unmanaged(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Seat_ptr seat,
    XWayland_ptr xwayland
)
{
    TRACE();

    XWaylandUnmanaged_ptr node = new XWaylandUnmanaged(
        wlr_xwayland_surface,
        mp_server,
        this,
        seat,
        xwayland
    );

    m_unmanaged_map[node->uid()] = node;
    spdlog::info("Created unmanaged X client {}", node->uid_formatted());

    return node;
}

void
Model::destroy_unmanaged(XWaylandUnmanaged_ptr unmanaged)
{
    TRACE();

    m_unmanaged_map.erase(unmanaged->uid());
    spdlog::info("Destroyed unmanaged X client {}", unmanaged->uid_formatted());

    delete unmanaged;
}
#endif

void
Model::register_view(View_ptr view, Workspace_ptr workspace)
{
    TRACE();

    view->format_uid();
    move_view_to_workspace(view, workspace);
    spdlog::info("Registered view {}", view->uid_formatted());
    sync_focus();
}

void
Model::unregister_view(View_ptr view)
{
    TRACE();

    if (view->mp_workspace) {
        view->mp_workspace->remove_view(view);
        apply_layout(view->mp_workspace);
    }

    spdlog::info("Unregistered view {}", view->uid_formatted());
    mp_output->focus_at_cursor();
    sync_focus();
}

void
Model::destroy_view(View_ptr view)
{
    TRACE();

    m_view_map.erase(view->uid());
    spdlog::info("Destroyed view {}", view->uid_formatted());
    delete view;
}

Layer_ptr
Model::create_layer(
    struct wlr_layer_surface_v1* layer_surface,
    Output_ptr output,
    SceneLayer scene_layer
)
{
    TRACE();

    Layer_ptr layer = new Layer(
        layer_surface,
        mp_server,
        this,
        &mp_server->m_seat,
        output,
        scene_layer
    );

    return layer;
}

void
Model::register_layer(Layer_ptr layer)
{
    TRACE();

    layer->mp_output->add_layer(layer);
    spdlog::info("Registered layer {}", layer->uid_formatted());
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
