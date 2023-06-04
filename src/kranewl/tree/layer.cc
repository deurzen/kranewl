#include <trace.hh>

#include <kranewl/manager.hh>
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
#include <wlr/types/wlr_xdg_shell.h>
}
#undef static
#undef namespace
#undef class

Layer::Layer(
    struct wlr_layer_surface_v1* layer_surface,
    Server_ptr server,
    Manager_ptr manager,
    Seat_ptr seat,
    Output_ptr output,
    SceneLayer scene_layer
)
    : Node(Type::LayerShell, reinterpret_cast<std::uintptr_t>(layer_surface)),
      mp_server(server),
      mp_manager(manager),
      mp_seat(seat),
      mp_output(output),
      m_scene_layer(scene_layer),
      mp_layer_surface(layer_surface),
      ml_map({ .notify = Layer::handle_map }),
      ml_unmap({ .notify = Layer::handle_unmap }),
      ml_surface_commit({ .notify = Layer::handle_surface_commit }),
      ml_new_popup({ .notify = Layer::handle_new_popup }),
      ml_destroy({ .notify = Layer::handle_destroy }),
      m_mapped(0),
      m_managed_since(std::chrono::steady_clock::now())
{
    wl_signal_add(&mp_layer_surface->events.map, &ml_map);
    wl_signal_add(&mp_layer_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_layer_surface->surface->events.commit, &ml_surface_commit);
    wl_signal_add(&mp_layer_surface->events.new_popup, &ml_new_popup);
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
Layer::handle_map(struct wl_listener* listener, void*)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_map);

    layer->m_pid = layer->pid();
    layer->format_uid();

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
    struct wlr_seat* seat = layer->mp_seat->mp_wlr_seat;

    if (layer->mp_layer_surface->surface == seat->keyboard_state.focused_surface)
        layer->mp_manager->refocus();

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

    switch (layer_surface->current.layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
        layer->m_scene_layer = SCENE_LAYER_BACKGROUND;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
        layer->m_scene_layer = SCENE_LAYER_BOTTOM;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
        layer->m_scene_layer = SCENE_LAYER_TOP;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
        layer->m_scene_layer = SCENE_LAYER_OVERLAY;
        break;
    default:
        spdlog::error("No applicable scene layer found for layer surface");
        spdlog::warn("Not committing surface");
        layer->m_scene_layer = SCENE_LAYER_NONE;
        return;
    }

    wlr_scene_node_reparent(
        &layer->mp_scene->node,
        layer->mp_server->m_scene_layers[layer->m_scene_layer]
    );

    Output_ptr output;
    if (!wlr_output || !(output = reinterpret_cast<Output_ptr>(wlr_output->data)))
        return;

    if (layer_surface->current.committed == 0 && layer->m_mapped == layer_surface->mapped)
        return;

    layer->m_mapped = layer_surface->mapped;

    if (layer->mp_server->m_scene_layers[layer->m_scene_layer] != layer->mp_scene)
        output->relayer_layer(layer, layer->m_scene_layer, layer->m_scene_layer);

    output->arrange_layers();
}

static inline LayerPopup_ptr
create_layer_popup(
    struct wlr_xdg_popup* wlr_popup,
    LayerPopup_ptr parent,
    Layer_ptr root,
    Server_ptr server,
    Manager_ptr manager,
    Seat_ptr seat
)
{
    LayerPopup_ptr popup = parent
        ? new LayerPopup(wlr_popup, parent, root, server, manager, seat)
        : new LayerPopup(wlr_popup, root, server, manager, seat);

    if (parent)
        parent->m_popups.push_back(popup);
    else
        root->m_popups.push_back(popup);

    struct wlr_output* wlr_output = root->mp_layer_surface->output;
    Output_ptr output = reinterpret_cast<Output_ptr>(wlr_output->data);

    Region const& output_region = output->full_region();
    Region const& relative_region = parent
        ? parent->region()
        : root->region();

    struct wlr_box mappable_region = {
        .x = -relative_region.pos.x,
        .y = -relative_region.pos.y,
        .width = output_region.dim.w,
        .height = output_region.dim.h
    };

    wlr_xdg_popup_unconstrain_from_box(wlr_popup, &mappable_region);

    return popup;
}

void
Layer::handle_new_popup(struct wl_listener* listener, void* data)
{
    TRACE();

    Layer_ptr layer = wl_container_of(listener, layer, ml_new_popup);
    struct wlr_xdg_popup* wlr_popup = reinterpret_cast<struct wlr_xdg_popup*>(data);

    create_layer_popup(
        wlr_popup,
        nullptr,
        layer,
        layer->mp_server,
        layer->mp_manager,
        layer->mp_seat
    );
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

static inline void
init_layer_popup(LayerPopup_ptr popup)
{
    TRACE();

    wl_signal_add(&popup->mp_wlr_popup->base->events.map, &popup->ml_map);
    wl_signal_add(&popup->mp_wlr_popup->base->events.unmap, &popup->ml_unmap);
    wl_signal_add(&popup->mp_wlr_popup->base->surface->events.commit, &popup->ml_surface_commit);
    wl_signal_add(&popup->mp_wlr_popup->base->events.new_popup, &popup->ml_new_popup);
    wl_signal_add(&popup->mp_wlr_popup->base->events.destroy, &popup->ml_destroy);
}

LayerPopup::LayerPopup(
    struct wlr_xdg_popup* wlr_popup,
    Layer_ptr parent,
    Server_ptr server,
    Manager_ptr manager,
    Seat_ptr seat
)
    : mp_server(server),
      mp_manager(manager),
      mp_seat(seat),
      mp_wlr_popup(wlr_popup),
      mp_parent(nullptr),
      mp_root(parent),
      m_popups({}),
      ml_map({ .notify = LayerPopup::handle_map }),
      ml_unmap({ .notify = LayerPopup::handle_unmap }),
      ml_surface_commit({ .notify = LayerPopup::handle_surface_commit }),
      ml_new_popup({ .notify = LayerPopup::handle_new_popup }),
      ml_destroy({ .notify = LayerPopup::handle_destroy })
{
    TRACE();
    init_layer_popup(this);
}

LayerPopup::LayerPopup(
    struct wlr_xdg_popup* wlr_popup,
    LayerPopup_ptr parent,
    Layer_ptr root,
    Server_ptr server,
    Manager_ptr manager,
    Seat_ptr seat
)
    : mp_server(server),
      mp_manager(manager),
      mp_seat(seat),
      mp_wlr_popup(wlr_popup),
      mp_parent(parent),
      mp_root(root),
      m_popups({}),
      ml_map({ .notify = LayerPopup::handle_map }),
      ml_unmap({ .notify = LayerPopup::handle_unmap }),
      ml_surface_commit({ .notify = LayerPopup::handle_surface_commit }),
      ml_new_popup({ .notify = LayerPopup::handle_new_popup }),
      ml_destroy({ .notify = LayerPopup::handle_destroy })
{
    TRACE();
    init_layer_popup(this);
}

LayerPopup::~LayerPopup()
{

}

void
LayerPopup::set_region(Region const& region)
{
    m_region = region;
}

void
LayerPopup::handle_map(struct wl_listener* listener, void*)
{
    TRACE();

    LayerPopup_ptr popup = wl_container_of(listener, popup, ml_map);
    struct wlr_xdg_popup* wlr_popup = popup->mp_wlr_popup;
    Layer_ptr root = popup->mp_root;
    LayerPopup_ptr parent = popup->mp_parent;
    Server_ptr server = root->mp_server;

    SceneLayer reference_scene_layer = parent
        ? parent->m_scene_layer
        : root->m_scene_layer;

    popup->m_scene_layer = SceneLayer::SCENE_LAYER_POPUP;
    popup->mp_scene = wlr_scene_tree_create(
        server->m_scene_layers[popup->m_scene_layer]
    );
    wlr_popup->base->surface->data = popup->mp_scene
        = wlr_scene_xdg_surface_create(
            server->m_scene_layers[popup->m_scene_layer],
            wlr_popup->base
        );
    popup->mp_scene->node.data = popup;

    wlr_scene_node_reparent(
        &popup->mp_scene->node,
        server->m_scene_layers[popup->m_scene_layer]
    );

    Region const& region = popup->region();
    wlr_scene_node_set_position(
        &popup->mp_scene->node,
        region.pos.x,
        region.pos.y
    );

    if (popup->m_scene_layer == reference_scene_layer)
        wlr_scene_node_place_above(
            &popup->mp_scene->node,
            &(parent ? parent->mp_scene : root->mp_scene)->node
        );
}

void
LayerPopup::handle_unmap(struct wl_listener*, void*)
{
    TRACE();

}

void
LayerPopup::handle_surface_commit(struct wl_listener* listener, void*)
{
    TRACE();

    LayerPopup_ptr popup = wl_container_of(listener, popup, ml_surface_commit);

    struct wlr_xdg_popup* wlr_popup = popup->mp_wlr_popup;
    Layer_ptr root = popup->mp_root;
    LayerPopup_ptr parent = popup->mp_parent;

    /* wlr_scene_node_reparent( */
    /*     layer->mp_scene, */
    /*     layer->mp_server->m_scene_layers[scene_layer] */
    /* ); */

    /* Output_ptr output; */
    /* if (!wlr_output || !(output = reinterpret_cast<Output_ptr>(wlr_output->data))) */
    /*     return; */

    /* if (layer_surface->current.committed == 0 && layer->m_mapped == layer_surface->mapped) */
    /*     return; */

    /* layer->m_mapped = layer_surface->mapped; */

    /* if (layer->mp_server->m_scene_layers[scene_layer] != layer->mp_scene) */
    /*     output->relayer_layer(layer, layer->m_scene_layer, scene_layer); */

    /* output->arrange_layers(); */
}

void
LayerPopup::handle_new_popup(struct wl_listener* listener, void* data)
{
    TRACE();

    LayerPopup_ptr popup = wl_container_of(listener, popup, ml_new_popup);
    struct wlr_xdg_popup* wlr_popup = reinterpret_cast<struct wlr_xdg_popup*>(data);

    create_layer_popup(
        wlr_popup,
        popup,
        popup->mp_root,
        popup->mp_server,
        popup->mp_manager,
        popup->mp_seat
    );
}

void
LayerPopup::handle_destroy(struct wl_listener* listener, void*)
{
    TRACE();

    LayerPopup_ptr popup = wl_container_of(listener, popup, ml_destroy);

    wl_list_remove(&popup->ml_map.link);
    wl_list_remove(&popup->ml_unmap.link);
    wl_list_remove(&popup->ml_surface_commit.link);
    wl_list_remove(&popup->ml_new_popup.link);
    wl_list_remove(&popup->ml_destroy.link);

    Util::erase_remove(
        popup->mp_parent
            ? popup->mp_parent->m_popups
            : popup->mp_root->m_popups,
        popup
    );

    delete popup;
}
