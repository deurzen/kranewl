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
        return (client->floating && (!client->fullscreen || client->contained))
            || !client->managed
            || client->disowned;
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

    Server_ptr p_server;

    Uid uid;
    Surface surface;

    struct wlr_scene_node* p_scene;
    struct wlr_scene_node* p_scene_surface;
    struct wlr_scene_rect* protrusions[4]; // top, bottom, left, right

    std::string title;

    Output_ptr p_output;
    Context_ptr p_context;
    Workspace_ptr p_workspace;

    Region free_region;
    Region tile_region;
    Region active_region;
    Region previous_region;
    Region inner_region;

    Decoration tile_decoration;
    Decoration free_decoration;
    Decoration active_decoration;

    Client_ptr p_parent;
    std::vector<Client_ptr> children;
    Client_ptr p_producer;
    std::vector<Client_ptr> consumers;

    bool focused;
    bool mapped;
    bool managed;
    bool urgent;
    bool floating;
    bool fullscreen;
    bool contained;
    bool invincible;
    bool sticky;
    bool iconifyable;
    bool iconified;
    bool disowned;
    bool producing;
    bool attaching;

    std::chrono::time_point<std::chrono::steady_clock> last_focused;
    std::chrono::time_point<std::chrono::steady_clock> managed_since;

    struct wl_listener l_commit;
    struct wl_listener l_map;
    struct wl_listener l_unmap;
    struct wl_listener l_destroy;
    struct wl_listener l_set_title;
    struct wl_listener l_fullscreen;
    struct wl_listener l_request_move;
    struct wl_listener l_request_resize;
#ifdef XWAYLAND
    struct wl_listener l_request_activate;
    struct wl_listener l_request_configure;
    struct wl_listener l_set_hints;
#else
    struct wl_listener l_new_xdg_popup;
#endif

private:
    OutsideState m_outside_state;

    void set_inner_region(Region&) noexcept;
    void set_active_region(Region&) noexcept;

}* Client_ptr;

inline bool
operator==(Client const& lhs, Client const& rhs)
{
    if (lhs.surface.type != rhs.surface.type)
        return false;

    switch (lhs.surface.type) {
    case SurfaceType::XDGShell: // fallthrough
    case SurfaceType::LayerShell: return lhs.surface.xdg == rhs.surface.xdg;
    case SurfaceType::X11Managed: // fallthrough
    case SurfaceType::X11Unmanaged: return lhs.surface.xwayland == rhs.surface.xwayland;
    }
}

namespace std
{
    template <>
    struct hash<Client> {

        std::size_t
        operator()(Client const& client) const
        {
            switch (client.surface.type) {
            case SurfaceType::XDGShell: // fallthrough
            case SurfaceType::LayerShell:
                return std::hash<wlr_xdg_surface*>{}(client.surface.xdg);
            case SurfaceType::X11Managed: // fallthrough
            case SurfaceType::X11Unmanaged:
                return std::hash<wlr_xwayland_surface*>{}(client.surface.xwayland);
            }
        }

    };
}
