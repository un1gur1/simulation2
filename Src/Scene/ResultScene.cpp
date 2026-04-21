#include "ResultScene.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include "../../CyberGrid.h"

namespace App {

    // ==========================================
    // コンストラクタ: リザルト画面の初期値設定
    // ==========================================
    ResultScene::ResultScene()
        : m_frameCount(0)          // フレームカウンター
        , m_psHandle(-1)           // ピクセルシェーダーハンドル
        , m_cbHandle(-1)           // 定数バッファハンドル
        , m_isWin(false)           // 勝敗フラグ
        , m_stats({ 0, 0, 0, 0, 0 })  // 戦績データ
    {
    }

    // ==========================================
    // デストラクタ: リソース解放
    // ==========================================
    ResultScene::~ResultScene() {
        Release();
    }

    // ==========================================
    // 初期化: バトル結果の取得とシェーダー準備
    // ==========================================
    void ResultScene::Init() {
        m_frameCount = 0;

        // SceneManagerから戦績データを取得
        auto* sm = SceneManager::GetInstance();
        m_isWin = sm->GetLastIsWin();      // 勝敗結果
        m_stats = sm->GetLastStats();      // 詳細戦績

        // 背景エフェクト用シェーダーの読み込み
        if (m_psHandle == -1) {
            m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
            m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        }
    }

    // ==========================================
    // リソース読み込み: 現在は非使用
    // ==========================================
    void ResultScene::Load() {}
    void ResultScene::LoadEnd() {}

    // ==========================================
    // 更新処理: タイトルへの遷移入力受付
    // ==========================================
    void ResultScene::Update() {
        ++m_frameCount;

        // 最初の1秒間はスキップ不可（演出時間確保）
        if (m_frameCount > 60) {
            // Space、Enter、マウスクリックでタイトルへ
            if (CheckHitKey(KEY_INPUT_SPACE) ||
                CheckHitKey(KEY_INPUT_RETURN) ||
                (GetMouseInput() & MOUSE_INPUT_LEFT)) {
                SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
            }
        }
    }

    // ==========================================
    // 描画処理: リザルト画面の表示
    // ==========================================
    void ResultScene::Draw() {
        int sw = 1920, sh = 1080;  // 画面サイズ

        // ==========================================
        // 1. テーマカラー設定（勝敗で色を切り替え）
        // ==========================================
        unsigned int themeColMain = m_isWin
            ? GetColor(255, 165, 0)    // 勝利: オレンジ
            : GetColor(50, 100, 255);   // 敗北: 青

        unsigned int themeColSub = m_isWin
            ? GetColor(255, 50, 0)      // 勝利: 赤
            : GetColor(0, 200, 255);    // 敗北: 水色

        unsigned int bgDarkCol = m_isWin
            ? GetColor(30, 10, 0)       // 勝利: 暗いオレンジ
            : GetColor(0, 10, 30);      // 敗北: 暗い青

        // ==========================================
        // 2. 背景シェーダー描画（サイバーグリッドエフェクト）
        // ==========================================
        DrawBox(0, 0, sw, sh, bgDarkCol, TRUE);

        if (m_psHandle != -1 && m_cbHandle != -1) {
            // シェーダーパラメータ設定
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_frameCount * 0.015f;  // 時間（アニメーション用）
            cb[1] = (float)sw;               // 画面幅
            cb[2] = (float)sh;               // 画面高さ
            cb[3] = 0.0f;                    // 予備
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

            // シェーダー適用
            SetUsePixelShader(m_psHandle);

            // 頂点データ作成（画面全体を覆う2つの三角形）
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0);
                v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(
                    (themeColMain >> 16) & 0xFF,
                    (themeColMain >> 8) & 0xFF,
                    themeColMain & 0xFF,
                    255
                );
                v[i].spc = GetColorU8(0, 0, 0, 0);
            }

            // UV座標設定（テクスチャマッピング用）
            v[0].pos.x = 0;  v[0].pos.y = 0;  v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = sw; v[1].pos.y = 0;  v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;  v[2].pos.y = sh; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = sw; v[3].pos.y = 0;  v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = sw; v[4].pos.y = sh; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;  v[5].pos.y = sh; v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }

        // 半透明オーバーレイ（シェーダーエフェクトを抑える）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, sw, sh, bgDarkCol, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // ==========================================
        // 3. タイトルロゴ（勝利 or 敗北）
        // ==========================================
        const char* resultMain = m_isWin ? "勝利" : "敗北";
        SetFontSize(100);
        int textW = GetDrawStringWidth(resultMain, (int)strlen(resultMain));
        DrawString(sw / 2 - textW / 2, 120, resultMain, GetColor(255, 255, 255));

        // ==========================================
        // 4. 戦績パネル（スコアボード）
        // ==========================================
        int panelX = sw / 2 - 400;
        int panelY = 300;

        // パネル背景（半透明の暗い矩形）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(panelX, panelY, panelX + 800, panelY + 450, GetColor(10, 10, 15), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // パネル枠線（テーマカラー）
        DrawBox(panelX, panelY, panelX + 800, panelY + 450, themeColMain, FALSE);

        // ==========================================
        // 5. 戦績項目の描画
        // ==========================================

        // 描画ヘルパー関数（ラムダ式）
        auto drawStat = [&](int yOffset, const char* label, const std::string& value, bool isHighlight = false) {
            SetFontSize(40);
            unsigned int color = isHighlight ? themeColMain : GetColor(200, 200, 200);

            // ラベル（左寄せ）
            DrawString(panelX + 80, panelY + yOffset, label, color);

            // 値（右寄せ）
            int valW = GetDrawStringWidth(value.c_str(), (int)value.length());
            DrawString(panelX + 720 - valW, panelY + yOffset, value.c_str(), GetColor(255, 255, 255));

            // 区切り線
            DrawLine(panelX + 60, panelY + yOffset + 50, panelX + 740, panelY + yOffset + 50, GetColor(50, 50, 60), 1);
            };

        // クリアタイム計算（ミリ秒 → MM:SS形式）
        int totalSec = m_stats.playTimeFrames / 1000;
        char timeStr[64];
        sprintf_s(timeStr, "%02d : %02d", totalSec / 60, totalSec % 60);

        // 各項目を表示
        drawStat(40, "クリアタイム", timeStr);
        drawStat(120, "総ターン数", std::to_string(m_stats.totalTurns));
        drawStat(200, "総移動数", std::to_string(m_stats.totalMoves));
        drawStat(280, "使用演算子数", std::to_string(m_stats.totalOpsUsed));
        drawStat(360, "最大ダメージ / スコア", std::to_string(m_stats.maxDamage), true);  // ハイライト表示

        // ==========================================
        // 6. 操作ガイド（点滅演出）
        // ==========================================
        if (m_frameCount > 90) {  // 1.5秒後から表示
            // サイン波で透明度を変化させる（点滅効果）
            int pulseAlpha = 100 + (int)(std::sin(m_frameCount / 15.0f) * 100);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);

            SetFontSize(30);
            const char* prompt = ">>クリックかスペース <<";
            int pW = GetDrawStringWidth(prompt, (int)strlen(prompt));
            DrawString(sw / 2 - pW / 2, sh - 150, prompt, GetColor(200, 200, 200));

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // ==========================================
    // 解放処理: シェーダーリソースの削除
    // ==========================================
    void ResultScene::Release() {
        if (m_psHandle != -1) {
            DeleteShader(m_psHandle);
            m_psHandle = -1;
        }
        if (m_cbHandle != -1) {
            DeleteShaderConstantBuffer(m_cbHandle);
            m_cbHandle = -1;
        }
    }

} // namespace App