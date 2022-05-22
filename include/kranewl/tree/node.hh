#pragma once

#include <kranewl/common.hh>

extern "C" {
#include <wayland-server-core.h>
}

typedef class Root* Root_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct Container* Container_ptr;

typedef struct Node {
    enum class Type {
        Root,
        Output,
        Context,
        Workspace,
        Container,
    };

protected:
    Node(Root_ptr);
    Node(Output_ptr);
    Node(Context_ptr);
    Node(Workspace_ptr);
    Node(Container_ptr);
    ~Node();

public:
    Uid m_uid;

    Type m_type;
    union {
        Root_ptr m_root;
        Output_ptr m_output;
        Context_ptr m_context;
        Workspace_ptr m_workspace;
        Container_ptr m_container;
    };

    bool m_destroying;
    bool m_dirty;

    struct {
        struct wl_signal destroy;
    } m_events;

}* Node_ptr;
