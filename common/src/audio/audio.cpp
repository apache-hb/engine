#include "engine/audio/audio.h"
#include "engine/base/win32.h"
#include <mmdeviceapi.h>

using namespace simcoe;
using namespace simcoe::audio;

namespace {
    constexpr WORD kChannels = 2;
    constexpr DWORD kSampleRate = 44100;
    constexpr WORD kSampleSize = 32;

    constexpr WAVEFORMATEX kWaveFormat = {
        .wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
        .nChannels = kChannels,
        .nSamplesPerSec = kSampleRate,
        .nAvgBytesPerSec = (kSampleRate * kChannels * kSampleSize) / CHAR_BIT,
        .nBlockAlign = 8,
        .wBitsPerSample = kSampleSize,
        .cbSize = 0
    };

    constexpr XAUDIO2_DEBUG_CONFIGURATION kDebugConfig = {
        .TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS,
        .BreakMask = XAUDIO2_LOG_ERRORS,
    };
}

Audio::Audio() {
    HR_CHECK(XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));

    xaudio->SetDebugConfiguration(&kDebugConfig);

    HR_CHECK(xaudio->CreateMasteringVoice(&masterVoice));
    HR_CHECK(xaudio->CreateSourceVoice(&sourceVoice, &kWaveFormat));
    HR_CHECK(sourceVoice->SetVolume(1.f));
}

void Audio::play(WaveData&& data) {
    wave = std::move(data);

    XAUDIO2_BUFFER buffer = {
        .Flags = 0,
        .AudioBytes = UINT32(wave.size() * sizeof(float)),
        .pAudioData = reinterpret_cast<const uint8_t *>(wave.data()),
        .PlayBegin = 0,
        .PlayLength = 0,
        .LoopBegin = 0,
        .LoopLength = 0,
        .LoopCount = XAUDIO2_LOOP_INFINITE
    };

    HR_CHECK(sourceVoice->SubmitSourceBuffer(&buffer));
    HR_CHECK(sourceVoice->Start());
}
