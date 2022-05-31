#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/tree/node.hh>

#include <chrono>

extern "C" {
#include <wayland-server-core.h>
}

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;

typedef struct Layer final : public Node {
    Layer(
        struct wlr_layer_surface_v1*,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        Output_ptr,
        SceneLayer
    );

    ~Layer();

    void format_uid() override;

    pid_t pid();

    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_surface_commit(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    bool mapped() const { return m_mapped; }
    void set_mapped(bool);

    Region const& region() { return m_region; }
    void set_region(Region const&);

    Server_ptr mp_server;
    Model_ptr mp_model;
    Seat_ptr mp_seat;

    Output_ptr mp_output;

	struct wlr_scene_node* mp_scene;
    SceneLayer m_scene_layer;
	struct wlr_layer_surface_v1* mp_layer_surface;

    pid_t m_pid;

    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_surface_commit;
    struct wl_listener ml_destroy;

private:
    Region m_region;
    int m_mapped;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

}* Layer_ptr;
