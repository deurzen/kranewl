#pragma once

#include <kranewl/common.hh>
#include <kranewl/context.hh>
#include <kranewl/geometry.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/tree/node.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
}

#include <array>
#include <unordered_map>
#include <vector>

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef class Layer* Layer_ptr;

typedef class Output final {
public:
    Output(
        Server_ptr,
        Manager_ptr,
        Seat_ptr,
        struct wlr_output*,
        Region const&&
    );

    ~Output();

    static void handle_frame(struct wl_listener*, void*);
    static void handle_present(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    void set_context(Context_ptr);
    Context_ptr context() const;
    Workspace_ptr workspace() const;

    bool enabled() const;
    Region full_region() const;
    Region placeable_region() const;
    void set_full_region(Region const&);
    void set_placeable_region(Region const&);

    void place_at_center(Region&) const;

    bool contains(Pos) const;
    bool contains(Region) const;

    void focus_at_cursor();

    void add_layer(Layer_ptr);
    void remove_layer(Layer_ptr);
    void relayer_layer(Layer_ptr, SceneLayer, SceneLayer);

    void migrate_layers_to_output(Output_ptr);

    void activate() const;
    void deactivate() const;

    void arrange_layers();

private:
    Context_ptr mp_context;
    Region m_full_region;
    Region m_placeable_region;

    bool m_cursor_focus_on_present;
    bool m_leased;

    std::unordered_map<SceneLayer, std::vector<Layer_ptr>> m_layer_map;
    std::array<struct wlr_scene_tree*, 4> m_layer_tree;

	struct wlr_scene_tree* mp_layer_popup_tree;
	struct wlr_scene_tree* mp_osd_tree;
	struct wlr_scene_tree* mp_session_lock_tree;

	struct wlr_scene_buffer* mp_workspace_osd;

public:
    Server_ptr mp_server;
    Manager_ptr mp_manager;
    Seat_ptr mp_seat;

    std::vector<struct wlr_output_mode*> m_modes;
    struct wlr_output_mode* mp_current_mode;

    bool m_dirty;

    struct wlr_output* mp_wlr_output = nullptr;

    struct wl_listener ml_frame;
    struct wl_listener ml_present;
    struct wl_listener ml_destroy;

    struct {
        struct wl_signal disable;
    } m_events;

}* Output_ptr;
