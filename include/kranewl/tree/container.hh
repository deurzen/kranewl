#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>
#include <kranewl/decoration.hh>
#include <kranewl/tree/node.hh>

#include <vector>
#include <chrono>

typedef struct View* View_ptr;

typedef struct Container final : public Node {
    enum class FullscreenMode {
        None,
        Workspace,
        Context,
    };

    enum class BorderType {
        None,
        Pixel,
        Normal,
        Csd,
    };

    struct State {
        double x, y;
        double w, h;

        FullscreenMode fullscreen_mode;
        bool focused;

        Workspace_ptr workspace;
        Container_ptr parent;
        std::vector<Container_ptr> children;
        Container_ptr focused_inactive_child;

        BorderType border;
        int border_thickness;
        bool border_top;
        bool border_bottom;
        bool border_left;
        bool border_right;

        double content_x, content_y;
        double content_width, content_height;
    };

    Container();
    ~Container();

    View_ptr m_view;

    State m_state;
    State m_pending_state;

    DRegion m_free_region;
    DRegion m_tile_region;
    DRegion m_active_region;
    DRegion m_previous_region;
    DRegion m_inner_region;

    Decoration m_tile_decoration;
    Decoration m_free_decoration;
    Decoration m_active_decoration;

    bool m_mapped;
    bool m_managed;
    bool m_urgent;
    bool m_floating;
    bool m_fullscreen;
    bool m_scratchpad;
    bool m_contained;
    bool m_invincible;
    bool m_sticky;
    bool m_iconifyable;
    bool m_iconified;
    bool m_disowned;

    std::string m_title;
    std::string m_title_formatted;

    double m_saved_x, m_saved_y;
    double m_saved_w, m_saved_h;

    float m_alpha;
    uint32_t m_resize;

    std::vector<Output_ptr> m_outputs;

    std::chrono::time_point<std::chrono::steady_clock> m_last_focused;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

    struct {
        struct wl_signal destroy;
    } m_events;

}* Container_ptr;
