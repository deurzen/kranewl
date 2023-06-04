#include <trace.hh>

#include <kranewl/server.hh>
#include <kranewl/tree/xdg-view.hh>
#include <kranewl/xdg-decoration.hh>

extern "C" {
#include <wlr/types/wlr_xdg_decoration_v1.h>
}

XDGDecoration::XDGDecoration(
    Server_ptr server,
    Manager_ptr manager,
    XDGView_ptr view,
    struct wlr_xdg_toplevel_decoration_v1* wlr_xdg_decoration
)
    : m_uid(reinterpret_cast<std::uintptr_t>(wlr_xdg_decoration->surface)),
      mp_server(server),
      mp_manager(manager),
      mp_view(view),
      mp_wlr_xdg_decoration(wlr_xdg_decoration),
      ml_request_mode({ .notify = XDGDecoration::handle_request_mode }),
      ml_destroy({ .notify = XDGDecoration::handle_destroy })
{}

XDGDecoration::~XDGDecoration()
{}

void
XDGDecoration::handle_request_mode(struct wl_listener* listener, void*)
{
    TRACE();

	XDGDecoration_ptr decoration
        = wl_container_of(listener, decoration, ml_request_mode);
	XDGView_ptr view = decoration->mp_view;

	enum wlr_xdg_toplevel_decoration_v1_mode mode
        = WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
	enum wlr_xdg_toplevel_decoration_v1_mode client_mode
        = decoration->mp_wlr_xdg_decoration->requested_mode;

    // TODO: allow client_mode if floating

    wlr_xdg_toplevel_decoration_v1_set_mode(
        decoration->mp_wlr_xdg_decoration,
        mode
    );
}

void
XDGDecoration::handle_destroy(struct wl_listener* listener, void*)
{
    TRACE();

	XDGDecoration_ptr decoration
        = wl_container_of(listener, decoration, ml_destroy);
	if (decoration->mp_view) {
		decoration->mp_view->mp_decoration = nullptr;
	}

	wl_list_remove(&decoration->ml_destroy.link);
	wl_list_remove(&decoration->ml_request_mode.link);

    decoration->mp_server->m_decorations.erase(decoration->m_uid);
    delete decoration;
}
