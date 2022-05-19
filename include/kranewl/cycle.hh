#pragma once

#include <kranewl/common.hh>

#include <cstdlib>
#include <deque>
#include <vector>
#include <optional>
#include <unordered_map>
#include <variant>

enum class StackAction
{
    Insert,
    Remove
};

template <typename T>
class HistoryStack final
{
    static_assert(std::is_pointer<T>::value,
        "Only pointer types may be stored in a history stack.");

public:
    HistoryStack();
    ~HistoryStack();

    void clear();
    void push_back(T);
    void replace(T, T);
    std::optional<T> peek_back() const;
    std::optional<T> pop_back();
    void remove(T);

    std::vector<T> const& as_vector() const;

private:
    std::vector<T> m_stack;

};

template <typename T>
class Cycle final
{
    static_assert(std::is_pointer<T>::value,
        "Only pointer types may be stored in a cycle.");

public:
    Cycle(std::vector<T>, bool);
    Cycle(std::initializer_list<T>, bool);
    ~Cycle();

    bool next_will_wrap(Direction) const;
    bool empty() const;
    bool contains(T) const;
    bool is_active_element(T) const;
    bool is_active_index(Index) const;

    std::size_t size() const;
    std::size_t length() const;

    std::optional<Index> index() const;
    Index active_index() const;
    Index last_index() const;
    Index next_index(Direction) const;
    Index next_index_from(Index, Direction) const;

    std::optional<Index> index_of_element(const T) const;

    std::optional<T> next_element(Direction) const;
    std::optional<T> active_element() const;
    std::optional<T> prev_active_element() const;
    std::optional<T> element_at_index(Index) const;
    std::optional<T> element_at_front(T) const;
    std::optional<T> element_at_back(T) const;

    void activate_first();
    void activate_last();
    void activate_at_index(Index);
    void activate_element(T);

    template<typename UnaryPredicate>
    void activate_for_condition(UnaryPredicate predicate)
    {
        auto it = std::find_if(
            m_elements.begin(),
            m_elements.end(),
            predicate
        );

        if (it != m_elements.end())
            activate_element(*it);
    }

    bool remove_first();
    bool remove_last();
    bool remove_at_index(Index);
    bool remove_element(T);

    template<typename UnaryPredicate>
    bool remove_for_condition(UnaryPredicate predicate)
    {
        auto it = std::find_if(
            m_elements.begin(),
            m_elements.end(),
            predicate
        );

        if (it != m_elements.end())
            return remove_element(*it);

        return false;
    }

    std::optional<T> pop_back();

    void replace_element(T, T);
    void swap_elements(T, T);
    void swap_indices(Index, Index);

    void reverse();
    void rotate(Direction);
    void rotate_range(Direction, Index, Index);
    std::optional<T> cycle_active(Direction);
    std::optional<T> drag_active(Direction);

    void insert_at_front(T);
    void insert_at_back(T);
    void insert_before_index(Index, T);
    void insert_after_index(Index, T);
    void insert_before_element(T, T);
    void insert_after_element(T, T);

    void clear();

    std::deque<T> const& as_deque() const;
    std::vector<T> const& stack() const;

    typename std::deque<T>::iterator
    begin()
    {
        return m_elements.begin();
    }

    typename std::deque<T>::const_iterator
    begin() const
    {
        return m_elements.begin();
    }

    typename std::deque<T>::const_iterator
    cbegin() const
    {
        return m_elements.cbegin();
    }

    typename std::deque<T>::iterator
    end()
    {
        return m_elements.end();
    }

    typename std::deque<T>::const_iterator
    end() const
    {
        return m_elements.end();
    }

    typename std::deque<T>::const_iterator
    cend() const
    {
        return m_elements.cend();
    }

    T operator[](std::size_t index)
    {
        return m_elements[index];
    }

    const T operator[](std::size_t index) const
    {
        return m_elements[index];
    }

private:
    Index m_index;

    std::deque<T> m_elements;

    bool m_unwindable;
    HistoryStack<T> m_stack;

    void sync_active();

    void push_index_to_stack(std::optional<Index>);
    void push_active_to_stack();
    std::optional<T> get_active_from_stack();

};
