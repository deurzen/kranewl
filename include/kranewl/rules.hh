#pragma once

#include <kranewl/common.hh>
#include <kranewl/search.hh>

#include <optional>
#include <vector>

extern "C" {
#include <wlr/util/edges.h>
}

struct Rules {
    Rules()
        : do_focus(std::nullopt),
          do_float(std::nullopt),
          do_center(std::nullopt),
          do_fullscreen(std::nullopt),
          to_output(std::nullopt),
          to_context(std::nullopt),
          to_workspace(std::nullopt),
          snap_edges(std::nullopt)
    {}

    ~Rules() = default;

    std::optional<bool> do_focus;
    std::optional<bool> do_float;
    std::optional<bool> do_center;
    std::optional<bool> do_fullscreen;
    std::optional<Index> to_output;
    std::optional<Index> to_context;
    std::optional<Index> to_workspace;
    std::optional<uint32_t> snap_edges;

    static std::vector<std::tuple<SearchSelector_ptr, Rules>>
        compile_default_rules(std::string const&);

    static Rules parse_rules(std::string_view, bool = false);
    static Rules merge_rules(Rules const&, Rules const&);

};
