#pragma once

#include <kranewl/common.hh>
#include <kranewl/decoration.hh>
#include <kranewl/geometry.hh>
#include <kranewl/tree/surface.hh>

extern "C" {
#include <wlr/backend.h>
}

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

typedef class Server* Server_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct Client* Client_ptr;
typedef struct Client final {
    static constexpr Dim MIN_CLIENT_DIM = Dim{25, 10};
    static constexpr Dim PREFERRED_INIT_CLIENT_DIM = Dim{480, 260};

    enum class OutsideState {
        Focused,
        FocusedDisowned,
        FocusedSticky,
        Unfocused,
        UnfocusedDisowned,
        UnfocusedSticky,
        Urgent
    };

    static bool
    is_free(Client_ptr client)
    {
        return (client->m_floating && (!client->m_fullscreen || client->m_contained))
            || !client->m_managed
            || client->m_disowned;
    }

    Client(
        Server_ptr server,
        Surface surface,
        Output_ptr output,
        Context_ptr context,
        Workspace_ptr workspace
    );

    ~Client();

    Client(Client const&) = default;
    Client& operator=(Client const&) = default;

    OutsideState get_outside_state() const noexcept;

    struct wlr_surface* get_surface() noexcept;

    void focus() noexcept;
    void unfocus() noexcept;

    void stick() noexcept;
    void unstick() noexcept;

    void disown() noexcept;
    void reclaim() noexcept;

    void set_urgent() noexcept;
    void unset_urgent() noexcept;

    void expect_unmap() noexcept;
    bool consume_unmap_if_expecting() noexcept;

    void set_tile_region(Region&) noexcept;
    void set_free_region(Region&) noexcept;

    void set_tile_decoration(Decoration const&) noexcept;
    void set_free_decoration(Decoration const&) noexcept;

    Server_ptr mp_server;

    Uid m_uid;
    Surface m_surface;

    struct wlr_scene_node* mp_scene;
    struct wlr_scene_node* mp_scene_surface;
    struct wlr_scene_rect* m_protrusions[4]; // top, bottom, left, right

    std::string m_title;

    Output_ptr mp_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

    Region m_free_region;
    Region m_tile_region;
    Region m_active_region;
    Region m_previous_region;
    Region m_inner_region;

    Decoration m_tile_decoration;
    Decoration m_free_decoration;
    Decoration m_active_decoration;

    bool m_focused;
    bool m_mapped;
    bool m_managed;
    bool m_urgent;
    bool m_floating;
    bool m_fullscreen;
    bool m_contained;
    bool m_invincible;
    bool m_sticky;
    bool m_iconifyable;
    bool m_iconified;
    bool m_disowned;
    bool m_producing;
    bool m_attaching;

    std::chrono::time_point<std::chrono::steady_clock> m_last_focused;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

    struct wl_listener ml_commit;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;
    struct wl_listener ml_set_title;
    struct wl_listener ml_fullscreen;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
#ifdef XWAYLAND
    struct wl_listener ml_request_activate;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_set_hints;
#else
    struct wl_listener ml_new_xdg_popup;
#endif

private:
    OutsideState m_outside_state;

    void set_inner_region(Region&) noexcept;
    void set_active_region(Region&) noexcept;

}* Client_ptr;

inline bool
operator==(Client const& lhs, Client const& rhs)
{
    if (lhs.m_surface.type != rhs.m_surface.type)
        return false;

    switch (lhs.m_surface.type) {
    case SurfaceType::XDGShell: // fallthrough
    case SurfaceType::LayerShell: return lhs.m_surface.xdg == rhs.m_surface.xdg;
    case SurfaceType::X11Managed: // fallthrough
    case SurfaceType::X11Unmanaged: return lhs.m_surface.xwayland == rhs.m_surface.xwayland;
    }
}

namespace std
{
    template <>
    struct hash<Client> {

        std::size_t
        operator()(Client const& client) const
        {
            switch (client.m_surface.type) {
            case SurfaceType::XDGShell: // fallthrough
            case SurfaceType::LayerShell:
                return std::hash<wlr_xdg_surface*>{}(client.m_surface.xdg);
            case SurfaceType::X11Managed: // fallthrough
            case SurfaceType::X11Unmanaged:
                return std::hash<wlr_xwayland_surface*>{}(client.m_surface.xwayland);
            }
        }

    };
}
