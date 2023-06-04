#pragma once

#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
#include <kranewl/input/bindings.hh>
#include <kranewl/layout.hh>
#include <kranewl/placement.hh>
#include <kranewl/rules.hh>
#include <kranewl/search.hh>
#include <kranewl/tree/layer.hh>
#include <kranewl/tree/view.hh>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

extern "C" {
#include <sys/types.h>
}

typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef class Server* Server_ptr;
typedef struct View* View_ptr;
typedef struct XWayland* XWayland_ptr;
typedef struct XDGView* XDGView_ptr;
#ifdef XWAYLAND
typedef struct XWaylandView* XWaylandView_ptr;
typedef struct XWaylandUnmanaged* XWaylandUnmanaged_ptr;
#endif
class Config;

class Manager final
{
public:
    Manager(Config const&);
    ~Manager();

    void evaluate_user_env_vars(std::optional<std::string> const&);
    void retrieve_user_default_rules(std::optional<std::string> const&);
    void run_user_autostart(std::optional<std::string> const&);

    void register_server(Server_ptr);
    void exit();

    View_ptr focused_view() const;
    Workspace_ptr workspace(Index) const;
    Context_ptr context(Index) const;
    Output_ptr output(Index) const;

    Cycle<Output_ptr> const& outputs() const;
    Cycle<Context_ptr> const& contexts() const;
    Cycle<Workspace_ptr> const& workspaces() const;

    KeyBindings const& key_bindings() const;
    CursorBindings const& cursor_bindings() const;

    Output_ptr create_output(struct wlr_output*, Region const&&);
    void register_output(Output_ptr);
    void unregister_output(Output_ptr);
    void output_reserve_context(Output_ptr);
    void update_outputs();

    XDGView_ptr create_xdg_shell_view(struct wlr_xdg_surface*, Seat_ptr);
#ifdef XWAYLAND
    XWaylandView_ptr create_xwayland_view(
        struct wlr_xwayland_surface*,
        Seat_ptr,
        XWayland_ptr
    );

    XWaylandUnmanaged_ptr create_xwayland_unmanaged(
        struct wlr_xwayland_surface*,
        Seat_ptr,
        XWayland_ptr
    );

    void destroy_unmanaged(XWaylandUnmanaged_ptr);
#endif
    void initialize_view(View_ptr, Workspace_ptr);
    void register_view(View_ptr, Workspace_ptr);
    void unregister_view(View_ptr);
    void destroy_view(View_ptr);

    Layer_ptr create_layer(struct wlr_layer_surface_v1*, Output_ptr, SceneLayer);
    void register_layer(Layer_ptr);

    void focus_view(View_ptr);
    void refocus();
    void place_view(Placement&);
    void relayer_view(View_ptr, SceneLayer);

    bool view_matches_search(View_ptr, SearchSelector const&) const;
    View_ptr search_view(SearchSelector const&);
    void jump_view(SearchSelector const&);

    void focus_output(Output_ptr);

    void cursor_interactive(Cursor::Mode, View_ptr);
    void abort_cursor_interactive();

    void cycle_focus(Direction);
    void cycle_focus_track(Direction);
    void drag_focus_track(Direction);

    void toggle_track();
    void activate_track(SceneLayer);
    void cycle_track(Direction);

    void sync_focus();
    void sync_indicators();

    void relayer_views(Workspace_ptr);
    void relayer_views(Context_ptr);
    void relayer_views(Output_ptr);
    void move_view_to_track(View_ptr, SceneLayer, bool = false);

    void reverse_views();
    void rotate_views(Direction);
    void shuffle_main(Direction);
    void shuffle_stack(Direction);

    void move_focus_to_workspace(Index);
    void move_view_to_workspace(View_ptr, Index);
    void move_view_to_workspace(View_ptr, Workspace_ptr);
    void move_focus_to_next_workspace(Direction);
    void move_view_to_next_workspace(View_ptr, Direction);
    void move_focus_to_context(Index);
    void move_view_to_context(View_ptr, Index);
    void move_view_to_context(View_ptr, Context_ptr);
    void move_focus_to_next_context(Direction);
    void move_view_to_next_context(View_ptr, Direction);
    void move_focus_to_output(Index);
    void move_view_to_output(View_ptr, Index);
    void move_view_to_output(View_ptr, Output_ptr);
    void move_focus_to_next_output(Direction);
    void move_view_to_next_output(View_ptr, Direction);
    void move_view_to_focused_output(View_ptr);

    void toggle_workspace();
    void toggle_workspace_current_context();
    void activate_next_workspace(Direction);
    void activate_next_workspace_current_context(Direction);
    void activate_workspace(Index);
    void activate_workspace_current_context(Index);
    void activate_workspace(Workspace_ptr);
    void toggle_context();
    void activate_next_context(Direction);
    void activate_context(Index);
    void activate_context(Context_ptr);
    void toggle_output();
    void activate_output(Index);
    void activate_output(Output_ptr);

    void toggle_layout();
    void set_layout(LayoutHandler::LayoutKind);
    void set_layout_retain_region(LayoutHandler::LayoutKind);

    void toggle_layout_data();
    void cycle_layout_data(Direction);
    void copy_data_from_prev_layout();

    void change_gap_size(Util::Change<int>);
    void change_main_count(Util::Change<int>);
    void change_main_factor(Util::Change<float>);
    void change_margin(Util::Change<int>);
    void change_margin(Edge, Util::Change<int>);
    void reset_gap_size();
    void reset_margin();
    void reset_layout_data();

    void save_layout(std::size_t) const;
    void load_layout(std::size_t);

    void apply_layout(Index);
    void apply_layout(Workspace_ptr);

    void kill_focus();
    void kill_view(View_ptr);

    void set_floating_focus(Toggle);
    void set_floating_view(Toggle, View_ptr);
    void set_fullscreen_focus(Toggle);
    void set_fullscreen_view(Toggle, View_ptr);
    void set_sticky_focus(Toggle);
    void set_sticky_view(Toggle, View_ptr);
    void set_contained_focus(Toggle);
    void set_contained_view(Toggle, View_ptr);
    void set_invincible_focus(Toggle);
    void set_invincible_view(Toggle, View_ptr);
    void set_iconifyable_focus(Toggle);
    void set_iconifyable_view(Toggle, View_ptr);
    void set_iconify_focus(Toggle);
    void set_iconify_view(Toggle, View_ptr);

    void center_focus();
    void center_view(View_ptr);
    void nudge_focus(Edge, Util::Change<std::size_t>);
    void nudge_view(Edge, Util::Change<std::size_t>, View_ptr);
    void stretch_focus(Edge, Util::Change<int>);
    void stretch_view(Edge, Util::Change<int>, View_ptr);
    void inflate_focus(Util::Change<int>);
    void inflate_view(Util::Change<int>, View_ptr);
    void snap_focus(uint32_t);
    void snap_view(View_ptr, uint32_t);

    void pop_deiconify();
    void deiconify_all();

    void set_focus_follows_cursor(Toggle, Workspace_ptr);
    void set_focus_follows_cursor(Toggle, Context_ptr);

    void spawn_external(std::string&&) const;

    Output_ptr mp_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

private:
    Server_ptr mp_server;
    Config const& m_config;

    bool m_running;

    Cycle<Output_ptr> m_outputs;
    Cycle<Context_ptr> m_contexts;
    Cycle<Workspace_ptr> m_workspaces;

    Output_ptr mp_prev_output;
    Context_ptr mp_prev_context;
    Workspace_ptr mp_prev_workspace;

    std::unordered_map<Uid, View_ptr> m_view_map;
    std::unordered_map<Uid, Node_ptr> m_unmanaged_map;
    std::unordered_map<pid_t, View_ptr> m_pid_map;
    std::unordered_map<View_ptr, Region> m_fullscreen_map;

    std::vector<View_ptr> m_sticky_views;

    View_ptr mp_focus;
    View_ptr mp_jumped_from;
    View_ptr mp_next_view;
    View_ptr mp_prev_view;

    std::vector<std::tuple<SearchSelector_ptr, Rules>> m_default_rules;

    const KeyBindings m_key_bindings;
    const CursorBindings m_cursor_bindings;

};
