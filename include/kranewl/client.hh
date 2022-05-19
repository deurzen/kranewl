#pragma once

#include <kranewl/common.hh>
#include <kranewl/decoration.hh>
#include <kranewl/geometry.hh>
#include <kranewl/tree/surface.hh>

extern "C" {
#include <wlr/backend.h>
}

#include <chrono>
#include <optional>
#include <string>
#include <vector>

typedef class Server* Server_ptr;
typedef class Partition* Partition_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;

typedef struct Client* Client_ptr;
typedef struct Client final
{
    enum class OutsideState
    {
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

    Client();
    Client(
        Surface surface,
        Partition_ptr partition,
        Context_ptr context,
        Workspace_ptr workspace,
        std::optional<Pid> pid,
        std::optional<Pid> ppid
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

    struct wl_list link;
    Server_ptr server;

    SurfaceType surface_type;
    Surface surface;

    struct wlr_scene_node* scene;
    struct wlr_scene_node* scene_surface;
    struct wlr_scene_rect* border[4]; // protrusions (top, bottom, left, right)

    std::string title;

    Partition_ptr partition;
    Context_ptr context;
    Workspace_ptr workspace;

    Region free_region;
    Region tile_region;
    Region active_region;
    Region previous_region;
    Region inner_region;

    Decoration tile_decoration;
    Decoration free_decoration;
    Decoration active_decoration;

    Client_ptr parent;
    std::vector<Client_ptr> children;
    Client_ptr producer;
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

    std::optional<Pid> pid;
    std::optional<Pid> ppid;

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
    if (lhs.surface_type != rhs.surface_type)
        return false;

    switch (lhs.surface_type) {
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
            switch (client.surface_type) {
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
