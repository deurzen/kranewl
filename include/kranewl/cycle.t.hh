#pragma once

#include <kranewl/cycle.hh>
#include <kranewl/util.hh>

template <typename T>
HistoryStack<T>::HistoryStack()
    : m_stack({})
{}

template <typename T>
HistoryStack<T>::~HistoryStack()
{}

template <typename T>
void
HistoryStack<T>::clear()
{
    m_stack.clear();
}

template <typename T>
void
HistoryStack<T>::push_back(T element)
{
    m_stack.push_back(element);
}

template <typename T>
void
HistoryStack<T>::replace(T element, T replacement)
{
    std::optional<Index> index = Util::index_of(m_stack, element);

    if (index)
        m_stack[*index] = replacement;
}

template <typename T>
std::optional<T>
HistoryStack<T>::peek_back() const
{
    if (!m_stack.empty())
        return m_stack.back();

    return std::nullopt;
}

template <typename T>
std::optional<T>
HistoryStack<T>::pop_back()
{
    if (!m_stack.empty()) {
        T element = m_stack.back();
        m_stack.pop_back();
        return element;
    }

    return std::nullopt;
}

template <typename T>
void
HistoryStack<T>::remove(T element)
{
    Util::erase_remove(m_stack, element);
}

template <typename T>
std::vector<T> const&
HistoryStack<T>::as_vector() const
{
    return m_stack;
}


template <typename T>
Cycle<T>::Cycle(std::vector<T> elements, bool unwindable)
    : m_index(Util::last_index(elements)),
      m_elements({}),
      m_unwindable(unwindable),
      m_stack(HistoryStack<T>())
{
    m_elements.resize(elements.size());
    std::copy(elements.begin(), elements.end(), m_elements.begin());
}

template <typename T>
Cycle<T>::Cycle(std::initializer_list<T> elements, bool unwindable)
    : m_index(0),
      m_elements({}),
      m_unwindable(unwindable),
      m_stack(HistoryStack<T>())
{
    std::copy(elements.begin(), elements.end(), m_elements.begin());
    m_index = Util::last_index(m_elements);
}

template <typename T>
Cycle<T>::~Cycle()
{}

template <typename T>
bool
Cycle<T>::next_will_wrap(Direction direction) const
{
    switch (direction) {
    case Direction::Backward: return m_index == 0;
    case Direction::Forward: return m_index == Util::last_index(m_elements);
    }
}

template <typename T>
bool
Cycle<T>::empty() const
{
    return m_elements.empty();
}

template <typename T>
bool
Cycle<T>::contains(T element) const
{
    return Util::contains(m_elements, element);
}

template <typename T>
bool
Cycle<T>::is_active_element(T element) const
{
    std::optional<Index> current = this->index();
    return current && *current == index_of_element(element);
}

template <typename T>
bool
Cycle<T>::is_active_index(Index index) const
{
    std::optional<Index> current = this->index();
    return current && *current == index;
}


template <typename T>
std::size_t
Cycle<T>::size() const
{
    return m_elements.size();
}

template <typename T>
std::size_t
Cycle<T>::length() const
{
    return m_elements.size();
}


template <typename T>
std::optional<Index>
Cycle<T>::index() const
{
    if (m_index < m_elements.size())
        return m_index;

    return std::nullopt;
}

template <typename T>
Index
Cycle<T>::active_index() const
{
    return m_index;
}

template <typename T>
Index
Cycle<T>::last_index() const
{
    return Util::last_index(m_elements);
}

template <typename T>
Index
Cycle<T>::next_index(Direction direction) const
{
    return next_index_from(m_index, direction);
}

template <typename T>
Index
Cycle<T>::next_index_from(Index index, Direction direction) const
{
    Index end = Util::last_index(m_elements);

    switch (direction) {
    case Direction::Backward: return index == 0 ? end : index - 1;
    case Direction::Forward: return index == end ? 0 : index + 1;
    default: return index;
    }
}


template <typename T>
std::optional<Index>
Cycle<T>::index_of_element(const T element) const
{
    return Util::index_of(m_elements, element);
}


template <typename T>
std::optional<T>
Cycle<T>::next_element(Direction direction) const
{
    std::optional<Index> index = next_index(direction);
    if (index && *index < m_elements.size())
        return m_elements[*index];

    return std::nullopt;
}

template <typename T>
std::optional<T>
Cycle<T>::active_element() const
{
    if (m_index < m_elements.size())
        return m_elements[m_index];

    return std::nullopt;
}

template <typename T>
std::optional<T>
Cycle<T>::prev_active_element() const
{
    return m_stack.peek_back();
}

template <typename T>
std::optional<T>
Cycle<T>::element_at_index(Index index) const
{
    if (index < m_elements.size())
        return m_elements[index];

    return std::nullopt;
}

template <typename T>
std::optional<T>
Cycle<T>::element_at_front(T) const
{
    if (!m_elements.empty())
        return m_elements[0];

    return std::nullopt;
}

template <typename T>
std::optional<T>
Cycle<T>::element_at_back(T) const
{
    if (!m_elements.empty())
        return m_elements[Util::last_index(m_elements)];

    return std::nullopt;
}


template <typename T>
void
Cycle<T>::activate_first()
{
    activate_at_index(0);
}

template <typename T>
void
Cycle<T>::activate_last()
{
    activate_at_index(Util::last_index(m_elements));
}

template <typename T>
void
Cycle<T>::activate_at_index(Index index)
{
    if (index != m_index) {
        push_active_to_stack();
        m_index = index;
    }
}

template <typename T>
void
Cycle<T>::activate_element(T element)
{
    std::optional<Index> index = Util::index_of(m_elements, element);
    if (index)
        activate_at_index(*index);
}


template <typename T>
bool
Cycle<T>::remove_first()
{
    bool must_resync = is_active_index(0);

    std::size_t size_before = m_elements.size();
    Util::erase_at_index(m_elements, 0);

    if (must_resync)
        sync_active();

    return size_before != m_elements.size();
}

template <typename T>
bool
Cycle<T>::remove_last()
{
    Index end = Util::last_index(m_elements);
    bool must_resync = is_active_index(end);

    std::size_t size_before = m_elements.size();
    Util::erase_at_index(m_elements, end);

    if (must_resync)
        sync_active();

    return size_before != m_elements.size();
}

template <typename T>
bool
Cycle<T>::remove_at_index(Index index)
{
    bool must_resync = is_active_index(index);

    std::size_t size_before = m_elements.size();
    Util::erase_at_index(m_elements, index);

    if (must_resync)
        sync_active();

    return size_before != m_elements.size();
}

template <typename T>
bool
Cycle<T>::remove_element(T element)
{
    std::optional<Index> index = index_of_element(element);
    bool must_resync = is_active_element(element);

    std::size_t size_before = m_elements.size();

    Util::erase_remove(m_elements, element);

    m_stack.remove(element);

    if (m_index != 0 && index && m_index >= *index)
        --m_index;

    if (must_resync)
        sync_active();

    return size_before != m_elements.size();
}


template <typename T>
std::optional<T>
Cycle<T>::pop_back()
{
    std::optional<T> value = std::nullopt;

    if (!m_elements.empty()) {
        bool must_resync = is_active_element(m_elements.back());

        value = std::optional(m_elements.back());
        m_elements.pop_back();

        if (must_resync)
            sync_active();
    }

    return value;
}


template <typename T>
void
Cycle<T>::replace_element(T element, T replacement)
{
    if (contains(replacement))
        return;

    std::optional<Index> index = index_of_element(element);

    if (index) {
        m_elements[*index] = replacement;
        m_stack.replace(element, replacement);
    }
}

template <typename T>
void
Cycle<T>::swap_elements(T element1, T element2)
{
    std::optional<Index> index1 = index_of_element(element1);
    std::optional<Index> index2 = index_of_element(element2);

    if (index1 && index2)
        std::iter_swap(m_elements.begin() + *index1, m_elements.begin() + *index2);
}

template <typename T>
void
Cycle<T>::swap_indices(Index index1, Index index2)
{
    if (index1 < m_elements.size() && index2 < m_elements.size())
        std::iter_swap(m_elements.begin() + index1, m_elements.begin() + index2);
}


template <typename T>
void
Cycle<T>::reverse()
{
    std::reverse(m_elements.begin(), m_elements.end());
}

template <typename T>
void
Cycle<T>::rotate(Direction direction)
{
    switch (direction) {
    case Direction::Backward:
    {
        std::rotate(
            m_elements.rbegin(),
            std::next(m_elements.rbegin()),
            m_elements.rend()
        );

        return;
    }
    case Direction::Forward:
    {
        std::rotate(
            m_elements.begin(),
            std::next(m_elements.begin()),
            m_elements.end()
        );

        return;
    }
    default: return;
    }
}

template <typename T>
void
Cycle<T>::rotate_range(Direction direction, Index begin, Index end)
{
    if (begin >= end || begin >= m_elements.size() || end > m_elements.size())
        return;

    switch (direction) {
    case Direction::Backward:
    {
        std::rotate(
            m_elements.begin() + begin,
            std::next(m_elements.begin() + begin),
            m_elements.begin() + end
        );

        return;
    }
    case Direction::Forward:
    {
        std::rotate(
            m_elements.rend() - end,
            std::next(m_elements.rend() - end),
            m_elements.rend() - begin
        );

        return;
    }
    default: return;
    }
}

template <typename T>
std::optional<T>
Cycle<T>::cycle_active(Direction direction)
{
    push_active_to_stack();
    m_index = next_index(direction);
    return active_element();
}

template <typename T>
std::optional<T>
Cycle<T>::drag_active(Direction direction)
{
    Index index = next_index(direction);

    if (m_index != index && m_index < m_elements.size() && index < m_elements.size())
        std::iter_swap(m_elements.begin() + m_index, m_elements.begin() + index);

    return cycle_active(direction);
}


template <typename T>
void
Cycle<T>::insert_at_front(T element)
{
    push_active_to_stack();
    m_elements.push_front(element);
    m_index = 0;
}

template <typename T>
void
Cycle<T>::insert_at_back(T element)
{
    push_active_to_stack();
    m_elements.push_back(element);
    m_index = Util::last_index(m_elements);
}

template <typename T>
void
Cycle<T>::insert_before_index(Index index, T element)
{
    if (index >= m_elements.size())
        index = Util::last_index(m_elements);

    push_active_to_stack();
    m_elements.insert(m_elements.begin() + index, element);
}

template <typename T>
void
Cycle<T>::insert_after_index(Index index, T element)
{
    if (m_elements.empty() || index >= m_elements.size() - 1) {
        insert_at_back(element);
        return;
    }

    push_active_to_stack();
    m_elements.insert(m_elements.begin() + index + 1, element);
}

template <typename T>
void
Cycle<T>::insert_before_element(T other, T element)
{
    std::optional<Index> index = index_of_element(other);

    if (index)
        insert_before_index(*index, element);
    else
        insert_at_back(element);
}

template <typename T>
void
Cycle<T>::insert_after_element(T other, T element)
{
    std::optional<Index> index = index_of_element(other);

    if (index)
        insert_after_index(*index, element);
    else
        insert_at_back(element);
}


template <typename T>
void
Cycle<T>::clear()
{
    m_elements.clear();
    m_stack.clear();
}


template <typename T>
void
Cycle<T>::sync_active()
{
    std::optional<T> element = get_active_from_stack();
    for (; element && !contains(*element); element = get_active_from_stack());
    if (element)
        m_index = *index_of_element(*element);
}


template <typename T>
void
Cycle<T>::push_index_to_stack(std::optional<Index> index)
{
    if (!m_unwindable || !index)
        return;

    std::optional<T> element = element_at_index(*index);
    if (element) {
        m_stack.remove(*element);
        m_stack.push_back(*element);
    }
}

template <typename T>
void
Cycle<T>::push_active_to_stack()
{
    if (!m_unwindable)
        return;

    push_index_to_stack(index());
}

template <typename T>
std::optional<T>
Cycle<T>::get_active_from_stack()
{
    return m_stack.pop_back();
}


template <typename T>
std::deque<T> const&
Cycle<T>::as_deque() const
{
    return m_elements;
}

template <typename T>
std::vector<T> const&
Cycle<T>::stack() const
{
    return m_stack.as_vector();
}

