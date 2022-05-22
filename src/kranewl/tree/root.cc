#include <trace.hh>

#include <kranewl/tree/root.hh>
#include <kranewl/cycle.t.hh>

extern "C" {
#include <wlr/types/wlr_output_layout.h>
}

Root::Root(
    Server_ptr server,
    Model_ptr model,
    struct wlr_output_layout* wlr_output_layout
)
    : Node(this),
      mp_server(server),
      mp_model(model),
      mp_output_layout(wlr_output_layout),
      ml_output_layout_change({ .notify = Root::handle_output_layout_change }),
      m_outputs({}, true),
      m_scratchpad({}, true)
{
    TRACE();

    wl_signal_add(&mp_output_layout->events.change, &ml_output_layout_change);
    wl_signal_init(&m_events.new_node);
}

Root::~Root()
{}

void
Root::handle_output_layout_change(struct wl_listener*, void*)
{
    TRACE();

}
