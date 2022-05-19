#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>
#include <kranewl/placement.hh>
#include <kranewl/cycle.hh>
#include <kranewl/util.hh>

typedef class Client* Client_ptr;
typedef class Context* Context_ptr;
typedef class Workspace final {
public:
    Workspace(Index index, std::string name, Context_ptr context)
        : m_index(index),
          m_name(name),
          m_layout_handler({}),
          mp_context(context),
          mp_active(nullptr),
          m_clients({}, true),
          m_icons({}, true),
          m_disowned({}, true),
          m_focus_follows_mouse(false)
    {}

    bool empty() const;
    bool contains(Client_ptr) const;

    bool focus_follows_mouse() const;
    void set_focus_follows_mouse(bool);

    bool layout_is_free() const;
    bool layout_has_margin() const;
    bool layout_has_gap() const;
    bool layout_is_persistent() const;
    bool layout_is_single() const;
    bool layout_wraps() const;

    Index size() const;
    Index length() const;
    Index main_count() const;

    Context_ptr context() const;

    Index index() const;
    std::string const& name() const;
    std::string identifier() const;
    Client_ptr active() const;

    Cycle<Client_ptr> const& clients() const;
    std::vector<Client_ptr> stack_after_focus() const;

    Client_ptr next_client() const;
    Client_ptr prev_client() const;

    void cycle(Direction);
    void drag(Direction);
    void reverse();
    void rotate(Direction);
    void shuffle_main(Direction);
    void shuffle_stack(Direction);

    void activate_client(Client_ptr);

    void add_client(Client_ptr);
    void remove_client(Client_ptr);
    void replace_client(Client_ptr, Client_ptr);

    void client_to_icon(Client_ptr);
    void icon_to_client(Client_ptr);
    void add_icon(Client_ptr);
    void remove_icon(Client_ptr);
    std::optional<Client_ptr> pop_icon();

    void client_to_disowned(Client_ptr);
    void disowned_to_client(Client_ptr);
    void add_disowned(Client_ptr);
    void remove_disowned(Client_ptr);

    void toggle_layout_data();
    void cycle_layout_data(Direction);
    void copy_data_from_prev_layout();

    void change_gap_size(Util::Change<int>);
    void change_main_count(Util::Change<int>);
    void change_main_factor(Util::Change<float>);
    void change_margin(Util::Change<int>);
    void change_margin(Edge, Util::Change<int>);
    void reset_gap_size();
    void reset_margin();
    void reset_layout_data();

    void save_layout(Index) const;
    void load_layout(Index);

    void toggle_layout();
    void set_layout(LayoutHandler::LayoutKind);
    std::vector<Placement> arrange(Region) const;

    std::deque<Client_ptr>::iterator
    begin()
    {
        return m_clients.begin();
    }

    std::deque<Client_ptr>::const_iterator
    begin() const
    {
        return m_clients.begin();
    }

    std::deque<Client_ptr>::const_iterator
    cbegin() const
    {
        return m_clients.cbegin();
    }

    std::deque<Client_ptr>::iterator
    end()
    {
        return m_clients.end();
    }

    std::deque<Client_ptr>::const_iterator
    end() const
    {
        return m_clients.end();
    }

    std::deque<Client_ptr>::const_iterator
    cend() const
    {
        return m_clients.cend();
    }

    Client_ptr
    operator[](Index i)
    {
        return m_clients[i];
    }

    Client_ptr
    operator[](Index i) const
    {
        return m_clients[i];
    }

private:
    Index m_index;
    std::string m_name;

    LayoutHandler m_layout_handler;

    Context_ptr mp_context;

    Client_ptr mp_active;
    Cycle<Client_ptr> m_clients;
    Cycle<Client_ptr> m_icons;
    Cycle<Client_ptr> m_disowned;

    bool m_focus_follows_mouse;

}* Workspace_ptr;
