#include "ProceduralAudio.h"
#include <DxLib.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


struct NoteData { int midi; int length; };
// ==========================================
// 🎵 バッハ『プレリュード』（高音サイバー・アルペジオVer）
// ==========================================
const NoteData CYBER_BACH_HIGH[] = {
    // パターン1: ハ短調
    {72, 1}, {75, 1}, {74, 1}, {75, 1}, {72, 1}, {75, 1}, {74, 1}, {75, 1},
    {72, 1}, {75, 1}, {74, 1}, {75, 1}, {72, 1}, {75, 1}, {74, 1}, {75, 1},

    // パターン2: 変イ長調
    {68, 1}, {77, 1}, {76, 1}, {77, 1}, {72, 1}, {77, 1}, {76, 1}, {77, 1},
    {68, 1}, {77, 1}, {76, 1}, {77, 1}, {72, 1}, {77, 1}, {76, 1}, {77, 1},

    // パターン3: ロ減七
    {71, 1}, {77, 1}, {75, 1}, {77, 1}, {74, 1}, {77, 1}, {75, 1}, {77, 1},
    {71, 1}, {77, 1}, {75, 1}, {77, 1}, {74, 1}, {77, 1}, {75, 1}, {77, 1},

    // パターン4: ハ短調
    {72, 1}, {79, 1}, {77, 1}, {79, 1}, {75, 1}, {79, 1}, {77, 1}, {79, 1},
    {72, 1}, {79, 1}, {77, 1}, {79, 1}, {75, 1}, {79, 1}, {77, 1}, {79, 1}
};
const int MELODY_LENGTH = sizeof(CYBER_BACH_HIGH) / sizeof(NoteData);


// ==========================================
// 🎹 初期化
// ==========================================
void ProceduralAudio::Init() {
    static bool isInitialized = false;
    if (isInitialized) return;
    isInitialized = true;

    // 高音アルペジオ専用のエンベロープ
    ADSR bgmEnv = { 0.01, 0.1, 0.2, 0.3 };

    for (int i = 0; i < 128; ++i) m_bgmNotes[i] = -1;

    // 高音（79など）に対応するため、生成範囲を 100 まで広げる！
    for (int i = 40; i <= 100; ++i) {

        m_bgmNotes[i] = GenerateWave(MidiToFreq(i), WaveType::SAWTOOTH, bgmEnv, 2.0, 0.1f);
    }

    // パワーSE用：澄み切った鐘のような音
    ADSR seEnv = { 0.01, 0.1, 0.2, 0.5 };
    double freqs[] = { 0, 261.6, 293.6, 329.6, 349.2, 392.0, 440.0, 493.8, 523.2, 587.3 };
    for (int n = 1; n <= 9; ++n) {
        m_powerNotes[n] = GenerateWave(freqs[n], WaveType::TRIANGLE, seEnv, 0.6, 0.5f);
    }
    // クリック音用：立ち上がりが一瞬で、余韻が全くない設定
    ADSR clickEnv = { 0.01, 0.05, 0.0, 0.0 };

    // 高めの音（880Hz=高いラ）で、矩形波(SQUARE)を使ってファミコン風の「ピッ」を作る
    // 長さはたったの 0.06秒！
    m_clickSE = GenerateWave(880.0, WaveType::SQUARE, clickEnv, 0.06, 0.4f);
    // エラー用
    ADSR errEnv = { 0.01, 0.2, 0.0, 0.1 };
    m_errorSE = GenerateWave(440.0, WaveType::NOISE, errEnv, 0.3, 0.4f);
}

// ==========================================
// 波形生成（SuperSaw + サブベース + ディレイ）
// ==========================================
int ProceduralAudio::GenerateWave(double freq, WaveType type, ADSR env, double duration, float volume) {
    int sampleRate = 44100;
    int numSamples = (int)(sampleRate * duration);

    // ステレオWAVヘッダ錬成
    int channels = 2;
    int blockAlign = channels * 2;
    int byteRate = sampleRate * blockAlign;
    int dataSize = numSamples * blockAlign;
    std::vector<char> wavData(44 + dataSize);
    char* p = wavData.data();

    memcpy(p, "RIFF", 4); p += 4;
    int fileSize = 36 + dataSize; memcpy(p, &fileSize, 4); p += 4;
    memcpy(p, "WAVE", 4); p += 4;
    memcpy(p, "fmt ", 4); p += 4;
    int fmtSize = 16; memcpy(p, &fmtSize, 4); p += 4;
    short format = 1; memcpy(p, &format, 2); p += 2;
    short channelsOut = (short)channels; memcpy(p, &channelsOut, 2); p += 2;
    memcpy(p, &sampleRate, 4); p += 4;
    memcpy(p, &byteRate, 4); p += 4;
    short blockAlignOut = (short)blockAlign; memcpy(p, &blockAlignOut, 2); p += 2;
    short bitsPerSample = 16; memcpy(p, &bitsPerSample, 2); p += 2;
    memcpy(p, "data", 4); p += 4;
    memcpy(p, &dataSize, 4); p += 4;
    short* pcmData = (short*)p;

    // シンセサイザーの心臓部（5重ユニゾン設定）
    const int UNISON_VOICES = 5;
    double detune[UNISON_VOICES] = { 0.988, 0.994, 1.000, 1.006, 1.012 };
    double panL[UNISON_VOICES] = { 1.0,   0.7,   0.5,   0.3,   0.0 };
    double panR[UNISON_VOICES] = { 0.0,   0.3,   0.5,   0.7,   1.0 };
    double phase[UNISON_VOICES];
    for (int v = 0; v < UNISON_VOICES; v++) phase[v] = (rand() % 1000) / 1000.0;

    // フィルター用バッファ
    double filterL0 = 0, filterL1 = 0;
    double filterR0 = 0, filterR1 = 0;

    // ピンポン・ディレイ（やまびこ）用のバッファ
    int delaySamples = (int)(sampleRate * 0.3); // 0.3秒遅れ
    std::vector<double> delayBufferL(numSamples, 0.0);
    std::vector<double> delayBufferR(numSamples, 0.0);
    double feedback = 0.45; // やまびこの減衰率

    // 波形計算ラムダ（位相指定対応）
    auto getOsc = [](WaveType t, double f, double time, double ph) {
        double currentPhase = fmod(time * f + ph, 1.0);
        switch (t) {
        case WaveType::SINE:     return sin(2.0 * M_PI * currentPhase);
        case WaveType::SQUARE:   return (currentPhase < 0.5) ? 1.0 : -1.0;
        case WaveType::TRIANGLE: return (currentPhase < 0.5) ? (4.0 * currentPhase - 1.0) : (3.0 - 4.0 * currentPhase);
        case WaveType::SAWTOOTH: return 2.0 * currentPhase - 1.0;
        case WaveType::NOISE:    return ((rand() % 20000) - 10000) / 10000.0;
        }
        return 0.0;
        };

    for (int i = 0; i < numSamples; i++) {
        double t = (double)i / sampleRate;

        // 1. ADSRエンベロープ
        double volEnv = 0.0;
        if (t < env.attack) volEnv = t / env.attack;
        else if (t < env.attack + env.decay) volEnv = 1.0 - (1.0 - env.sustain) * ((t - env.attack) / env.decay);
        else if (t < duration - env.release) volEnv = env.sustain;
        else {
            double releaseT = t - (duration - env.release);
            volEnv = env.sustain * (1.0 - (releaseT / env.release));
            if (volEnv < 0) volEnv = 0;
        }

        // LFO (ビブラート)
        double lfo = sin(2.0 * M_PI * 4.0 * t) * 0.002;

        // 2. ユニゾン（SuperSaw）合成
        double waveL = 0.0, waveR = 0.0;
        for (int v = 0; v < UNISON_VOICES; v++) {
            double currentFreq = freq * detune[v] * (1.0 + lfo);
            double w = getOsc(type, currentFreq, t, phase[v]);
            waveL += w * panL[v];
            waveR += w * panR[v];
        }
        waveL /= UNISON_VOICES;
        waveR /= UNISON_VOICES;

        // 3. トランジェント・アタック（打撃音によるアタック強調）
        double transient = 0.0;
        if (t < 0.015) {
            transient = getOsc(WaveType::NOISE, 0, t, 0) * (1.0 - (t / 0.015)) * 0.4;
        }

        // 4. スクエア・サブベース（腹に響くファミコン低音）
        double subBass = getOsc(WaveType::SQUARE, freq * 0.5, t, 0) * 0.25;

        // すべての音をミックス
        waveL += transient + subBass;
        waveR += transient + subBass;

        // 5. 動的フィルター（最初は明るく、徐々にこもる）
        double filterEnv = 0.1 + 0.9 * volEnv;
        double cutoff = filterEnv * filterEnv;
        filterL0 += cutoff * (waveL - filterL0); filterL1 += cutoff * (filterL0 - filterL1);
        filterR0 += cutoff * (waveR - filterR0); filterR1 += cutoff * (filterR0 - filterR1);

        // 6. サチュレーション（真空管の歪み）
        double outL = tanh(filterL1 * 1.5) * volume;
        double outR = tanh(filterR1 * 1.5) * volume;

        // 7. ピンポン・ディレイ（左右に飛び交うやまびこ）
        double delayL = (i >= delaySamples) ? delayBufferL[i - delaySamples] : 0.0;
        double delayR = (i >= delaySamples) ? delayBufferR[i - delaySamples] : 0.0;

        // クロス・フィードバック（左の残響を右へ、右を左へ）
        delayBufferL[i] = outL + delayR * feedback;
        delayBufferR[i] = outR + delayL * feedback;

        // 原音にディレイ音を40%ブレンド
        outL = outL + delayL * 0.4;
        outR = outR + delayR * 0.4;

        // 8. 音割れ防止リミッター
        if (outL > 1.0) outL = 1.0; else if (outL < -1.0) outL = -1.0;
        if (outR > 1.0) outR = 1.0; else if (outR < -1.0) outR = -1.0;

        // 書き込み
        pcmData[i * 2 + 0] = (short)(outL * 32767.0);
        pcmData[i * 2 + 1] = (short)(outR * 32767.0);
    }

    return LoadSoundMemByMemImage(wavData.data(), (int)wavData.size());
}

double ProceduralAudio::MidiToFreq(int midi) {
    if (midi == 0) return 0;
    return 440.0 * pow(2.0, (midi - 69) / 12.0);
}

// ==========================================
// 再生インターフェース
// ==========================================
void ProceduralAudio::PlayBGM(bool play) {
    m_isBgmPlaying = play;
    if (play) m_nextNoteTime = GetNowCount();
}

void ProceduralAudio::PlayPowerSE(int num) {
    if (num >= 1 && num <= 9) PlaySoundMem(m_powerNotes[num], DX_PLAYTYPE_BACK);
}

void ProceduralAudio::PlayErrorSE() {
    PlaySoundMem(m_errorSE, DX_PLAYTYPE_BACK);
}

// ==========================================
// シーケンサー（自動演奏の心臓部）
// ==========================================
void ProceduralAudio::Update() {
    if (!m_isBgmPlaying) return;

    long long now = GetNowCount();
    if (now >= m_nextNoteTime) {
        NoteData note = CYBER_BACH_HIGH[m_bgmNoteIndex];
        if (note.midi > 0 && m_bgmNotes[note.midi] != -1) {
            PlaySoundMem(m_bgmNotes[note.midi], DX_PLAYTYPE_BACK);
        }

		// 16分音符単位で次のノート時間を計算（トルコ行進曲は速いので16分音符で刻む）
        int msPer16th = 60000 / m_tempoBPM / 4;
        m_nextNoteTime = now + (note.length * msPer16th);
        m_bgmNoteIndex = (m_bgmNoteIndex + 1) % MELODY_LENGTH;
    }
}

void ProceduralAudio::PlayClickSE() {
    PlaySoundMem(m_clickSE, DX_PLAYTYPE_BACK);
}