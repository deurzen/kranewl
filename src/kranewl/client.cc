#include <kranewl/client.hh>

Client::Client(
    Surface surface,
    Partition_ptr partition,
    Context_ptr context,
    Workspace_ptr workspace,
    std::optional<Pid> pid,
    std::optional<Pid> ppid
)
    : surface(surface),
      partition(partition),
      context(context),
      workspace(workspace),
      free_region({}),
      tile_region({}),
      active_region({}),
      previous_region({}),
      inner_region({}),
      tile_decoration({}),
      free_decoration({}),
      active_decoration({}),
      parent(nullptr),
      children({}),
      producer(nullptr),
      consumers({}),
      focused(false),
      mapped(false),
      managed(true),
      urgent(false),
      floating(false),
      fullscreen(false),
      contained(false),
      invincible(false),
      sticky(false),
      iconifyable(true),
      iconified(false),
      disowned(false),
      producing(true),
      attaching(false),
      pid(pid),
      ppid(ppid),
      last_focused(std::chrono::steady_clock::now()),
      managed_since(std::chrono::steady_clock::now()),
      m_outside_state(OutsideState::Unfocused)
{}

Client::~Client()
{}

Client::OutsideState
Client::get_outside_state() const noexcept
{
    if (urgent)
        return Client::OutsideState::Urgent;

    return m_outside_state;
}

void
Client::focus() noexcept
{
    focused = true;
    last_focused = std::chrono::steady_clock::now();

    switch (m_outside_state) {
    case OutsideState::Unfocused:         m_outside_state = OutsideState::Focused;         return;
    case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::FocusedDisowned; return;
    case OutsideState::UnfocusedSticky:   m_outside_state = OutsideState::FocusedSticky;   return;
    default: return;
    }
}

void
Client::unfocus() noexcept
{
    focused = false;

    switch (m_outside_state) {
    case OutsideState::Focused:         m_outside_state = OutsideState::Unfocused;         return;
    case OutsideState::FocusedDisowned: m_outside_state = OutsideState::UnfocusedDisowned; return;
    case OutsideState::FocusedSticky:   m_outside_state = OutsideState::UnfocusedSticky;   return;
    default: return;
    }
}

void
Client::stick() noexcept
{
    sticky = true;

    switch (m_outside_state) {
    case OutsideState::Focused:   m_outside_state = OutsideState::FocusedSticky;   return;
    case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedSticky; return;
    default: return;
    }
}

void
Client::unstick() noexcept
{
    sticky = false;

    switch (m_outside_state) {
    case OutsideState::FocusedSticky:   m_outside_state = OutsideState::Focused;   return;
    case OutsideState::UnfocusedSticky: m_outside_state = OutsideState::Unfocused; return;
    default: return;
    }
}

void
Client::disown() noexcept
{
    disowned = true;

    switch (m_outside_state) {
    case OutsideState::Focused:   m_outside_state = OutsideState::FocusedDisowned;   return;
    case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedDisowned; return;
    default: return;
    }
}

void
Client::reclaim() noexcept
{
    disowned = false;

    switch (m_outside_state) {
    case OutsideState::FocusedDisowned:   m_outside_state = OutsideState::Focused;   return;
    case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::Unfocused; return;
    default: return;
    }
}

void
Client::set_tile_region(Region& region) noexcept
{
    tile_region = region;
    set_active_region(region);
}

void
Client::set_free_region(Region& region) noexcept
{
    free_region = region;
    set_active_region(region);
}

void
Client::set_active_region(Region& region) noexcept
{
    previous_region = active_region;
    set_inner_region(region);
    active_region = region;
}

void
Client::set_tile_decoration(Decoration const& decoration) noexcept
{
    tile_decoration = decoration;
    active_decoration = decoration;
}

void
Client::set_free_decoration(Decoration const& decoration) noexcept
{
    free_decoration = decoration;
    active_decoration = decoration;
}

void
Client::set_inner_region(Region& region) noexcept
{
    if (active_decoration.frame) {
        Frame const& frame = *active_decoration.frame;

        inner_region.pos.x = frame.extents.left;
        inner_region.pos.y = frame.extents.top;
        inner_region.dim.w = region.dim.w - frame.extents.left - frame.extents.right;
        inner_region.dim.h = region.dim.h - frame.extents.top - frame.extents.bottom;
    } else {
        inner_region.pos.x = 0;
        inner_region.pos.y = 0;
        inner_region.dim = region.dim;
    }
}
