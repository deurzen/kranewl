#include <kranewl/rules.hh>
#include <kranewl/env.hh>

#include <spdlog/spdlog.h>

#include <fstream>

std::vector<std::tuple<SearchSelector_ptr, Rules>>
Rules::compile_default_rules(std::string const& rules_path)
{
    std::vector<std::tuple<SearchSelector_ptr, Rules>>
        default_rules = {};

    if (!file_exists(rules_path))
        return default_rules;

    std::ifstream rules_if(rules_path);
    if (!rules_if.good())
        return default_rules;

    std::string line;

    while (std::getline(rules_if, line)) {
        std::string::size_type pos = line.find('#');

        if (pos != std::string::npos)
            line = line.substr(0, pos);

        if (line.length() < 5)
            continue;

        line.erase(4, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.length() < 5)
            continue;

        pos = line.find(':');
        std::string rules = line.substr(0, pos);
        if (pos == std::string::npos || rules.empty() || pos >= (line.size() - 1))
            continue;

        line = line.substr(pos + 1);
        if (line.length() < 5)
            continue;

        SearchSelector::SelectionCriterium criterium;
        switch (line[1]) {
        case 'T':
        {
            switch (line[0]) {
            case '=': criterium = SearchSelector::SelectionCriterium::ByTitleEquals;   break;
            case '~': criterium = SearchSelector::SelectionCriterium::ByTitleContains; break;
            default: continue;
            }

            break;
        }
        case 'A':
        {
            switch (line[0]) {
            case '=': criterium = SearchSelector::SelectionCriterium::ByAppIdEquals;   break;
            case '~': criterium = SearchSelector::SelectionCriterium::ByAppIdContains; break;
            default: continue;
            }

            break;
        }
        case 'H':
        {
            switch (line[0]) {
            case '=': criterium = SearchSelector::SelectionCriterium::ByHandleEquals;   break;
            case '~': criterium = SearchSelector::SelectionCriterium::ByHandleContains; break;
            default: continue;
            }

            break;
        }
        default: continue;
        }

        default_rules.push_back({
            new SearchSelector{criterium, line.substr(3)},
            Rules::parse_rules(rules, true)
        });
    }

    return default_rules;
}

Rules
Rules::parse_rules(std::string_view rule, bool ignore_prefix)
{
    static const std::string prefix = "kranewl:";
    static auto snapedge_list_handler = [](Rules& rules, auto iter) {
        switch (*iter) {
        case 'l': *rules.snap_edges |= WLR_EDGE_LEFT;   return;
        case 't': *rules.snap_edges |= WLR_EDGE_TOP;    return;
        case 'r': *rules.snap_edges |= WLR_EDGE_RIGHT;  return;
        case 'b': *rules.snap_edges |= WLR_EDGE_BOTTOM; return;
        default: return;
        }
    };

    Rules rules{};
    std::string_view::const_iterator iter;

    if (!ignore_prefix) {
        std::string::size_type prefix_pos = rule.find(prefix);
        if (prefix_pos == std::string::npos)
            return rules;

        iter = rule.begin() + prefix_pos + prefix.size();
    } else
        iter = rule.begin();

    spdlog::info("Parsing rule: {}", std::string(iter, rule.end()));

    bool invert = false;
    bool next_output = false;
    bool next_context = false;
    bool next_workspace = false;

    std::optional<decltype(snapedge_list_handler)>
        list_handler = std::nullopt;

    for (; iter != rule.end(); ++iter) {
        if (*iter == '.') {
            list_handler = std::nullopt;
            continue;
        }
        if (list_handler) {
            (*list_handler)(rules, iter);
            continue;
        }

        switch (*iter) {
        case ':': return rules;
        case '!': invert = true;                 continue;
        case 'O': next_output = true;            continue;
        case 'C': next_context = true;           continue;
        case 'W': next_workspace = true;         continue;
        case '@': rules.do_focus = !invert;      break;
        case 'f': rules.do_float = !invert;      break;
        case 'F': rules.do_fullscreen = !invert; break;
        case 'c': rules.do_center = !invert;     break;
        case '0': // fallthrough
        case '1': // fallthrough
        case '2': // fallthrough
        case '3': // fallthrough
        case '4': // fallthrough
        case '5': // fallthrough
        case '6': // fallthrough
        case '7': // fallthrough
        case '8': // fallthrough
        case '9':
        {
            if (next_output)
                rules.to_output = *iter - '0';
            if (next_context)
                rules.to_context = *iter - '0';
            if (next_workspace)
                rules.to_workspace = *iter - '0';
            break;
        }
        case 'S':
        {
            if (!rules.snap_edges)
                *rules.snap_edges = WLR_EDGE_NONE;

            list_handler = snapedge_list_handler;
            break;
        }
        default: break;
        }

        invert = false;
        next_output = false;
        next_context = false;
        next_workspace = false;
    }

    return rules;
}


Rules
Rules::merge_rules(Rules const& base, Rules const& merger)
{
    Rules rules = base;

    if (merger.do_focus)
        rules.do_focus = merger.do_focus;
    if (merger.do_float)
        rules.do_float = merger.do_float;
    if (merger.do_center)
        rules.do_center = merger.do_center;
    if (merger.do_fullscreen)
        rules.do_fullscreen = merger.do_fullscreen;
    if (merger.to_output)
        rules.to_output = merger.to_output;
    if (merger.to_context)
        rules.to_context = merger.to_context;
    if (merger.to_workspace)
        rules.to_workspace = merger.to_workspace;
    if (merger.snap_edges)
        rules.snap_edges = merger.snap_edges;

    return rules;
}
