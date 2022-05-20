#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>
#include <kranewl/context.hh>
#include <kranewl/util.hh>

#include <spdlog/spdlog.h>

extern "C" {
#include <wlr/backend.h>
}

typedef class Server* Server_ptr;
typedef class Context* Context_ptr;
typedef class Output final {
public:
    Output(
        Server_ptr server,
        struct wlr_output* wlr_output,
        struct wlr_scene_output* wlr_scene_output
    )
        : mp_context(nullptr),
          mp_server(server),
          mp_wlr_output(wlr_output),
          mp_wlr_scene_output(wlr_scene_output)
    {}

    void
    set_context(Context_ptr context)
    {
        if (!context)
            spdlog::error("output must contain a valid context");

        if (mp_context)
            mp_context->set_output(nullptr);

        context->set_output(this);
        mp_context = context;
    }

    Context_ptr
    context() const
    {
        return mp_context;
    }

    Region
    full_region() const
    {
        return m_full_region;
    }

    Region
    placeable_region() const
    {
        return m_placeable_region;
    }

    bool
    contains(Pos pos) const
    {
        return m_full_region.contains(pos);
    }

    bool
    contains(Region region) const
    {
        return m_full_region.contains(region);
    }

private:
    Context_ptr mp_context;
    Region m_full_region;
    Region m_placeable_region;

public:
    Server_ptr mp_server;

    struct wlr_output* mp_wlr_output;
	struct wlr_scene_output* mp_wlr_scene_output;

    struct wl_listener ml_frame;
    struct wl_listener ml_destroy;

}* Output_ptr;
