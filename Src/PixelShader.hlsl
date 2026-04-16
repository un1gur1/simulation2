// 外部（C++）から受け取るパラメータ
cbuffer cb0 : register(b0)
{
    float time; // 経過時間
    float resX; // 画面幅
    float resY; // 画面高さ
    float padding; // パディング（16バイト境界合わせ用）
};

// 頂点シェーダーから受け取るデータ
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

// ピクセルシェーダーのメイン関数
float4 main(PS_INPUT input) : SV_TARGET
{
    // UV座標を -1.0 ～ 1.0 に変換し、画面の縦横比を補正
    float2 uv = (input.TexCoord * 2.0) - 1.0;
    uv.x *= (resX / resY);

    // 擬似的な3D（床のパースペクティブ）を計算
    // 下半分にだけ床があるように見せる
    float y = abs(uv.y) + 0.1;
    float2 gridUV = float2(uv.x / y, 1.0 / y);
    
    // 時間（time）を使って奥から手前にスクロールさせる
    gridUV.y -= time * 2.0;

    // グリッド線の描画計算
    float2 grid = abs(frac(gridUV * 5.0) - 0.5);
    float lineThickness = 0.05 / y; // 奥に行くほど線を細くする
    float lines = smoothstep(lineThickness, 0.0, grid.x) + smoothstep(lineThickness, 0.0, grid.y);

    // 奥の方を暗くフェードアウトさせる（フォグ効果）
    float fog = smoothstep(0.0, 1.5, y);

    // 発光するシアン（水色）系の色に乗算
    float3 color = float3(0.0, 0.5, 1.0) * lines * fog;

    // アルファ値は1.0（不透明）で出力
    return float4(color, 1.0);
}