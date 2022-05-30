#pragma once

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Util
{

    template <
        typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type
    >
    struct Change final
    {
        Change(T value)
            : value(value)
        {}

        operator T() const { return value; }

        T value;
    };

    template<typename T>
    struct is_iterable final
    {
    private:
        template<typename Container> static char test(typename Container::iterator*);
        template<typename Container> static int  test(...);
    public:
        enum { value = sizeof(test<T>(0)) == sizeof(char) };
    };

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, void>::type
    erase_remove(Container& c, typename Container::value_type const& t)
    {
        auto iter = std::remove(c.begin(), c.end(), t);

        if (iter == c.end())
            return;

        c.erase(iter, c.end());
    }

    template <typename Container, typename UnaryPredicate>
    typename std::enable_if<is_iterable<Container>::value, void>::type
    erase_remove_if(Container& c, UnaryPredicate p)
    {
        auto iter = std::remove_if(c.begin(), c.end(), p);

        if (iter == c.end())
            return;

        c.erase(iter, c.end());
    }

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, void>::type
    erase_at_index(Container& c, const std::size_t index)
    {
        if (index < c.size())
            c.erase(c.begin() + index);
    }

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, void>::type
    append(Container& c1, Container const& c2)
    {
        c1.reserve(c1.size() + c2.size());
        c1.insert(c1.end(), c2.begin(), c2.end());
    }

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, const bool>::type
    contains(Container const& c, typename Container::value_type const& t)
    {
        return std::find(c.begin(), c.end(), t) != c.end();
    }

    template <typename K, typename V>
    std::optional<V>
    retrieve(std::unordered_map<K, V>& c, K& key)
    {
        typename std::unordered_map<K, V>::iterator iter = c.find(key);

        if (iter == c.end())
            return std::nullopt;

        return iter->second;
    }

    template <typename K, typename V>
    std::optional<const V>
    const_retrieve(std::unordered_map<K, V> const& c, K const& key)
    {
        typename std::unordered_map<K, V>::const_iterator iter = c.find(key);

        if (iter == c.end())
            return std::nullopt;

        return iter->second;
    }

    template <typename K, typename V>
    V&
    at(std::unordered_map<K, V>& c, K& key)
    {
        return c.at(key);
    }

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, const std::optional<const std::size_t>>::type
    index_of(Container const& c, typename Container::value_type const& t)
    {
        auto iter = std::find(c.begin(), c.end(), t);
        if (iter != c.end())
            return iter - c.begin();

        return std::nullopt;
    }

    template <typename Container>
    typename std::enable_if<is_iterable<Container>::value, const std::size_t>::type
    last_index(Container const& c)
    {
        return c.empty() ? 0 : c.size() - 1;
    }

    // https://stackoverflow.com/a/28139075
    template <typename T>
    struct reversion_wrapper { T& iterable; };

    template <typename T>
    auto begin(reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

    template <typename T>
    auto end(reversion_wrapper<T> w)
    {
        return std::rend(w.iterable);
    }

    template <typename T>
    reversion_wrapper<T> reverse(T&& iterable)
    {
        return { iterable };
    }

}
