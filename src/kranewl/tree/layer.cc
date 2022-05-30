#include <trace.hh>

#include <kranewl/tree/layer.hh>

// https://github.com/swaywm/wlroots/issues/682
#define namespace namespace_
extern "C" {
#include <wlr/types/wlr_layer_shell_v1.h>
}
#undef namespace

Layer::Layer(
    struct wlr_layer_surface_v1* layer_surface,
    Server_ptr server,
    Model_ptr model,
    Output_ptr output,
    SceneLayer scene_layer
)
    : m_uid(reinterpret_cast<std::uintptr_t>(layer_surface)),
      m_uid_formatted([this]() {
          std::stringstream uid_ss;
          uid_ss << "0x" << std::hex << m_uid;
          return uid_ss.str();
      }()),
      mp_server(server),
      mp_model(model),
      mp_output(output),
      m_scene_layer(scene_layer),
      mp_layer_surface(layer_surface),
      ml_map({ .notify = Layer::handle_map }),
      ml_unmap({ .notify = Layer::handle_unmap }),
      ml_surface_commit({ .notify = Layer::handle_surface_commit }),
      ml_destroy({ .notify = Layer::handle_destroy }),
      m_mapped(false),
      m_managed_since(std::chrono::steady_clock::now())
{
    wl_signal_add(&mp_layer_surface->events.map, &ml_map);
    wl_signal_add(&mp_layer_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_layer_surface->surface->events.new_subsurface, &ml_destroy);
    wl_signal_add(&mp_layer_surface->events.destroy, &ml_destroy);
}

Layer::~Layer()
{}

void
Layer::set_mapped(bool mapped)
{
    m_mapped = mapped;
}

void
Layer::set_region(Region const& region)
{
    m_region = region;
}

void
Layer::handle_map(struct wl_listener*, void*)
{

}

void
Layer::handle_unmap(struct wl_listener*, void*)
{

}

void
Layer::handle_surface_commit(struct wl_listener*, void*)
{

}

void
Layer::handle_destroy(struct wl_listener*, void*)
{

}
