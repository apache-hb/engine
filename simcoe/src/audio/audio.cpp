#include "simcoe/audio/audio.h"
#include "simcoe/core/system.h"

using namespace simcoe;
using namespace simcoe::audio;

Context::Context() {
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 22050;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HR_CHECK(XAudio2Create(&pAudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
    HR_CHECK(pAudio->CreateMasteringVoice(&pMaster));
    HR_CHECK(pAudio->CreateSourceVoice(&pSource, &wfx));
}

void Context::play(const void *pBuffer, size_t size) {
    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = (UINT32)size;
    buffer.pAudioData = (const BYTE*)pBuffer;
    
    HR_CHECK(pSource->SubmitSourceBuffer(&buffer));
    HR_CHECK(pSource->Start(0));
}
