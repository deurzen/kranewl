#include <trace.hh>

#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/layer.hh>
#include <kranewl/tree/output.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
}
#undef static
#undef namespace
#undef class

Layer::Layer(
    struct wlr_layer_surface_v1* layer_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    Output_ptr output,
    SceneLayer scene_layer
)
    : Node(Type::LayerShell, reinterpret_cast<std::uintptr_t>(layer_surface)),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(output),
      m_scene_layer(scene_layer),
      mp_layer_surface(layer_surface),
      ml_map({ .notify = Layer::handle_map }),
      ml_unmap({ .notify = Layer::handle_unmap }),
      ml_surface_commit({ .notify = Layer::handle_surface_commit }),
      ml_destroy({ .notify = Layer::handle_destroy }),
      m_mapped(0),
      m_managed_since(std::chrono::steady_clock::now())
{
    wl_signal_add(&mp_layer_surface->events.map, &ml_map);
    wl_signal_add(&mp_layer_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_layer_surface->surface->events.commit, &ml_surface_commit);
    wl_signal_add(&mp_layer_surface->events.destroy, &ml_destroy);
}

Layer::~Layer()
{}

void
Layer::format_uid()
{
    std::stringstream uid_ss;
    uid_ss << "0x" << std::hex << uid() << std::dec;
    uid_ss << " [" << m_pid << "]";
    uid_ss << " (L)";
    m_uid_formatted = uid_ss.str();
}

pid_t
Layer::pid()
{
    TRACE();

    pid_t pid;
    struct wl_client* client
        = wl_resource_get_client(mp_layer_surface->surface->resource);

    wl_client_get_credentials(client, &pid, nullptr, nullptr);
    return pid;
}

void
Layer::set_mapped(int mapped)
{
    m_mapped = mapped;
}

void
Layer::set_region(Region const& region)
{
    m_region = region;
}

void
Layer::handle_map(struct wl_listener* listener, void*)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_map);

    layer->m_pid = layer->pid();
    layer->format_uid();

    layer->mp_model->register_layer(layer);

	struct wlr_layer_surface_v1_state initial_state = layer->mp_layer_surface->current;
	layer->mp_layer_surface->current = layer->mp_layer_surface->pending;
    layer->mp_output->arrange_layers();
	layer->mp_layer_surface->current = initial_state;

    wlr_surface_send_enter(
        layer->mp_layer_surface->surface,
        layer->mp_layer_surface->output
    );

    if (layer->mp_seat->mp_cursor)
        layer->mp_seat->mp_cursor->process_cursor_motion(0);
}

static inline void
unmap_layer(Layer_ptr layer)
{
    TRACE();

    layer->mp_layer_surface->mapped = 0;
    layer->set_mapped(0);

    struct wlr_seat* seat = layer->mp_seat->mp_wlr_seat;

    if (layer->mp_layer_surface->surface == seat->keyboard_state.focused_surface)
        layer->mp_model->refocus();

    if (layer->mp_seat->mp_cursor)
        layer->mp_seat->mp_cursor->process_cursor_motion(0);
}

void
Layer::handle_unmap(struct wl_listener* listener, void*)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_unmap);
    unmap_layer(layer);
}

void
Layer::handle_surface_commit(struct wl_listener* listener, void*)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_surface_commit);

    struct wlr_layer_surface_v1* layer_surface = layer->mp_layer_surface;
    struct wlr_output* wlr_output = layer_surface->output;

    SceneLayer scene_layer;
    switch (layer_surface->current.layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
        scene_layer = SCENE_LAYER_BACKGROUND;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
        scene_layer = SCENE_LAYER_BOTTOM;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
        scene_layer = SCENE_LAYER_TOP;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
        scene_layer = SCENE_LAYER_OVERLAY;
        break;
    default:
        spdlog::error("No applicable scene layer found for layer surface");
        spdlog::warn("Not committing surface");
        scene_layer = SCENE_LAYER_NONE;
        return;
    }

    wlr_scene_node_reparent(
        layer->mp_scene,
        layer->mp_server->m_scene_layers[scene_layer]
    );

    Output_ptr output;
    if (!wlr_output || !(output = reinterpret_cast<Output_ptr>(wlr_output->data)))
        return;

    if (layer_surface->current.committed == 0 && layer->m_mapped == layer_surface->mapped)
        return;

    layer->m_mapped = layer_surface->mapped;

    if (layer->mp_server->m_scene_layers[scene_layer] != layer->mp_scene)
        output->relayer_layer(layer, layer->m_scene_layer, scene_layer);

    output->arrange_layers();
}

void
Layer::handle_destroy(struct wl_listener* listener, void*)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_destroy);
    struct wlr_output* wlr_output = layer->mp_layer_surface->output;

    if (layer->mp_layer_surface->mapped)
        unmap_layer(layer);

    wl_list_remove(&layer->ml_map.link);
    wl_list_remove(&layer->ml_unmap.link);
    wl_list_remove(&layer->ml_surface_commit.link);
    wl_list_remove(&layer->ml_destroy.link);

    Output_ptr output;
    if (wlr_output && (output = reinterpret_cast<Output_ptr>(wlr_output->data))) {
        output->remove_layer(layer);
        output->arrange_layers();
        layer->mp_layer_surface->output = nullptr;
    }

    spdlog::info("Destroyed layer {}", layer->m_uid_formatted);
    delete layer;
}
