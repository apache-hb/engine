#pragma once

#include <xaudio2.h>

namespace simcoe::audio {
    struct Context {
        Context();

        void play(const void *pBuffer, size_t size);

    private:
        IXAudio2 *pAudio = nullptr;
        IXAudio2MasteringVoice *pMaster = nullptr;
        IXAudio2SourceVoice *pSource = nullptr;
    };
}
