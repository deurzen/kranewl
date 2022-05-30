#pragma once

#include <limits>

enum SceneLayer : unsigned short {
    SCENE_LAYER_NONE       = std::numeric_limits<unsigned short>::max(),
    SCENE_LAYER_BACKGROUND = 0,
    SCENE_LAYER_BOTTOM     = 1,
    SCENE_LAYER_TILE       = 2,
    SCENE_LAYER_FREE       = 3,
    SCENE_LAYER_TOP        = 4,
    SCENE_LAYER_OVERLAY    = 5,
    SCENE_LAYER_NOFOCUS    = 6,
};
