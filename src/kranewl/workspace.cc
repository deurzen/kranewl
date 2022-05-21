#include <kranewl/workspace.hh>

#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/tree/client.hh>
#include <kranewl/util.hh>

#include <algorithm>
#include <optional>

bool
Workspace::empty() const
{
    return m_clients.empty();
}

bool
Workspace::contains(Client_ptr client) const
{
    return m_clients.contains(client);
}

bool
Workspace::focus_follows_mouse() const
{
    return m_focus_follows_mouse;
}

void
Workspace::set_focus_follows_mouse(bool focus_follows_mouse)
{
    m_focus_follows_mouse = focus_follows_mouse;
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
    return m_clients.size();
}

std::size_t
Workspace::length() const
{
    return m_clients.length();
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

Client_ptr
Workspace::active() const
{
    return mp_active;
}


Cycle<Client_ptr> const&
Workspace::clients() const
{
    return m_clients;
}

std::vector<Client_ptr>
Workspace::stack_after_focus() const
{
    std::vector<Client_ptr> stack = m_clients.stack();

    if (mp_active) {
        Util::erase_remove(stack, mp_active);
        stack.push_back(mp_active);
    }

    return stack;
}

Client_ptr
Workspace::next_client() const
{
    std::optional<Client_ptr> client
        = m_clients.next_element(Direction::Forward);

    if (client != mp_active)
        return *client;

    return nullptr;
}

Client_ptr
Workspace::prev_client() const
{
    std::optional<Client_ptr> client
        = m_clients.next_element(Direction::Backward);

    if (client != mp_active)
        return *client;

    return nullptr;
}

void
Workspace::cycle(Direction direction)
{
    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && m_clients.active_index() == m_clients.last_index())
            return;

        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && m_clients.active_index() == 0)
            return;

        break;
    }
    }

    m_clients.cycle_active(direction);
    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::drag(Direction direction)
{
    switch (direction) {
    case Direction::Forward:
    {
        if (!layout_wraps() && m_clients.active_index() == m_clients.last_index())
            return;

        break;
    }
    case Direction::Backward:
    {
        if (!layout_wraps() && m_clients.active_index() == 0)
            return;

        break;
    }
    }

    m_clients.drag_active(direction);
    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::reverse()
{
    m_clients.reverse();
    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::rotate(Direction direction)
{
    m_clients.rotate(direction);
    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::shuffle_main(Direction direction)
{
    m_clients.rotate_range(
        direction,
        0,
        static_cast<Index>(m_layout_handler.main_count())
    );

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::shuffle_stack(Direction direction)
{
    m_clients.rotate_range(
        direction,
        static_cast<Index>(m_layout_handler.main_count()),
        m_clients.size()
    );

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::activate_client(Client_ptr client)
{
    if (m_clients.contains(client)) {
        m_clients.activate_element(client);
        mp_active = client;
    }
}

void
Workspace::add_client(Client_ptr client)
{
    if (m_clients.contains(client))
        return;

    m_clients.insert_at_back(client);
    mp_active = client;
}

void
Workspace::remove_client(Client_ptr client)
{
    m_clients.remove_element(client);
    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::replace_client(Client_ptr client, Client_ptr replacement)
{
    bool was_active
        = m_clients.active_element().value_or(nullptr) == client;

    m_clients.replace_element(client, replacement);

    if (was_active) {
        m_clients.activate_element(replacement);
        mp_active = replacement;
    }
}

void
Workspace::client_to_icon(Client_ptr client)
{
    if (m_clients.remove_element(client))
        m_icons.insert_at_back(client);

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::icon_to_client(Client_ptr client)
{
    if (m_icons.remove_element(client))
        m_clients.insert_at_back(client);

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::add_icon(Client_ptr client)
{
    if (m_icons.contains(client))
        return;

    m_icons.insert_at_back(client);
}

void
Workspace::remove_icon(Client_ptr client)
{
    m_icons.remove_element(client);
}

std::optional<Client_ptr>
Workspace::pop_icon()
{
    return m_icons.empty()
        ? std::nullopt
        : std::optional(m_icons[m_icons.size() - 1]);
}

void
Workspace::client_to_disowned(Client_ptr client)
{
    if (m_clients.remove_element(client))
        m_disowned.insert_at_back(client);

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::disowned_to_client(Client_ptr client)
{
    if (m_disowned.remove_element(client))
        m_clients.insert_at_back(client);

    mp_active = m_clients.active_element().value_or(nullptr);
}

void
Workspace::add_disowned(Client_ptr client)
{
    if (m_disowned.contains(client))
        return;

    m_disowned.insert_at_back(client);
}

void
Workspace::remove_disowned(Client_ptr client)
{
    m_disowned.remove_element(client);
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
    std::deque<Client_ptr> clients = m_clients.as_deque();
    std::vector<Placement> placements;
    placements.reserve(clients.size());

    auto fullscreen_iter = std::stable_partition(
        clients.begin(),
        clients.end(),
        [](const Client_ptr client) -> bool {
            return client->m_fullscreen && !client->m_contained;
        }
    );

    auto free_iter = std::stable_partition(
        fullscreen_iter,
        clients.end(),
        [=,this](const Client_ptr client) -> bool {
            return !layout_is_free() && Client::is_free(client);
        }
    );

    std::transform(
        clients.begin(),
        fullscreen_iter,
        std::back_inserter(placements),
        [region](const Client_ptr client) -> Placement {
            return Placement {
                Placement::PlacementMethod::Tile,
                client,
                NO_DECORATION,
                region
            };
        }
    );

    std::transform(
        fullscreen_iter,
        free_iter,
        std::back_inserter(placements),
        [](const Client_ptr client) -> Placement {
            return Placement {
                Placement::PlacementMethod::Free,
                client,
                FREE_DECORATION,
                client->m_free_region
            };
        }
    );

    m_layout_handler.arrange(
        region,
        placements,
        free_iter,
        clients.end()
    );

    if (layout_is_single()) {
        std::for_each(
            placements.begin(),
            placements.end(),
            [](Placement& placement) {
                if (!placement.client->m_focused)
                    placement.region = std::nullopt;
            }
        );
    }

    return placements;
}
