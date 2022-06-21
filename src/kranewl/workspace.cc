#include <trace.hh>

#include <kranewl/workspace.hh>

#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

#include <algorithm>
#include <optional>

Workspace::Workspace(Index index, std::string name, Context_ptr context)
    : m_index(index),
      m_name(name),
      m_layout_handler({}),
      mp_context(context),
      mp_active(nullptr),
      m_views({}, true),
      m_track_layer(SceneLayer::SCENE_LAYER_FREE),
      m_prev_track_layer(SceneLayer::SCENE_LAYER_TILE),
      m_tracks({
          new Cycle<View_ptr>{{}, false}, // SceneLayer::SCENE_LAYER_BACKGROUND
          new Cycle<View_ptr>{{}, true},  // SceneLayer::SCENE_LAYER_BOTTOM
          new Cycle<View_ptr>{{}, true},  // SceneLayer::SCENE_LAYER_TILE
          new Cycle<View_ptr>{{}, true},  // SceneLayer::SCENE_LAYER_FREE
          new Cycle<View_ptr>{{}, true},  // SceneLayer::SCENE_LAYER_TOP
          new Cycle<View_ptr>{{}, true},  // SceneLayer::SCENE_LAYER_OVERLAY
          new Cycle<View_ptr>{{}, false}, // SceneLayer::SCENE_LAYER_POPUP
          new Cycle<View_ptr>{{}, false}, // SceneLayer::SCENE_LAYER_NOFOCUS
      }),
      mp_track(m_tracks[SceneLayer::SCENE_LAYER_FREE]),
      m_free_views({}, true),
      m_iconified_views({}, true),
      m_disowned_views({}, true),
      m_focus_follows_cursor(true)
{}

Workspace::~Workspace()
{}

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
Workspace::track_size() const
{
    return mp_track->size();
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

Output_ptr
Workspace::output() const
{
    return mp_context->output();
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


SceneLayer
Workspace::track_layer() const
{
    return m_track_layer;
}

void
Workspace::activate_track(SceneLayer layer)
{
    TRACE();

    mp_track = m_tracks[layer];
    mp_active = mp_track->active_element().value_or(nullptr);
    m_prev_track_layer = m_track_layer;
    m_track_layer = layer;

    if (mp_active)
        m_views.activate_element(mp_active);
}

void
Workspace::toggle_track()
{
    TRACE();
    activate_track(m_prev_track_layer);
}

bool
Workspace::cycle_track(Direction direction)
{
    TRACE();

    int last_index, cycles;
    last_index = cycles = Util::last_index(m_tracks);

    do {
        switch (direction) {
        case Direction::Backward:
            m_track_layer = static_cast<SceneLayer>(
                m_track_layer == 0
                    ? last_index
                    : m_track_layer - 1
            );
            break;
        case Direction::Forward:
            m_track_layer = static_cast<SceneLayer>(
                m_track_layer == last_index
                    ? 0
                    : m_track_layer + 1
            );
            break;
        default: return false;
        }
    } while (m_tracks[m_track_layer]->empty() && cycles--);

    activate_track(m_track_layer);
    return cycles >= 0;
}

void
Workspace::add_view_to_track(View_ptr view, SceneLayer layer)
{
    TRACE();

    if (m_tracks[layer]->contains(view))
        return;

    m_tracks[layer]->insert_at_back(view);

    if (layer == m_track_layer)
        mp_active = view;
}

void
Workspace::remove_view_from_track(View_ptr view, SceneLayer layer)
{
    TRACE();

    m_tracks[layer]->remove_element(view);

    if (layer == m_track_layer) {
        mp_active = m_tracks[layer]->active_element().value_or(nullptr);

        if (mp_active)
            m_views.activate_element(mp_active);
        else
            cycle_track(Direction::Forward);
    }
}

void
Workspace::change_view_track(View_ptr view, SceneLayer layer)
{
    TRACE();

    if (view->scene_layer() == layer)
        return;

    remove_view_from_track(view, view->scene_layer());
    add_view_to_track(view, layer);
}


Cycle<View_ptr> const&
Workspace::views() const
{
    return m_views;
}

View_ptr
Workspace::next_view() const
{
    std::optional<View_ptr> view
        = m_views.next_element(Direction::Forward);

    if (view && *view != mp_active)
        return *view;

    return nullptr;
}

View_ptr
Workspace::prev_view() const
{
    std::optional<View_ptr> view
        = m_views.next_element(Direction::Backward);

    if (view && *view != mp_active)
        return *view;

    return nullptr;
}

std::optional<View_ptr>
Workspace::find_view(ViewSelector const& selector) const
{
    TRACE();

    if (m_views.empty())
        return std::nullopt;

    switch (selector.criterium()) {
    case ViewSelector::SelectionCriterium::AtFirst:
    {
        return m_views[0];
    }
    case ViewSelector::SelectionCriterium::AtLast:
    {
        return m_views[Util::last_index(m_views.as_deque())];
    }
    case ViewSelector::SelectionCriterium::AtMain:
    {
        std::size_t main_count = m_layout_handler.main_count();

        if (main_count <= m_views.size())
            return m_views[main_count];

        break;
    }
    case ViewSelector::SelectionCriterium::AtIndex:
    {
        std::size_t index = selector.index();

        if (index <= m_views.size())
            return m_views[index];

        break;
    }
    }

    return std::nullopt;
}

std::pair<std::optional<View_ptr>, std::optional<View_ptr>>
Workspace::cycle_focus(Direction direction)
{
    TRACE();

    View_ptr prev_active = mp_active;

    switch (direction) {
    case Direction::Forward:
    {
        if (mp_track->active_index() == mp_track->last_index()) {
            if (!cycle_track(direction))
                return std::pair{std::nullopt, std::nullopt};
            else
                mp_track->activate_at_index(0);
        } else
            return cycle_focus_track(direction);

        break;
    }
    case Direction::Backward:
    {
        if (mp_track->active_index() == 0) {
            if (!cycle_track(direction))
                return std::pair{std::nullopt, std::nullopt};
            else
                mp_track->activate_at_index(mp_track->last_index());
        } else
            return cycle_focus_track(direction);

        break;
    }
    }

    mp_active = mp_track->active_element().value_or(nullptr);
    if (mp_active)
        m_views.activate_element(mp_active);

    return std::pair{
        prev_active
            ? std::optional{prev_active}
            : std::nullopt,
        mp_active
            ? std::optional{mp_active}
            : std::nullopt
    };
}

std::pair<std::optional<View_ptr>, std::optional<View_ptr>>
Workspace::cycle_focus_track(Direction direction)
{
    TRACE();

    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && mp_track->active_index() == mp_track->last_index())
            return std::pair{std::nullopt, std::nullopt};
        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && mp_track->active_index() == 0)
            return std::pair{std::nullopt, std::nullopt};
        break;
    }
    }

    std::pair<std::optional<View_ptr>, std::optional<View_ptr>> views
        = mp_track->cycle_active(direction);

    mp_active = views.second.value_or(nullptr);
    if (mp_active)
        m_views.activate_element(mp_active);

    return views;
}

std::pair<std::optional<View_ptr>, std::optional<View_ptr>>
Workspace::drag_focus_track(Direction direction)
{
    TRACE();

    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && mp_track->active_index() == mp_track->last_index())
            return std::pair{std::nullopt, std::nullopt};
        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && mp_track->active_index() == 0)
            return std::pair{std::nullopt, std::nullopt};
        break;
    }
    }

    std::pair<std::optional<View_ptr>, std::optional<View_ptr>> views
        = mp_track->drag_active(direction);

    if (views.first && views.second)
        m_views.swap_elements(*views.first, *views.second);

    mp_active = views.second.value_or(nullptr);
    if (mp_active)
        m_views.activate_element(mp_active);

    return views;
}

void
Workspace::reverse()
{
    TRACE();
    m_views.reverse();
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::rotate(Direction direction)
{
    TRACE();
    m_views.rotate(direction);
    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::shuffle_main(Direction direction)
{
    TRACE();

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
    TRACE();

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
    TRACE();

    if (m_views.contains(view)) {
        m_views.activate_element(view);
        m_tracks[view->scene_layer()]->activate_element(view);
        mp_active = view;
    }
}

void
Workspace::add_view(View_ptr view)
{
    TRACE();

    if (m_views.contains(view))
        return;

    m_views.insert_at_back(view);
    m_tracks[view->scene_layer()]->insert_at_back(view);

    m_track_layer = view->scene_layer();
    mp_active = view;
}

void
Workspace::remove_view(View_ptr view)
{
    TRACE();

    m_views.remove_element(view);
    remove_view_from_track(view, view->scene_layer());
}

void
Workspace::replace_view(View_ptr view, View_ptr replacement)
{
    TRACE();

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
    TRACE();

    if (m_views.remove_element(view))
        m_iconified_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::icon_to_view(View_ptr view)
{
    TRACE();

    if (m_iconified_views.remove_element(view))
        m_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::add_icon(View_ptr view)
{
    TRACE();

    if (m_iconified_views.contains(view))
        return;

    m_iconified_views.insert_at_back(view);
}

void
Workspace::remove_icon(View_ptr view)
{
    TRACE();
    m_iconified_views.remove_element(view);
}

std::optional<View_ptr>
Workspace::pop_icon()
{
    TRACE();

    return m_iconified_views.empty()
        ? std::nullopt
        : std::optional(m_iconified_views[m_iconified_views.size() - 1]);
}

void
Workspace::view_to_disowned(View_ptr view)
{
    TRACE();

    if (m_views.remove_element(view))
        m_disowned_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::disowned_to_view(View_ptr view)
{
    TRACE();

    if (m_disowned_views.remove_element(view))
        m_views.insert_at_back(view);

    mp_active = m_views.active_element().value_or(nullptr);
}

void
Workspace::add_disowned(View_ptr view)
{
    TRACE();

    if (m_disowned_views.contains(view))
        return;

    m_disowned_views.insert_at_back(view);
}

void
Workspace::remove_disowned(View_ptr view)
{
    TRACE();
    m_disowned_views.remove_element(view);
}

void
Workspace::save_layout(int number) const
{
    TRACE();
    m_layout_handler.save_layout(number);
}

void
Workspace::load_layout(int number)
{
    TRACE();
    m_layout_handler.load_layout(number);
}

void
Workspace::toggle_layout_data()
{
    TRACE();
    m_layout_handler.set_prev_layout_data();
}

void
Workspace::cycle_layout_data(Direction direction)
{
    TRACE();
    m_layout_handler.cycle_layout_data(direction);
}

void
Workspace::copy_data_from_prev_layout()
{
    TRACE();
    m_layout_handler.copy_data_from_prev_layout();
}


void
Workspace::change_gap_size(Util::Change<int> change)
{
    TRACE();
    m_layout_handler.change_gap_size(change);
}

void
Workspace::change_main_count(Util::Change<int> change)
{
    TRACE();
    m_layout_handler.change_main_count(change);
}

void
Workspace::change_main_factor(Util::Change<float> change)
{
    TRACE();
    m_layout_handler.change_main_factor(change);
}

void
Workspace::change_margin(Util::Change<int> change)
{
    TRACE();
    m_layout_handler.change_margin(change);
}

void
Workspace::change_margin(Edge edge, Util::Change<int> change)
{
    TRACE();
    m_layout_handler.change_margin(edge, change);
}

void
Workspace::reset_gap_size()
{
    TRACE();
    m_layout_handler.reset_gap_size();
}

void
Workspace::reset_margin()
{
    TRACE();
    m_layout_handler.reset_margin();
}

void
Workspace::reset_layout_data()
{
    TRACE();
    m_layout_handler.reset_layout_data();
}


void
Workspace::toggle_layout()
{
    TRACE();
    m_layout_handler.set_prev_kind();
}

void
Workspace::set_layout(LayoutHandler::LayoutKind layout)
{
    TRACE();
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
