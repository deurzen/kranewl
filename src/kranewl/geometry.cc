#include <kranewl/geometry.hh>

void
Region::apply_minimum_dim(Dim const& dim)
{
    this->dim.w = std::max(this->dim.w, dim.w);
    this->dim.h = std::max(this->dim.h, dim.h);
}

void
Region::apply_extents(Extents const& extents)
{
    pos.x -= extents.left;
    pos.y -= extents.top;
    dim.w += extents.left + extents.right;
    dim.h += extents.top + extents.bottom;
}

void
Region::remove_extents(Extents const& extents)
{
    pos.x += extents.left;
    pos.y += extents.top;
    dim.w -= extents.left + extents.right;
    dim.h -= extents.top + extents.bottom;
}

bool
Region::contains(Pos pos) const
{
    return pos.x >= this->pos.x
        && pos.y >= this->pos.y
        && pos.x < this->pos.x + this->dim.w
        && pos.y < this->pos.y + this->dim.h;
}

bool
Region::contains(Region const& region) const
{
    return contains(region.pos)
        && contains(region.pos + region.dim);
}

Pos
Region::center() const
{
    return pos + Pos::from_center_of_dim(dim);
}
