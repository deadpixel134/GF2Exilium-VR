// Storage-only Y reversal. Integer variant preserves every RGBA8/half-float bit.
float4 vs(uint id : SV_VertexID) : SV_Position {
    return float4(id == 2 ? 3 : -1, id == 1 ? 3 : -1, 0, 1);
}
#ifdef RAW_UINT
Texture2D<uint4> source : register(t0);
uint4 ps(float4 p : SV_Position) : SV_Target {
#else
Texture2D<float4> source : register(t0);
float4 ps(float4 p : SV_Position) : SV_Target {
#endif
    uint w, h; source.GetDimensions(w, h);
    return source.Load(int3(uint(p.x), h - 1 - uint(p.y), 0));
}
