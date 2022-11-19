#pragma once

#include "engine/base/container/com/com.h"
#include <xaudio2.h>

#include <span>
#include <vector>

namespace simcoe::audio {
    using WaveData = std::vector<float>;

    struct Audio {
        Audio();
        
        void play(WaveData&& data);
    private:
        com::UniqueComPtr<IXAudio2> xaudio;

        IXAudio2MasteringVoice *masterVoice;
        IXAudio2SourceVoice *sourceVoice;

        WaveData wave;
    };
}
