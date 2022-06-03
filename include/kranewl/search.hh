#pragma once

#include <kranewl/common.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/workspace.hh>

#include <functional>
#include <string>
#include <utility>

typedef class SearchSelector final {
public:
    enum class SelectionCriterium {
        OnWorkspaceBySelector,
        ByTitleEquals,
        ByAppIdEquals,
        ByHandleEquals,
        ByTitleContains,
        ByAppIdContains,
        ByHandleContains,
        ForCondition,
    };

    SearchSelector(Index index, Workspace::ViewSelector::SelectionCriterium criterium)
        : m_tag(SearchSelectorTag::OnWorkspaceBySelector),
          m_workspace_selector(std::pair(index, criterium))
    {}

    SearchSelector(SelectionCriterium criterium, std::string&& str_)
        : m_string(str_)
    {
        switch (criterium) {
        case SelectionCriterium::ByTitleEquals:    m_tag = SearchSelectorTag::ByTitleEquals;    return;
        case SelectionCriterium::ByAppIdEquals:    m_tag = SearchSelectorTag::ByAppIdEquals;    return;
        case SelectionCriterium::ByHandleEquals:   m_tag = SearchSelectorTag::ByHandleEquals;   return;
        case SelectionCriterium::ByTitleContains:  m_tag = SearchSelectorTag::ByTitleContains;  return;
        case SelectionCriterium::ByAppIdContains:  m_tag = SearchSelectorTag::ByAppIdContains;  return;
        case SelectionCriterium::ByHandleContains: m_tag = SearchSelectorTag::ByHandleContains; return;
        default: return;
        }
    }

    SearchSelector(std::function<bool(const View_ptr)>&& filter)
        : m_tag(SearchSelectorTag::ForCondition),
          m_filter(filter)
    {}

    ~SearchSelector()
    {
        switch (m_tag) {
        case SearchSelectorTag::OnWorkspaceBySelector:
        {
            (&m_workspace_selector)->std::pair<
                    Index,
                    Workspace::ViewSelector::SelectionCriterium
            >::pair::~pair();

            return;
        }
        case SearchSelectorTag::ByTitleEquals:    // fallthrough
        case SearchSelectorTag::ByAppIdEquals:    // fallthrough
        case SearchSelectorTag::ByHandleEquals:   // fallthrough
        case SearchSelectorTag::ByTitleContains:  // fallthrough
        case SearchSelectorTag::ByAppIdContains:  // fallthrough
        case SearchSelectorTag::ByHandleContains:
        {
            (&m_string)->std::string::~string();

            return;
        }
        case SearchSelectorTag::ForCondition:
        {
            (&m_filter)->std::function<bool(const View_ptr)>::function::~function();

            return;
        }
        default: return;
        }
    }

    SearchSelector(SearchSelector&&) = delete;

    SelectionCriterium
    criterium() const
    {
        switch (m_tag) {
        case SearchSelectorTag::OnWorkspaceBySelector: return SelectionCriterium::OnWorkspaceBySelector;
        case SearchSelectorTag::ByTitleEquals:         return SelectionCriterium::ByTitleEquals;
        case SearchSelectorTag::ByAppIdEquals:         return SelectionCriterium::ByAppIdEquals;
        case SearchSelectorTag::ByHandleEquals:        return SelectionCriterium::ByAppIdEquals;
        case SearchSelectorTag::ByTitleContains:       return SelectionCriterium::ByTitleContains;
        case SearchSelectorTag::ByAppIdContains:       return SelectionCriterium::ByAppIdContains;
        case SearchSelectorTag::ByHandleContains:      return SelectionCriterium::ByHandleContains;
        case SearchSelectorTag::ForCondition:          return SelectionCriterium::ForCondition;
        default: break;
        }

        return {};
    }

    std::pair<Index, Workspace::ViewSelector::SelectionCriterium> const&
    workspace_selector() const
    {
        return m_workspace_selector;
    }

    std::string const&
    string_value() const
    {
        return m_string;
    }

    std::function<bool(const View_ptr)> const&
    filter() const
    {
        return m_filter;
    }

private:
    enum class SearchSelectorTag {
        OnWorkspaceBySelector,
        ByTitleEquals,
        ByAppIdEquals,
        ByHandleEquals,
        ByTitleContains,
        ByAppIdContains,
        ByHandleContains,
        ForCondition,
    };

    SearchSelectorTag m_tag;
    union {
        std::pair<Index, Workspace::ViewSelector::SelectionCriterium> m_workspace_selector;
        std::string m_string;
        std::function<bool(const View_ptr)> m_filter;
    };

}* SearchSelector_ptr;
