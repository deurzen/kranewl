#include <kranewl/tree/view.hh>

View::View(XDGView_ptr)
    : m_type(Type::XDGShell)
{}

#if XWAYLAND
View::View(XWaylandView_ptr)
    : m_type(Type::XWayland)
{}
#endif

View::~View()
{}

ViewChild::ViewChild(SubsurfaceViewChild_ptr)
    : m_type(Type::Subsurface)
{}

ViewChild::ViewChild(PopupViewChild_ptr)
    : m_type(Type::Popup)
{}

ViewChild::~ViewChild()
{}
