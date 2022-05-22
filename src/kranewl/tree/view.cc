#include <kranewl/tree/view.hh>

View::View(
    XDGView_ptr,
    Uid uid,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace,
    void(*handle_foreign_activate_request)(wl_listener*, void*),
    void(*handle_foreign_fullscreen_request)(wl_listener*, void*),
    void(*handle_foreign_close_request)(wl_listener*, void*),
    void(*handle_foreign_destroy)(wl_listener*, void*),
    void(*handle_surface_new_subsurface)(wl_listener*, void*)
)
    : m_uid(uid),
      m_type(Type::XDGShell),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(output),
      mp_context(context),
      mp_workspace(workspace),
      ml_foreign_activate_request({ .notify = handle_foreign_activate_request }),
      ml_foreign_fullscreen_request({ .notify = handle_foreign_fullscreen_request }),
      ml_foreign_close_request({ .notify = handle_foreign_close_request }),
      ml_foreign_destroy({ .notify = handle_foreign_destroy }),
      ml_surface_new_subsurface({ .notify = handle_surface_new_subsurface })
{
    wl_signal_init(&m_events.unmap);
}

#if XWAYLAND
View::View(
    XWaylandView_ptr,
    Uid uid,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace,
    void(*handle_foreign_activate_request)(wl_listener*, void*),
    void(*handle_foreign_fullscreen_request)(wl_listener*, void*),
    void(*handle_foreign_close_request)(wl_listener*, void*),
    void(*handle_foreign_destroy)(wl_listener*, void*),
    void(*handle_surface_new_subsurface)(wl_listener*, void*)
)
    : m_uid(uid),
      m_type(Type::XWayland),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(output),
      mp_context(context),
      mp_workspace(workspace),
      ml_foreign_activate_request({ .notify = handle_foreign_activate_request }),
      ml_foreign_fullscreen_request({ .notify = handle_foreign_fullscreen_request }),
      ml_foreign_close_request({ .notify = handle_foreign_close_request }),
      ml_foreign_destroy({ .notify = handle_foreign_destroy }),
      ml_surface_new_subsurface({ .notify = handle_surface_new_subsurface })
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
