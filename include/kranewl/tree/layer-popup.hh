#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/tree/node.hh>

extern "C" {
#include <wayland-server-core.h>
}

#include <vector>

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef struct LayerPopup* LayerPopup_ptr;

typedef struct LayerPopup final {
    LayerPopup(
        struct wlr_xdg_popup*,
        Layer_ptr,
        Server_ptr,
        Manager_ptr,
        Seat_ptr
    );

    LayerPopup(
        struct wlr_xdg_popup*,
        LayerPopup_ptr,
        Layer_ptr,
        Server_ptr,
        Manager_ptr,
        Seat_ptr
    );

    ~LayerPopup();

    Region const& region() { return m_region; }
    void set_region(Region const&);

    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_surface_commit(struct wl_listener*, void*);
    static void handle_new_popup(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    Server_ptr mp_server;
    Manager_ptr mp_manager;
    Seat_ptr mp_seat;

	Layer_ptr mp_root;
	LayerPopup_ptr mp_parent;

	struct wlr_scene_tree* mp_scene;
    SceneLayer m_scene_layer;
    struct wlr_xdg_popup* mp_wlr_popup;

    std::vector<LayerPopup_ptr> m_popups;

    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_surface_commit;
    struct wl_listener ml_new_popup;
    struct wl_listener ml_destroy;

private:
    Region m_region;

}* LayerPopup_ptr;
