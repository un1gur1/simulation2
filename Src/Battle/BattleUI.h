#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace App {

    class BattleMaster; // 相互参照を防ぐための前方宣言

    // ==========================================
    // BattleUI: バトル画面の描画と演出を「専属」で担当するクラス
    // 役割: 計算は一切しない。Masterからデータをもらって映すだけ。
    // ==========================================
    class BattleUI {
    public:
        BattleUI();
        ~BattleUI();

        void Init();
        // MasterのUpdateから呼ばれ、シェーダーやアニメーションの時間を進める
        void Update(float effectIntensity, int p1Num, int p2Num);

        // メイン描画関数。Master自身のデータ（const参照）を受け取って描画する
        void Draw(const BattleMaster& master) const;

        // ログの管理もUIの仕事
        void AddLog(const std::string& message);
        void ScrollLog(int wheelDelta, float mouseX, float mouseY);

        // フォント取得（cppにあったものをこっちで管理）
        static int GetCachedFont(int size);
    private:
        void DrawEnemyDangerArea(const BattleMaster& master) const;
        void DrawMovableArea(const BattleMaster& master) const;

        // --- BattleMasterから引っ越してくる変数たち ---
        int m_psHandle;
        int m_cbHandle;
        float m_shaderTime;

        std::vector<std::string> m_actionLog;
        int m_logScrollOffset;

        float m_uiCursorX_1P;
        float m_uiCursorX_2P;
    };

} // namespace App