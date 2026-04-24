#pragma once

// 波形の種類
enum class WaveType {
    SINE,       // 澄んだ音（サイン波）
    SQUARE,     // ピコピコ音（矩形波）
    TRIANGLE,   // 柔らかい音（三角波）
    SAWTOOTH,   // 尖った太い音（ノコギリ波）
    NOISE       // 爆発音・打楽器用（ホワイトノイズ）
};

// ADSRエンベロープ（音の輪郭を決定するパラメータ）
struct ADSR {
    double attack;  // 立ち上がり時間(秒)
    double decay;   // 減衰時間(秒)
    double sustain; // 減衰後の保持ボリューム(0.0 ~ 1.0)
    double release; // 余韻(秒)
};

class ProceduralAudio {
public:
    // シングルトンインスタンスの取得
    static ProceduralAudio& GetInstance() {
        static ProceduralAudio instance;
        return instance;
    }

    void Init();               // 起動時に波形を事前計算してメモリに配置
    void Update();             // 毎フレーム呼ぶ（BGMのシーケンス処理）
    void PlayBGM(bool play);   // BGMの再生/停止
    void PlayPowerSE(int num); // パワー(1〜9)に応じたUI音
    void PlayErrorSE();        // 失敗時のノイズ音
    void PlayClickSE(); // 再生用関数

private:
    ProceduralAudio() {}
    ~ProceduralAudio() {}
    ProceduralAudio(const ProceduralAudio&) = delete;
    ProceduralAudio& operator=(const ProceduralAudio&) = delete;

    // 究極のシンセサイザー波形生成関数
    int GenerateWave(double freq, WaveType type, ADSR env, double duration, float volume = 1.0f);
    double MidiToFreq(int midi);

    // 生成したサウンドハンドルの保管庫
    int m_bgmNotes[128];  // MIDIノート番号(0~127)に対応する音
    int m_powerNotes[10]; // パワー(1~9)用
    int m_errorSE;        // エラー音用
    int m_clickSE; // クリック音のハンドルを入れる変数

    // 自動演奏（シーケンサー）用の変数
    bool m_isBgmPlaying = false;
    int m_bgmNoteIndex = 0;
    long long m_nextNoteTime = 0;
    int m_tempoBPM = 150; // トルコ行進曲のテンポ
};