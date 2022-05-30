#pragma once

typedef struct Node {
    enum class Type {
        XDGShell,
        LayerShell,
#ifdef XWAYLAND
        XWaylandManaged,
        XWaylandUnmanaged,
#endif
    };

    virtual bool is_focusable() const
    {
#ifdef XWAYLAND
        return m_type == Type::XDGShell
            || m_type == Type::XWaylandManaged;
#else
        return m_type == Type::XDGShell;
#endif
    }

protected:
    Node(Type type)
        : m_type(type)
    {}

    ~Node()
    {}

    Type m_type;

}* Node_ptr;
