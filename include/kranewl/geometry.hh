#pragma once

#include <ostream>
#include <sstream>

extern "C" {
#include <wlr/util/box.h>
}

struct Dim final {
    int w;
    int h;
};

inline bool
operator==(Dim const& lhs, Dim const& rhs)
{
    return lhs.w == rhs.w && lhs.h == rhs.h;
}

inline std::ostream&
operator<<(std::ostream& os, Dim const& dim)
{
    return os << "(" << dim.w << "×" << dim.h << ")";
}

struct DDim final {
    double w;
    double h;
};

inline std::ostream&
operator<<(std::ostream& os, DDim const& dim)
{
    return os << "(" << dim.w << "×" << dim.h << ")";
}

struct Pos final {
    int x;
    int y;

    static Pos
    from_center_of_dim(Dim dim) noexcept
    {
        return Pos {
            static_cast<int>(static_cast<float>(dim.w) / 2.f),
            static_cast<int>(static_cast<float>(dim.h) / 2.f)
        };
    }

    static bool
    is_at_origin(Pos& pos) noexcept
    {
        return pos.x == 0 && pos.y == 0;
    }
};

inline bool
operator==(Pos const& lhs, Pos const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline std::ostream&
operator<<(std::ostream& os, Pos const& pos)
{
    return os << "(" << pos.x << ", " << pos.y << ")";
}

inline Pos
operator+(Pos const& pos1, Pos const& pos2)
{
    return Pos{
        pos1.x + pos2.x,
        pos1.y + pos2.y
    };
}

inline Pos
operator-(Pos const& pos1, Pos const& pos2)
{
    return Pos{
        pos1.x - pos2.x,
        pos1.y - pos2.y
    };
}

inline Pos
operator+(Pos const& pos, Dim const& dim)
{
    return Pos{
        pos.x + dim.w,
        pos.y + dim.h
    };
}

inline Pos
operator-(Pos const& pos, Dim const& dim)
{
    return Pos{
        pos.x - dim.w,
        pos.y - dim.h
    };
}

struct DPos final {
    double x;
    double y;

    static DPos
    from_center_of_dim(DDim dim) noexcept
    {
        return DPos {
            dim.w / 2.0,
            dim.h / 2.0
        };
    }
};

inline std::ostream&
operator<<(std::ostream& os, DPos const& pos)
{
    return os << "(" << pos.x << ", " << pos.y << ")";
}

inline DPos
operator+(DPos const& pos1, DPos const& pos2)
{
    return DPos{
        pos1.x + pos2.x,
        pos1.y + pos2.y
    };
}

inline DPos
operator-(DPos const& pos1, DPos const& pos2)
{
    return DPos{
        pos1.x - pos2.x,
        pos1.y - pos2.y
    };
}

inline DPos
operator+(DPos const& pos, DDim const& dim)
{
    return DPos{
        pos.x + dim.w,
        pos.y + dim.h
    };
}

inline DPos
operator-(DPos const& pos, DDim const& dim)
{
    return DPos{
        pos.x - dim.w,
        pos.y - dim.h
    };
}

struct Padding final {
    int left;
    int right;
    int top;
    int bottom;
};

typedef Padding Extents;

inline std::ostream&
operator<<(std::ostream& os, Padding const& padding)
{
    return os << "[" << padding.left
        << "; " << padding.top
        << "; " << padding.right
        << "; " << padding.bottom << "]";
}

struct Region final {
    Pos pos;
    Dim dim;

    static inline Region
    from_box(struct wlr_box box)
    {
        return Region{
            .pos = {
                .x = box.x,
                .y = box.y
            },
            .dim = {
                .w = box.width,
                .h = box.height
            }
        };
    }

    operator struct wlr_box() const
    {
        struct wlr_box box;

        box.x = this->pos.x;
        box.y = this->pos.y;
        box.width = this->dim.w;
        box.height = this->dim.h;

        return box;
    }

    void apply_minimum_dim(Dim const&);
    void apply_extents(Extents const&);
    void remove_extents(Extents const&);

    bool contains(Pos) const;
    bool contains(Region const&) const;

    Pos center() const;
};

inline bool
operator==(Region const& lhs, Region const& rhs)
{
    return lhs.pos == rhs.pos && lhs.dim == rhs.dim;
}

inline std::ostream&
operator<<(std::ostream& os, Region const& region)
{
    return os << "[" << region.pos << " " << region.dim << "]";
}

namespace std {

    inline std::string
    to_string(Region const& region) {
        std::ostringstream oss;
        oss << region;
        return oss.str();
    }

}

struct DRegion final {
    DPos pos;
    DDim dim;

    void apply_minimum_dim(DDim const&);
    void apply_extents(Extents const&);
    void remove_extents(Extents const&);

    bool contains(DPos) const;
    bool contains(DRegion const&) const;

    Pos center() const;
};

inline std::ostream&
operator<<(std::ostream& os, DRegion const& region)
{
    return os << "[" << region.pos << " " << region.dim << "]";
}

struct Distance final {
    int dx;
    int dy;
};

inline std::ostream&
operator<<(std::ostream& os, Distance const& dist)
{
    return os << "𝛿(" << dist.dx << ", " << dist.dy << ")";
}

struct DDistance final {
    double dx;
    double dy;
};

inline std::ostream&
operator<<(std::ostream& os, DDistance const& dist)
{
    return os << "𝛿(" << dist.dx << ", " << dist.dy << ")";
}
