#include <kranewl/tree/view.hh>
#include <kranewl/tree/xwayland_view.hh>

#if XWAYLAND
XWaylandView::XWaylandView()
    : View(this)
{

}

XWaylandView::~XWaylandView()
{

}
#endif
