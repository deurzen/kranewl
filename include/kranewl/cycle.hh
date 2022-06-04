#pragma once

#include <kranewl/common.hh>

#include <cstdlib>
#include <deque>
#include <vector>
#include <optional>
#include <unordered_map>
#include <variant>

enum class StackAction {
    Insert,
    Remove
};

template <typename T>
class HistoryStack final {
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
class Cycle final {
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

    template<typename UnaryPredicate>
    std::optional<Index>
    next_index_with_condition(
        Direction direction,
        UnaryPredicate predicate
    ) const
    {
        if (m_index >= m_elements.size())
            return std::nullopt;

        switch (direction) {
        case Direction::Forward:
        {
            auto after_it = m_index != last_index()
                ? std::find_if(
                    m_elements.begin() + m_index + 1,
                    m_elements.end(),
                    predicate
                )
                :  m_elements.end();

            if (after_it != m_elements.end())
                return std::distance(m_elements.begin(), after_it);

            auto before_it = std::find_if(
                m_elements.begin(),
                m_elements.begin() + m_index,
                predicate
            );

            if (before_it != m_elements.begin() + m_index)
                return std::distance(m_elements.begin(), before_it);

            break;
        }
        case Direction::Backward:
        {
            auto after_it = m_index != 0
                ? std::find_if(
                    m_elements.rbegin() + (m_elements.size() - m_index - 1) + 1,
                    m_elements.rend(),
                    predicate
                )
                : m_elements.rend();

            if (after_it != m_elements.rend())
                return std::distance(after_it, m_elements.rend()) - 1;

            auto before_it = std::find_if(
                m_elements.rbegin(),
                m_elements.rbegin() + (m_elements.size() - m_index - 1),
                predicate
            );

            if (before_it != m_elements.rbegin() + (m_elements.size() - m_index - 1))
                return std::distance(before_it, m_elements.rend()) - 1;

            break;
        }
        }

        return std::nullopt;
    }

    template<typename UnaryPredicate>
    std::optional<Index>
    next_index_with_condition_from(
        Index index,
        Direction direction,
        UnaryPredicate predicate
    ) const
    {
        if (index >= m_elements.size())
            return std::nullopt;

        switch (direction) {
        case Direction::Forward:
        {
            auto after_it = std::find_if(
                m_elements.begin() + index,
                m_elements.end(),
                predicate
            );

            if (after_it != m_elements.end())
                return std::distance(m_elements.begin(), after_it);

            auto before_it = std::find_if(
                m_elements.begin(),
                m_elements.begin() + index,
                predicate
            );

            if (before_it != m_elements.begin() + index)
                return std::distance(m_elements.begin(), before_it);

            break;
        }
        case Direction::Backward:
        {
            auto after_it = std::find_if(
                m_elements.rbegin() + (m_elements.size() - index - 1),
                m_elements.rend(),
                predicate
            );

            if (after_it != m_elements.rend())
                return std::distance(m_elements.rbegin(), after_it);

            auto before_it = std::find_if(
                m_elements.rbegin(),
                m_elements.rbegin() + (m_elements.size() - index - 1),
                predicate
            );

            if (before_it != m_elements.rbegin() + (m_elements.size() - index - 1))
                return std::distance(m_elements.rbegin(), before_it);

            break;
        }
        }

        return std::nullopt;
    }

    std::optional<Index> index_of_element(const T) const;

    std::optional<T> next_element(Direction) const;
    std::optional<T> active_element() const;
    std::optional<T> prev_active_element() const;
    std::optional<T> element_at_index(Index) const;
    std::optional<T> element_at_front(T) const;
    std::optional<T> element_at_back(T) const;

    template<typename UnaryPredicate>
    std::optional<T>
    first_element_with_condition(UnaryPredicate predicate)
    {
        auto it = std::find_if(
            m_elements.begin(),
            m_elements.end(),
            predicate
        );

        if (it != m_elements.end())
            return *it;

        return std::nullopt;
    }

    void activate_first();
    void activate_last();
    void activate_at_index(Index);
    void activate_element(T);

    template<typename UnaryPredicate>
    void
    activate_for_condition(UnaryPredicate predicate)
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
    bool
    remove_for_condition(UnaryPredicate predicate)
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

    template<typename UnaryPredicate>
    std::optional<T>
    cycle_active_with_condition(
        Direction direction,
        UnaryPredicate predicate,
        bool wraps
    )
    {
        std::optional<Index> index
            = next_index_with_condition(direction, predicate);

        if (!index)
            return std::nullopt;

        switch (direction) {
        case Direction::Forward:
        {
            if (!wraps && *index <= active_index())
                return std::nullopt;
            break;
        }
        case Direction::Backward:
        {
            if (!wraps && *index >= last_index())
                return std::nullopt;
            break;
        }
        }

        push_active_to_stack();
        m_index = *index;
        return active_element();
    }

    template<typename UnaryPredicate>
    std::optional<T>
    drag_active_with_condition(
        Direction direction,
        UnaryPredicate predicate,
        bool wraps
    )
    {
        std::optional<Index> index
            = next_index_with_condition(direction, predicate);

        if (!index)
            return std::nullopt;

        switch (direction) {
        case Direction::Forward:
        {
            if (!wraps && *index <= active_index())
                return std::nullopt;
            break;
        }
        case Direction::Backward:
        {
            if (!wraps && *index >= last_index())
                return std::nullopt;
            break;
        }
        }

        if (m_index != *index && m_index < m_elements.size() && *index < m_elements.size())
            std::iter_swap(m_elements.begin() + m_index, m_elements.begin() + *index);

        return cycle_active_with_condition(direction, predicate, wraps);
    }

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

    T
    operator[](std::size_t index)
    {
        return m_elements[index];
    }

    const T
    operator[](std::size_t index) const
    {
        return m_elements[index];
    }

private:
    void sync_active();
    void push_index_to_stack(std::optional<Index>);
    void push_active_to_stack();
    std::optional<T> get_active_from_stack();

    Index m_index;
    std::deque<T> m_elements;

    bool m_unwindable;
    HistoryStack<T> m_stack;

};
