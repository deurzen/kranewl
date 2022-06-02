#pragma once

#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
#include <kranewl/layout.hh>
#include <kranewl/placement.hh>
#include <kranewl/util.hh>

typedef struct View* View_ptr;
typedef class Context* Context_ptr;
typedef class Workspace final {
public:
    Workspace(Index index, std::string name, Context_ptr context)
        : m_index(index),
          m_name(name),
          m_layout_handler({}),
          mp_context(context),
          mp_active(nullptr),
          m_views({}, true),
          m_free_views({}, true),
          m_iconified_views({}, true),
          m_disowned_views({}, true),
          m_focus_follows_cursor(true)
    {}

    bool empty() const;
    bool contains(View_ptr) const;

    bool focus_follows_cursor() const;
    void set_focus_follows_cursor(bool);

    bool layout_is_free() const;
    bool layout_has_margin() const;
    bool layout_has_gap() const;
    bool layout_is_persistent() const;
    bool layout_is_single() const;
    bool layout_wraps() const;

    Index size() const;
    Index length() const;
    int main_count() const;

    Context_ptr context() const;

    Index index() const;
    std::string const& name() const;
    std::string identifier() const;
    View_ptr active() const;

    Cycle<View_ptr> const& views() const;
    std::vector<View_ptr> stack_after_focus() const;

    View_ptr next_view() const;
    View_ptr prev_view() const;

    void cycle(Direction);
    void drag(Direction);
    void reverse();
    void rotate(Direction);
    void shuffle_main(Direction);
    void shuffle_stack(Direction);

    void activate_view(View_ptr);

    void add_view(View_ptr);
    void remove_view(View_ptr);
    void replace_view(View_ptr, View_ptr);

    void view_to_icon(View_ptr);
    void icon_to_view(View_ptr);
    void add_icon(View_ptr);
    void remove_icon(View_ptr);
    std::optional<View_ptr> pop_icon();

    void view_to_disowned(View_ptr);
    void disowned_to_view(View_ptr);
    void add_disowned(View_ptr);
    void remove_disowned(View_ptr);

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

    void save_layout(int) const;
    void load_layout(int);

    void toggle_layout();
    void set_layout(LayoutHandler::LayoutKind);
    std::vector<Placement> arrange(Region) const;

    std::deque<View_ptr>::iterator
    begin()
    {
        return m_views.begin();
    }

    std::deque<View_ptr>::const_iterator
    begin() const
    {
        return m_views.begin();
    }

    std::deque<View_ptr>::const_iterator
    cbegin() const
    {
        return m_views.cbegin();
    }

    std::deque<View_ptr>::iterator
    end()
    {
        return m_views.end();
    }

    std::deque<View_ptr>::const_iterator
    end() const
    {
        return m_views.end();
    }

    std::deque<View_ptr>::const_iterator
    cend() const
    {
        return m_views.cend();
    }

    View_ptr
    operator[](Index i)
    {
        return m_views[i];
    }

    View_ptr
    operator[](Index i) const
    {
        return m_views[i];
    }

private:
    Index m_index;
    std::string m_name;

    LayoutHandler m_layout_handler;

    Context_ptr mp_context;

    View_ptr mp_active;
    Cycle<View_ptr> m_views;
    Cycle<View_ptr> m_free_views;
    Cycle<View_ptr> m_iconified_views;
    Cycle<View_ptr> m_disowned_views;

    bool m_focus_follows_cursor;

}* Workspace_ptr;
