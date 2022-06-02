#pragma once

#include <kranewl/common.hh>
#include <kranewl/cycle.hh>

#include <string>
#include <unordered_set>

typedef class Client* Client_ptr;
typedef class Workspace* Workspace_ptr;
typedef class Output* Output_ptr;
typedef class Context final {
public:
    Context(Index index, std::string name)
        : m_index(index),
          m_name(name),
          mp_output(nullptr),
          mp_active(nullptr),
          mp_prev_active(nullptr),
          m_workspaces({}, true),
          m_focus_follows_cursor(true)
    {}

    Index
    index() const
    {
        return m_index;
    }

    std::string const&
    name() const
    {
        return m_name;
    }

    std::size_t
    size() const
    {
        return m_workspaces.size();
    }

    Workspace_ptr
    workspace() const
    {
        return mp_active;
    }

    Workspace_ptr
    prev_workspace() const
    {
        return mp_prev_active;
    }

    Output_ptr
    output() const
    {
        return mp_output;
    }

    bool
    is_outputed() const
    {
        return mp_output != nullptr;
    }

    void
    set_output(Output_ptr output)
    {
        mp_output = output;
    }

    void
    register_workspace(Workspace_ptr workspace)
    {
        m_workspaces.insert_at_back(workspace);
    }

    void
    activate_workspace(Index index)
    {
        Workspace_ptr prev_active = mp_active;
        m_workspaces.activate_at_index(index);
        mp_active = *m_workspaces.active_element();

        if (prev_active != mp_active)
            mp_prev_active = prev_active;
    }

    void
    activate_workspace(Workspace_ptr workspace)
    {
        Workspace_ptr prev_active = mp_active;
        m_workspaces.activate_element(workspace);
        mp_active = workspace;

        if (prev_active != mp_active)
            mp_prev_active = prev_active;
    }

    Cycle<Workspace_ptr> const&
    workspaces() const
    {
        return m_workspaces;
    }

    void
    register_sticky_client(Client_ptr client)
    {
        m_sticky_clients.insert(client);
    }

    void
    unregister_sticky_client(Client_ptr client)
    {
        m_sticky_clients.erase(client);
    }

    std::unordered_set<Client_ptr> const&
    sticky_clients() const
    {
        return m_sticky_clients;
    }

    bool
    focus_follows_cursor() const
    {
        return m_focus_follows_cursor;
    }

    void
    set_focus_follows_cursor(bool focus_follows_cursor)
    {
        m_focus_follows_cursor = focus_follows_cursor;
    }

    std::deque<Workspace_ptr>::iterator
    begin()
    {
        return m_workspaces.begin();
    }

    std::deque<Workspace_ptr>::const_iterator
    begin() const
    {
        return m_workspaces.begin();
    }

    std::deque<Workspace_ptr>::const_iterator
    cbegin() const
    {
        return m_workspaces.cbegin();
    }

    std::deque<Workspace_ptr>::iterator
    end()
    {
        return m_workspaces.end();
    }

    std::deque<Workspace_ptr>::const_iterator
    end() const
    {
        return m_workspaces.end();
    }

    std::deque<Workspace_ptr>::const_iterator
    cend() const
    {
        return m_workspaces.cend();
    }

    Workspace_ptr
    operator[](std::size_t i)
    {
        return m_workspaces[i];
    }

    Workspace_ptr
    operator[](std::size_t i) const
    {
        return m_workspaces[i];
    }

private:
    Index m_index;
    std::string m_name;

    Output_ptr mp_output;

    Workspace_ptr mp_active;
    Workspace_ptr mp_prev_active;

    Cycle<Workspace_ptr> m_workspaces;
    std::unordered_set<Client_ptr> m_sticky_clients;

    bool m_focus_follows_cursor;

}* Context_ptr;
