#pragma once

#include <kranewl/common.hh>
#include <kranewl/context.hh>
#include <kranewl/geometry.hh>
#include <kranewl/tree/node.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
}

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Context* Context_ptr;

typedef class Output final : public Node {
public:
    Output(
        Server_ptr,
        Model_ptr,
        struct wlr_output*,
        struct wlr_scene_output*
    );
    ~Output();

    static void handle_frame(struct wl_listener*, void*);
    static void handle_commit(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);
    static void handle_present(struct wl_listener*, void*);
    static void handle_mode(struct wl_listener*, void*);

    void set_context(Context_ptr context);
    Context_ptr context() const;
    Region full_region() const;
    Region placeable_region() const;
    bool contains(Pos pos) const;
    bool contains(Region region) const;

private:
    Context_ptr mp_context;
    Region m_full_region;
    Region m_placeable_region;

public:
    Server_ptr mp_server;
    Model_ptr mp_model;

    struct wlr_output* mp_wlr_output;
    struct wlr_scene_output* mp_wlr_scene_output;

    struct wlr_output_damage* mp_damage;
    struct wlr_output_mode* mp_current_mode;
    enum wl_output_subpixel m_subpixel;

    struct wl_listener ml_frame;
    struct wl_listener ml_commit;
    struct wl_listener ml_destroy;
    struct wl_listener ml_present;
    struct wl_listener ml_mode;

    struct {
        struct wl_signal disable;
    } m_events;

}* Output_ptr;
