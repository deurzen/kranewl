#include <trace.hh>

#include <kranewl/workspace.hh>

#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

#include <algorithm>
#include <optional>

bool
Workspace::empty() const
{
    return m_views.empty();
}

bool
Workspace::contains(View_ptr view) const
{
    return m_views.contains(view);
}

bool
Workspace::focus_follows_cursor() const
{
    return m_focus_follows_cursor;
}

void
Workspace::set_focus_follows_cursor(bool focus_follows_cursor)
{
    m_focus_follows_cursor = focus_follows_cursor;
}

bool
Workspace::layout_is_free() const
{
    return m_layout_handler.layout_is_free();
}

bool
Workspace::layout_has_margin() const
{
    return m_layout_handler.layout_has_margin();
}

bool
Workspace::layout_has_gap() const
{
    return m_layout_handler.layout_has_gap();
}

bool
Workspace::layout_is_persistent() const
{
    return m_layout_handler.layout_is_persistent();
}

bool
Workspace::layout_is_single() const
{
    return m_layout_handler.layout_is_single();
}

bool
Workspace::layout_wraps() const
{
    return m_layout_handler.layout_wraps();
}

std::size_t
Workspace::size() const
{
    return m_views.size();
}

std::size_t
Workspace::length() const
{
    return m_views.length();
}

int
Workspace::main_count() const
{
    return m_layout_handler.main_count();
}

Context_ptr
Workspace::context() const
{
    return mp_context;
}

Index
Workspace::index() const
{
    return m_index;
}

std::string const&
Workspace::name() const
{
    return m_name;
}

std::string
Workspace::identifier() const
{
    if (!m_name.empty())
        return mp_context->name()
            + ":"
            + std::to_string(m_index)
            + ":"
            + m_name;

    return mp_context->name()
        + ":"
        + std::to_string(m_index);
}

View_ptr
Workspace::active() const
{
    return mp_active;
}


Cycle<View_ptr> const&
Workspace::views() const
{
    return m_views;
}

std::vector<View_ptr>
Workspace::stack_after_focus() const
{
    std::vector<View_ptr> stack = m_views.stack();

    if (mp_active) {
        Util::erase_remove(stack, mp_active);
        stack.push_back(mp_active);
    }

    return stack;
}

View_ptr
Workspace::next_view() const
{
    std::optional<View_ptr> view
        = m_views.next_element(Direction::Forward);

    if (view != mp_active)
        return *view;

    return nullptr;
}

View_ptr
Workspace::prev_view() const
{
    std::optional<View_ptr> view
        = m_views.next_element(Direction::Backward);

    if (view != mp_active)
        return *view;

    return nullptr;
}

void
Workspace::cycle(Direction direction)
{
    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && m_views.active_index() == m_views.last_index())
            return;

        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && m_views.active_index() == 0)
            return;

        break;
    }
    }

    m_views.cycle_active(direction);
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::drag(Direction direction)
{
    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && m_views.active_index() == m_views.last_index())
            return;

        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && m_views.active_index() == 0)
            return;

        break;
    }
    }

    m_views.drag_active(direction);
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::reverse()
{
    m_views.reverse();
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::rotate(Direction direction)
{
    m_views.rotate(direction);
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::shuffle_main(Direction direction)
{
    m_views.rotate_range(
        direction,
        0,
        static_cast<Index>(m_layout_handler.main_count())
    );

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::shuffle_stack(Direction direction)
{
    m_views.rotate_range(
        direction,
        static_cast<Index>(m_layout_handler.main_count()),
        m_views.size()
    );

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::activate_view(View_ptr view)
{
    if (m_views.contains(view)) {
        m_views.activate_element(view);
        mp_active = view;
    }
}

void
Workspace::add_view(View_ptr view)
{
    if (m_views.contains(view))
        return;

    m_views.insert_at_back(view);
    mp_active = view;
}

void
Workspace::remove_view(View_ptr view)
{
    m_views.remove_element(view);
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::replace_view(View_ptr view, View_ptr replacement)
{
    bool was_active
        = m_views.active_element().value_or(nullptr) == view;

    m_views.replace_element(view, replacement);

    if (was_active) {
        m_views.activate_element(replacement);
        mp_active = replacement;
    }
}

void
Workspace::view_to_icon(View_ptr view)
{
    if (m_views.remove_element(view))
        m_iconified_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::icon_to_view(View_ptr view)
{
    if (m_iconified_views.remove_element(view))
        m_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::add_icon(View_ptr view)
{
    if (m_iconified_views.contains(view))
        return;

    m_iconified_views.insert_at_back(view);
}

void
Workspace::remove_icon(View_ptr view)
{
    m_iconified_views.remove_element(view);
}

std::optional<View_ptr>
Workspace::pop_icon()
{
    return m_iconified_views.empty()
        ? std::nullopt
        : std::optional(m_iconified_views[m_iconified_views.size() - 1]);
}

void
Workspace::view_to_disowned(View_ptr view)
{
    if (m_views.remove_element(view))
        m_disowned_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::disowned_to_view(View_ptr view)
{
    if (m_disowned_views.remove_element(view))
        m_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::add_disowned(View_ptr view)
{
    if (m_disowned_views.contains(view))
        return;

    m_disowned_views.insert_at_back(view);
}

void
Workspace::remove_disowned(View_ptr view)
{
    m_disowned_views.remove_element(view);
}

void
Workspace::save_layout(int number) const
{
    m_layout_handler.save_layout(number);
}

void
Workspace::load_layout(int number)
{
    m_layout_handler.load_layout(number);
}

void
Workspace::toggle_layout_data()
{
    m_layout_handler.set_prev_layout_data();
}

void
Workspace::cycle_layout_data(Direction direction)
{
    m_layout_handler.cycle_layout_data(direction);
}

void
Workspace::copy_data_from_prev_layout()
{
    m_layout_handler.copy_data_from_prev_layout();
}


void
Workspace::change_gap_size(Util::Change<int> change)
{
    m_layout_handler.change_gap_size(change);
}

void
Workspace::change_main_count(Util::Change<int> change)
{
    m_layout_handler.change_main_count(change);
}

void
Workspace::change_main_factor(Util::Change<float> change)
{
    m_layout_handler.change_main_factor(change);
}

void
Workspace::change_margin(Util::Change<int> change)
{
    m_layout_handler.change_margin(change);
}

void
Workspace::change_margin(Edge edge, Util::Change<int> change)
{
    m_layout_handler.change_margin(edge, change);
}

void
Workspace::reset_gap_size()
{
    m_layout_handler.reset_gap_size();
}

void
Workspace::reset_margin()
{
    m_layout_handler.reset_margin();
}

void
Workspace::reset_layout_data()
{
    m_layout_handler.reset_layout_data();
}


void
Workspace::toggle_layout()
{
    m_layout_handler.set_prev_kind();
}

void
Workspace::set_layout(LayoutHandler::LayoutKind layout)
{
    m_layout_handler.set_kind(layout);
}

std::vector<Placement>
Workspace::arrange(Region region) const
{
    TRACE();

    std::deque<View_ptr> views = m_views.as_deque();
    std::vector<Placement> placements;
    placements.reserve(views.size());

    auto fullscreen_iter = std::stable_partition(
        views.begin(),
        views.end(),
        [](const View_ptr view) -> bool {
            return view->fullscreen() && !view->contained();
        }
    );

    auto free_iter = std::stable_partition(
        fullscreen_iter,
        views.end(),
        [=,this](const View_ptr view) -> bool {
            return !layout_is_free() && View::is_free(view);
        }
    );

    std::transform(
        views.begin(),
        fullscreen_iter,
        std::back_inserter(placements),
        [region](const View_ptr view) -> Placement {
            return Placement {
                Placement::PlacementMethod::Tile,
                view,
                NO_DECORATION,
                region
            };
        }
    );

    std::transform(
        fullscreen_iter,
        free_iter,
        std::back_inserter(placements),
        [](const View_ptr view) -> Placement {
            return Placement {
                Placement::PlacementMethod::Free,
                view,
                FREE_DECORATION,
                view->free_region()
            };
        }
    );

    m_layout_handler.arrange(
        region,
        placements,
        free_iter,
        views.end()
    );

    if (layout_is_single()) {
        std::for_each(
            placements.begin(),
            placements.end(),
            [](Placement& placement) {
                if (!placement.view->focused())
                    placement.region = std::nullopt;
            }
        );
    }

    return placements;
}
