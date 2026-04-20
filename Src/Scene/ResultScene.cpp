#include "ResultScene.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include "../../CyberGrid.h"

namespace App {

    // ★ コンストラクタで変数を初期化
    ResultScene::ResultScene()
        : m_frameCount(0), m_psHandle(-1), m_cbHandle(-1)
        , m_isWin(false), m_stats({ 0, 0, 0, 0, 0 })
    {
    }

    ResultScene::~ResultScene() { Release(); }

    void ResultScene::Init() {
        m_frameCount = 0;

        // ==========================================
        // ★ SceneManager から戦績データを引き出す！
        // ==========================================
        auto* sm = SceneManager::GetInstance();
        m_isWin = sm->GetLastIsWin();
        m_stats = sm->GetLastStats();

        if (m_psHandle == -1) {
            m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
            m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        }
    }

    void ResultScene::Load() {}
    void ResultScene::LoadEnd() {}

    void ResultScene::Update() {
        ++m_frameCount;
        if (m_frameCount > 60) { // 1秒はスキップ不可
            if (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_RETURN) || (GetMouseInput() & MOUSE_INPUT_LEFT)) {
                SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
            }
        }
    }

    void ResultScene::Draw() {
        int sw = 1920, sh = 1080;
        // ★ s_isWin ではなく m_isWin を使う
        unsigned int themeColMain = m_isWin ? GetColor(255, 165, 0) : GetColor(50, 100, 255);
        unsigned int themeColSub = m_isWin ? GetColor(255, 50, 0) : GetColor(0, 200, 255);
        unsigned int bgDarkCol = m_isWin ? GetColor(30, 10, 0) : GetColor(0, 10, 30);

        // 背景シェーダー描画
        DrawBox(0, 0, sw, sh, bgDarkCol, TRUE);
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_frameCount * 0.015f; cb[1] = (float)sw; cb[2] = (float)sh; cb[3] = 0.0f;
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);
            SetUsePixelShader(m_psHandle);
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                v[i].dif = GetColorU8((themeColMain >> 16) & 0xFF, (themeColMain >> 8) & 0xFF, themeColMain & 0xFF, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
            }
            v[0].pos.x = 0;  v[0].pos.y = 0;  v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = sw; v[1].pos.y = 0;  v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;  v[2].pos.y = sh; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = sw; v[3].pos.y = 0;  v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = sw; v[4].pos.y = sh; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;  v[5].pos.y = sh; v[5].u = 0.0f; v[5].v = 1.0f;
            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, sw, sh, bgDarkCol, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // タイトルロゴ
        const char* resultMain = m_isWin ? "MISSION CLEAR" : "MISSION FAILED";
        SetFontSize(100);
        int textW = GetDrawStringWidth(resultMain, (int)strlen(resultMain));
        DrawString(sw / 2 - textW / 2, 120, resultMain, GetColor(255, 255, 255));

        // ==========================================
        // ★ 戦績（スコアボード）の描画
        // ==========================================
        int panelX = sw / 2 - 400;
        int panelY = 300;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(panelX, panelY, panelX + 800, panelY + 450, GetColor(10, 10, 15), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(panelX, panelY, panelX + 800, panelY + 450, themeColMain, FALSE);

        // 各項目の描画ヘルパー
        auto drawStat = [&](int yOffset, const char* label, const std::string& value, bool isHighlight = false) {
            SetFontSize(40);
            unsigned int color = isHighlight ? themeColMain : GetColor(200, 200, 200);
            DrawString(panelX + 80, panelY + yOffset, label, color);
            int valW = GetDrawStringWidth(value.c_str(), (int)value.length());
            DrawString(panelX + 720 - valW, panelY + yOffset, value.c_str(), GetColor(255, 255, 255));
            DrawLine(panelX + 60, panelY + yOffset + 50, panelX + 740, panelY + yOffset + 50, GetColor(50, 50, 60), 1);
            };

        // 時間計算（フレーム数を秒に変換して MM:SS にする）
        int totalSec = m_stats.playTimeFrames / 1000;
        char timeStr[64];
        sprintf_s(timeStr, "%02d : %02d", totalSec / 60, totalSec % 60);

        // ★ s_stats ではなく m_stats を使う
        drawStat(40, "CLEAR TIME", timeStr);
        drawStat(120, "TOTAL TURNS", std::to_string(m_stats.totalTurns));
        drawStat(200, "TOTAL MOVES", std::to_string(m_stats.totalMoves));
        drawStat(280, "OPERATORS USED", std::to_string(m_stats.totalOpsUsed));
        drawStat(360, "MAX DAMAGE / SCORE", std::to_string(m_stats.maxDamage), true);

        // 操作ガイド
        if (m_frameCount > 90) {
            int pulseAlpha = 100 + (int)(std::sin(m_frameCount / 15.0f) * 100);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
            SetFontSize(30);
            const char* prompt = ">> CLICK OR SPACE TO RETURN TITLE <<";
            int pW = GetDrawStringWidth(prompt, (int)strlen(prompt));
            DrawString(sw / 2 - pW / 2, sh - 150, prompt, GetColor(200, 200, 200));
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    void ResultScene::Release() {
        if (m_psHandle != -1) { DeleteShader(m_psHandle); m_psHandle = -1; }
        if (m_cbHandle != -1) { DeleteShaderConstantBuffer(m_cbHandle); m_cbHandle = -1; }
    }

} // namespace App