#include <trace.hh>

#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/workspace.hh>
#include <kranewl/tree/output.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
}
#undef static
#undef class
#undef namespace

Output::Output(
    Server_ptr server,
    Model_ptr model,
    struct wlr_output* wlr_output,
    struct wlr_scene_output* wlr_scene_output,
    Region const&& output_region
)
    : Node(this),
      mp_context(nullptr),
      m_full_region(output_region),
      m_placeable_region(output_region),
      mp_server(server),
      mp_model(model),
      m_dirty(true),
      m_cursor_focus_on_present(false),
      mp_wlr_output(wlr_output),
      mp_wlr_scene_output(wlr_scene_output),
      ml_frame({ .notify = Output::handle_frame }),
      ml_commit({ .notify = Output::handle_commit }),
      ml_present({ .notify = Output::handle_present }),
      ml_mode({ .notify = Output::handle_mode }),
      ml_destroy({ .notify = Output::handle_destroy })
{
    TRACE();

    wl_signal_add(&mp_wlr_output->events.frame, &ml_frame);
    wl_signal_add(&mp_wlr_output->events.commit, &ml_commit);
    wl_signal_add(&mp_wlr_output->events.present, &ml_present);
    wl_signal_add(&mp_wlr_output->events.mode, &ml_mode);
    wl_signal_add(&mp_wlr_output->events.destroy, &ml_destroy);

    wl_signal_init(&m_events.disable);
}

Output::~Output()
{}

void
Output::handle_frame(struct wl_listener* listener, void*)
{
    TRACE();

    Output_ptr output = wl_container_of(listener, output, ml_frame);

    if (!wlr_scene_output_commit(output->mp_wlr_scene_output))
        return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->mp_wlr_scene_output, &now);
}

void
Output::handle_commit(struct wl_listener*, void*)
{
    TRACE();

}

void
Output::handle_present(struct wl_listener* listener, void*)
{
    TRACE();

    Output_ptr output = wl_container_of(listener, output, ml_present);

    if (output->m_cursor_focus_on_present && output == output->mp_model->mp_output) {
        if (true /* TODO: focus_follows_mouse */) {
            View_ptr view_under_cursor
                = output->mp_server->m_seat.mp_cursor->view_under_cursor();

            if (view_under_cursor)
                output->mp_model->focus_view(view_under_cursor);
        }

        output->m_cursor_focus_on_present = false;
    }
}

void
Output::handle_mode(struct wl_listener*, void*)
{
    TRACE();

}

void
Output::handle_destroy(struct wl_listener*, void* data)
{
    TRACE();

    struct wlr_output* wlr_output = reinterpret_cast<struct wlr_output*>(data);
    Output_ptr output = reinterpret_cast<Output_ptr>(wlr_output->data);

    wl_list_remove(&output->ml_frame.link);
    wl_list_remove(&output->ml_destroy.link);
    wl_list_remove(&output->ml_present.link);
    wl_list_remove(&output->ml_mode.link);
    wl_list_remove(&output->ml_commit.link);

    wlr_scene_output_destroy(output->mp_wlr_scene_output);
    wlr_output_layout_remove(output->mp_server->mp_output_layout, output->mp_wlr_output);

    output->mp_model->unregister_output(output);
}

void
Output::set_context(Context_ptr context)
{
    TRACE();

    if (!context)
        spdlog::error("Output must contain a valid context");

    if (mp_context)
        mp_context->set_output(nullptr);

    context->set_output(this);
    mp_context = context;
}

Context_ptr
Output::context() const
{
    return mp_context;
}

Region
Output::full_region() const
{
    return m_full_region;
}

Region
Output::placeable_region() const
{
    return m_placeable_region;
}

bool
Output::contains(Pos pos) const
{
    return m_full_region.contains(pos);
}

bool
Output::contains(Region region) const
{
    return m_full_region.contains(region);
}

void
Output::focus_at_cursor()
{
    m_cursor_focus_on_present = true;
}
