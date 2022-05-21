#include <kranewl/tree/node.hh>

Node::Node(Root_ptr root)
    : m_uid(reinterpret_cast<Uid>(root)),
      m_type(NodeType::Root),
      m_root(root),
      m_destroying(false),
      m_dirty(false),
      m_events{ .destroy = {} }
{}

Node::Node(Output_ptr output)
    : m_uid(reinterpret_cast<Uid>(output)),
      m_type(NodeType::Output),
      m_output(output),
      m_destroying(false),
      m_dirty(false),
      m_events{ .destroy = {} }
{}

Node::Node(Context_ptr context)
    : m_uid(reinterpret_cast<Uid>(context)),
      m_type(NodeType::Context),
      m_context(context),
      m_destroying(false),
      m_dirty(false),
      m_events{ .destroy = {} }
{}

Node::Node(Workspace_ptr workspace)
    : m_uid(reinterpret_cast<Uid>(workspace)),
      m_type(NodeType::Workspace),
      m_workspace(workspace),
      m_destroying(false),
      m_dirty(false),
      m_events{ .destroy = {} }
{}

Node::Node(Container_ptr container)
    : m_uid(reinterpret_cast<Uid>(container)),
      m_type(NodeType::Container),
      m_container(container),
      m_destroying(false),
      m_dirty(false),
      m_events{ .destroy = {} }
{}

Node::~Node()
{}
