#pragma once

namespace engine::render {
    struct Render {
        virtual void begin() = 0;
        virtual void end() = 0;
        
        virtual ~Render() = default;
    };

    Render *getRender();
}
