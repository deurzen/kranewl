#pragma once

#include <kranewl/cycle.hh>
#include <kranewl/placement.hh>
#include <kranewl/decoration.hh>
#include <kranewl/util.hh>

#include <vector>
#include <deque>
#include <unordered_map>

class LayoutHandler final {
    typedef std::deque<View_ptr>::const_iterator view_iter;
    typedef std::vector<Placement>& placement_vector;

public:
    enum class LayoutKind {
        /// free layouts
        Float,
        FramelessFloat,
        SingleFloat,
        FramelessSingleFloat,

        // overlapping tiled layouts
        Center,
        Monocle,
        MainDeck,
        StackDeck,
        DoubleDeck,

        // non-overlapping tiled layouts
        Paper,
        CompactPaper,
        DoubleStack,
        CompactDoubleStack,
        HorizontalStack,
        CompactHorizontalStack,
        VerticalStack,
        CompactVerticalStack,
    };

private:
    typedef struct Layout final {
        typedef struct LayoutData final {
            constexpr static int MAX_MAIN_COUNT = 16;
            constexpr static int MAX_GAP_SIZE = 300;
            constexpr static Extents MAX_MARGIN = Extents{700, 700, 400, 400};

            LayoutData(
                Extents margin,
                int gap_size,
                int main_count,
                float main_factor
            )
                : margin(margin),
                  gap_size(gap_size),
                  main_count(main_count),
                  main_factor(main_factor)
            {}

            // generic layout data
            Extents margin;
            int gap_size;

            // tiled layout data
            int main_count;
            float main_factor;
        }* LayoutData_ptr;

    private:
        struct LayoutConfig final
        {
            Placement::PlacementMethod method;
            Decoration decoration;
            bool margin;
            bool gap;
            bool persistent;
            bool single;
            bool wraps;
        };

    public:
        Layout(LayoutKind);
        ~Layout();

        inline bool
        operator==(const Layout& other) const
        {
            return other.kind == kind;
        }

        const LayoutKind kind;
        const LayoutConfig config;

        const LayoutData default_data;
        Cycle<LayoutData_ptr> data;

        static LayoutConfig kind_to_config(LayoutKind kind);
        static LayoutData kind_to_default_data(LayoutKind kind);

    }* Layout_ptr;

public:
    LayoutHandler();
    ~LayoutHandler();

    void arrange(Region, placement_vector, view_iter, view_iter) const;

    LayoutKind kind() const;
    void set_kind(LayoutKind);
    void set_prev_kind();

    bool layout_is_free() const;
    bool layout_has_margin() const;
    bool layout_has_gap() const;
    bool layout_is_persistent() const;
    bool layout_is_single() const;
    bool layout_wraps() const;

    int gap_size() const;
    int main_count() const;
    float main_factor() const;
    Extents margin() const;

    void copy_data_from_prev_layout();
    void set_prev_layout_data();

    void change_gap_size(Util::Change<int>);
    void change_main_count(Util::Change<int>);
    void change_main_factor(Util::Change<float>);
    void change_margin(Util::Change<int>);
    void change_margin(Edge, Util::Change<int>);
    void reset_gap_size();
    void reset_margin();
    void reset_layout_data();
    void cycle_layout_data(Direction);

    void save_layout(int) const;
    void load_layout(int);

private:
    LayoutKind m_kind;
    LayoutKind m_prev_kind;

    std::unordered_map<LayoutKind, Layout_ptr> m_layouts;

    Layout_ptr mp_layout;
    Layout_ptr mp_prev_layout;

    void arrange_float(Region, placement_vector, view_iter, view_iter) const;
    void arrange_frameless_float(Region, placement_vector, view_iter, view_iter) const;
    void arrange_single_float(Region, placement_vector, view_iter, view_iter) const;
    void arrange_frameless_single_float(Region, placement_vector, view_iter, view_iter) const;
    void arrange_center(Region, placement_vector, view_iter, view_iter) const;
    void arrange_monocle(Region, placement_vector, view_iter, view_iter) const;
    void arrange_main_deck(Region, placement_vector, view_iter, view_iter) const;
    void arrange_stack_deck(Region, placement_vector, view_iter, view_iter) const;
    void arrange_double_deck(Region, placement_vector, view_iter, view_iter) const;
    void arrange_paper(Region, placement_vector, view_iter, view_iter) const;
    void arrange_compact_paper(Region, placement_vector, view_iter, view_iter) const;
    void arrange_double_stack(Region, placement_vector, view_iter, view_iter) const;
    void arrange_compact_double_stack(Region, placement_vector, view_iter, view_iter) const;
    void arrange_horizontal_stack(Region, placement_vector, view_iter, view_iter) const;
    void arrange_compact_horizontal_stack(Region, placement_vector, view_iter, view_iter) const;
    void arrange_vertical_stack(Region, placement_vector, view_iter, view_iter) const;
    void arrange_compact_vertical_stack(Region, placement_vector, view_iter, view_iter) const;

};
