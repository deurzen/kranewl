#pragma once

#include <kranewl/common.hh>

#include <string>
#include <sstream>

typedef struct Node {
    enum class Type {
        XDGShell,
        LayerShell,
#ifdef XWAYLAND
        XWaylandManaged,
        XWaylandUnmanaged,
#endif
    };

    virtual bool
    is_focusable() const
    {
#ifdef XWAYLAND
        return m_type == Type::XDGShell
            || m_type == Type::XWaylandManaged;
#else
        return m_type == Type::XDGShell;
#endif
    }

    virtual bool
    is_view() const
    {
#ifdef XWAYLAND
        return m_type != Type::LayerShell
            && m_type != Type::XWaylandUnmanaged;
#else
        return m_type != Type::LayerShell;
#endif
    }

    virtual Uid uid() const { return m_uid; }
    virtual std::string const& uid_formatted() const { return m_uid_formatted; }

    virtual void format_uid() = 0;

protected:
    Node(Type type, Uid uid)
        : m_type(type),
          m_uid(uid),
          m_uid_formatted([uid]() {
              std::stringstream uid_ss;
              uid_ss << "0x" << std::hex << uid;
              return uid_ss.str();
          }())
    {}

    ~Node()
    {}

    Type m_type;
    Uid m_uid;
    std::string m_uid_formatted;

}* Node_ptr;
