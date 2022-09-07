#pragma once

namespace engine::input {
    struct Stick {
        float x;
        float y;
    };

    struct Input {
        Stick lstick;
        Stick rstick;

        float ltrigger;
        float rtrigger;
    };
}
