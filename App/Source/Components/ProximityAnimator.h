#pragma once
#include "Mupfel.h"
#include <optional>

struct ProximityAnimator
{
    Mupfel::Animation on_enter;
    Mupfel::Animation on_exit;
    uint32_t current_visitors = 0;
    std::optional<Mupfel::Animation> pending;

    void BuildFromJSON(const nlohmann::json &source);
};