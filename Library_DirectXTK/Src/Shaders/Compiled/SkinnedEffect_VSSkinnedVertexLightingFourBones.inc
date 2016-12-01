#if 0
//
// Generated by Microsoft (R) D3D Shader Disassembler
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Position              0   xyzw        0     NONE   float   xyzw
// NORMAL                   0   xyz         1     NONE   float   xyz 
// TEXCOORD                 0   xy          2     NONE   float   xy  
// BLENDINDICES             0   xyzw        3     NONE    uint   xyzw
// BLENDWEIGHT              0   xyzw        4     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 0   xy          2     NONE   float   xy  
// SV_Position              0   xyzw        3      POS   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c0         cb0             0       245  ( FLT, FLT, FLT, FLT)
//
//
// Runtime generated constant mappings:
//
// Target Reg                               Constant Description
// ---------- --------------------------------------------------
// c245                            Vertex Shader position offset
//
//
// Level9 shader bytecode:
//
    vs_2_0
    def c246, 3, 0, 1, 0
    dcl_texcoord v0
    dcl_texcoord1 v1
    dcl_texcoord2 v2
    dcl_texcoord3 v3
    dcl_texcoord4 v4
    mul r0, v3, c246.x
    mova a0, r0.yxzw
    mul r1, v4.y, c26[a0.x]
    mad r1, c26[a0.y], v4.x, r1
    mad r0, c26[a0.z], v4.z, r1
    mad r0, c26[a0.w], v4.w, r0
    dp3 r1.x, v1, r0
    dp4 r0.x, v0, r0
    mul r2, v4.y, c27[a0.x]
    mul r3, v4.y, c28[a0.x]
    mad r3, c28[a0.y], v4.x, r3
    mad r2, c27[a0.y], v4.x, r2
    mad r2, c27[a0.z], v4.z, r2
    mad r3, c28[a0.z], v4.z, r3
    mad r3, c28[a0.w], v4.w, r3
    mad r2, c27[a0.w], v4.w, r2
    dp3 r1.y, v1, r2
    dp4 r0.y, v0, r2
    dp3 r1.z, v1, r3
    dp4 r0.z, v0, r3
    dp3 r2.x, r1, c19
    dp3 r2.y, r1, c20
    dp3 r2.z, r1, c21
    nrm r1.xyz, r2
    dp3 r2.x, -c3, r1
    dp3 r2.y, -c4, r1
    dp3 r2.z, -c5, r1
    sge r3.xyz, r2, c246.y
    mul r2.xyz, r2, r3
    mul r4.xyz, r2.y, c7
    mad r2.xyw, r2.x, c6.xyzz, r4.xyzz
    mad r2.xyz, r2.z, c8, r2.xyww
    mov r4.xyz, c0
    mad oT0.xyz, r2, r4, c1
    mov r0.w, v0.w
    dp4 r2.x, r0, c15
    dp4 r2.y, r0, c16
    dp4 r2.z, r0, c17
    add r2.xyz, -r2, c12
    nrm r4.xyz, r2
    add r2.xyz, r4, -c3
    nrm r5.xyz, r2
    dp3 r2.x, r5, r1
    add r5.xyz, r4, -c4
    add r4.xyz, r4, -c5
    nrm r6.xyz, r4
    dp3 r2.z, r6, r1
    nrm r4.xyz, r5
    dp3 r2.y, r4, r1
    max r1.xyz, r2, c246.y
    mul r1.xyz, r3, r1
    log r2.x, r1.x
    log r2.y, r1.y
    log r2.z, r1.z
    mul r1.xyz, r2, c2.w
    exp r1.y, r1.y
    mul r2.xyz, r1.y, c10
    exp r1.x, r1.x
    exp r1.y, r1.z
    mad r1.xzw, r1.x, c9.xyyz, r2.xyyz
    mad r1.xyz, r1.y, c11, r1.xzww
    mul oT1.xyz, r1, c2
    dp4 oPos.z, r0, c24
    dp4 r1.x, r0, c14
    max r1.x, r1.x, c246.y
    min oT1.w, r1.x, c246.z
    dp4 r1.x, r0, c22
    dp4 r1.y, r0, c23
    dp4 r0.x, r0, c25
    mad oPos.xy, r0.x, c245, r1
    mov oPos.w, r0.x
    mov oT0.w, c0.w
    mov oT2.xy, v2

// approximately 83 instruction slots used
vs_4_0
dcl_constantbuffer cb0[245], dynamicIndexed
dcl_input v0.xyzw
dcl_input v1.xyz
dcl_input v2.xy
dcl_input v3.xyzw
dcl_input v4.xyzw
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_output o2.xy
dcl_output_siv o3.xyzw, position
dcl_temps 6
imul null, r0.xyzw, v3.xyzw, l(3, 3, 3, 3)
mul r1.xyzw, v4.yyyy, cb0[r0.y + 26].xyzw
mad r1.xyzw, cb0[r0.x + 26].xyzw, v4.xxxx, r1.xyzw
mad r1.xyzw, cb0[r0.z + 26].xyzw, v4.zzzz, r1.xyzw
mad r1.xyzw, cb0[r0.w + 26].xyzw, v4.wwww, r1.xyzw
dp3 r2.x, v1.xyzx, r1.xyzx
dp4 r1.x, v0.xyzw, r1.xyzw
mul r3.xyzw, v4.yyyy, cb0[r0.y + 27].xyzw
mad r3.xyzw, cb0[r0.x + 27].xyzw, v4.xxxx, r3.xyzw
mad r3.xyzw, cb0[r0.z + 27].xyzw, v4.zzzz, r3.xyzw
mad r3.xyzw, cb0[r0.w + 27].xyzw, v4.wwww, r3.xyzw
dp3 r2.y, v1.xyzx, r3.xyzx
dp4 r1.y, v0.xyzw, r3.xyzw
mul r3.xyzw, v4.yyyy, cb0[r0.y + 28].xyzw
mad r3.xyzw, cb0[r0.x + 28].xyzw, v4.xxxx, r3.xyzw
mad r3.xyzw, cb0[r0.z + 28].xyzw, v4.zzzz, r3.xyzw
mad r0.xyzw, cb0[r0.w + 28].xyzw, v4.wwww, r3.xyzw
dp3 r2.z, v1.xyzx, r0.xyzx
dp4 r1.z, v0.xyzw, r0.xyzw
dp3 r0.x, r2.xyzx, cb0[19].xyzx
dp3 r0.y, r2.xyzx, cb0[20].xyzx
dp3 r0.z, r2.xyzx, cb0[21].xyzx
dp3 r0.w, r0.xyzx, r0.xyzx
rsq r0.w, r0.w
mul r0.xyz, r0.wwww, r0.xyzx
dp3 r2.x, -cb0[3].xyzx, r0.xyzx
dp3 r2.y, -cb0[4].xyzx, r0.xyzx
dp3 r2.z, -cb0[5].xyzx, r0.xyzx
ge r3.xyz, r2.xyzx, l(0.000000, 0.000000, 0.000000, 0.000000)
and r3.xyz, r3.xyzx, l(0x3f800000, 0x3f800000, 0x3f800000, 0)
mul r2.xyz, r2.xyzx, r3.xyzx
mul r4.xyz, r2.yyyy, cb0[7].xyzx
mad r2.xyw, r2.xxxx, cb0[6].xyxz, r4.xyxz
mad r2.xyz, r2.zzzz, cb0[8].xyzx, r2.xywx
mad o0.xyz, r2.xyzx, cb0[0].xyzx, cb0[1].xyzx
mov o0.w, cb0[0].w
mov r1.w, v0.w
dp4 r2.x, r1.xyzw, cb0[15].xyzw
dp4 r2.y, r1.xyzw, cb0[16].xyzw
dp4 r2.z, r1.xyzw, cb0[17].xyzw
add r2.xyz, -r2.xyzx, cb0[12].xyzx
dp3 r0.w, r2.xyzx, r2.xyzx
rsq r0.w, r0.w
mad r4.xyz, r2.xyzx, r0.wwww, -cb0[3].xyzx
dp3 r2.w, r4.xyzx, r4.xyzx
rsq r2.w, r2.w
mul r4.xyz, r2.wwww, r4.xyzx
dp3 r4.x, r4.xyzx, r0.xyzx
mad r5.xyz, r2.xyzx, r0.wwww, -cb0[4].xyzx
mad r2.xyz, r2.xyzx, r0.wwww, -cb0[5].xyzx
dp3 r0.w, r5.xyzx, r5.xyzx
rsq r0.w, r0.w
mul r5.xyz, r0.wwww, r5.xyzx
dp3 r4.y, r5.xyzx, r0.xyzx
dp3 r0.w, r2.xyzx, r2.xyzx
rsq r0.w, r0.w
mul r2.xyz, r0.wwww, r2.xyzx
dp3 r4.z, r2.xyzx, r0.xyzx
max r0.xyz, r4.xyzx, l(0.000000, 0.000000, 0.000000, 0.000000)
mul r0.xyz, r3.xyzx, r0.xyzx
log r0.xyz, r0.xyzx
mul r0.xyz, r0.xyzx, cb0[2].wwww
exp r0.xyz, r0.xyzx
mul r2.xyz, r0.yyyy, cb0[10].xyzx
mad r0.xyw, r0.xxxx, cb0[9].xyxz, r2.xyxz
mad r0.xyz, r0.zzzz, cb0[11].xyzx, r0.xywx
mul o1.xyz, r0.xyzx, cb0[2].xyzx
dp4_sat o1.w, r1.xyzw, cb0[14].xyzw
mov o2.xy, v2.xyxx
dp4 o3.x, r1.xyzw, cb0[22].xyzw
dp4 o3.y, r1.xyzw, cb0[23].xyzw
dp4 o3.z, r1.xyzw, cb0[24].xyzw
dp4 o3.w, r1.xyzw, cb0[25].xyzw
ret 
// Approximately 0 instruction slots used
#endif

const BYTE SkinnedEffect_VSSkinnedVertexLightingFourBones[] =
{
     68,  88,  66,  67,  90, 157, 
      5,  24, 209, 210,  25,  42, 
    252,  13, 154,  30,  46, 104, 
    166, 207,   1,   0,   0,   0, 
    240,  16,   0,   0,   4,   0, 
      0,   0,  48,   0,   0,   0, 
    128,   5,   0,   0, 164,  15, 
      0,   0, 100,  16,   0,   0, 
     65, 111, 110,  57,  72,   5, 
      0,   0,  72,   5,   0,   0, 
      0,   2, 254, 255,  20,   5, 
      0,   0,  52,   0,   0,   0, 
      1,   0,  36,   0,   0,   0, 
     48,   0,   0,   0,  48,   0, 
      0,   0,  36,   0,   1,   0, 
     48,   0,   0,   0,   0,   0, 
    245,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 245,   0, 
      0,   2, 254, 255,  81,   0, 
      0,   5, 246,   0,  15, 160, 
      0,   0,  64,  64,   0,   0, 
      0,   0,   0,   0, 128,  63, 
      0,   0,   0,   0,  31,   0, 
      0,   2,   5,   0,   0, 128, 
      0,   0,  15, 144,  31,   0, 
      0,   2,   5,   0,   1, 128, 
      1,   0,  15, 144,  31,   0, 
      0,   2,   5,   0,   2, 128, 
      2,   0,  15, 144,  31,   0, 
      0,   2,   5,   0,   3, 128, 
      3,   0,  15, 144,  31,   0, 
      0,   2,   5,   0,   4, 128, 
      4,   0,  15, 144,   5,   0, 
      0,   3,   0,   0,  15, 128, 
      3,   0, 228, 144, 246,   0, 
      0, 160,  46,   0,   0,   2, 
      0,   0,  15, 176,   0,   0, 
    225, 128,   5,   0,   0,   4, 
      1,   0,  15, 128,   4,   0, 
     85, 144,  26,  32, 228, 160, 
      0,   0,   0, 176,   4,   0, 
      0,   5,   1,   0,  15, 128, 
     26,  32, 228, 160,   0,   0, 
     85, 176,   4,   0,   0, 144, 
      1,   0, 228, 128,   4,   0, 
      0,   5,   0,   0,  15, 128, 
     26,  32, 228, 160,   0,   0, 
    170, 176,   4,   0, 170, 144, 
      1,   0, 228, 128,   4,   0, 
      0,   5,   0,   0,  15, 128, 
     26,  32, 228, 160,   0,   0, 
    255, 176,   4,   0, 255, 144, 
      0,   0, 228, 128,   8,   0, 
      0,   3,   1,   0,   1, 128, 
      1,   0, 228, 144,   0,   0, 
    228, 128,   9,   0,   0,   3, 
      0,   0,   1, 128,   0,   0, 
    228, 144,   0,   0, 228, 128, 
      5,   0,   0,   4,   2,   0, 
     15, 128,   4,   0,  85, 144, 
     27,  32, 228, 160,   0,   0, 
      0, 176,   5,   0,   0,   4, 
      3,   0,  15, 128,   4,   0, 
     85, 144,  28,  32, 228, 160, 
      0,   0,   0, 176,   4,   0, 
      0,   5,   3,   0,  15, 128, 
     28,  32, 228, 160,   0,   0, 
     85, 176,   4,   0,   0, 144, 
      3,   0, 228, 128,   4,   0, 
      0,   5,   2,   0,  15, 128, 
     27,  32, 228, 160,   0,   0, 
     85, 176,   4,   0,   0, 144, 
      2,   0, 228, 128,   4,   0, 
      0,   5,   2,   0,  15, 128, 
     27,  32, 228, 160,   0,   0, 
    170, 176,   4,   0, 170, 144, 
      2,   0, 228, 128,   4,   0, 
      0,   5,   3,   0,  15, 128, 
     28,  32, 228, 160,   0,   0, 
    170, 176,   4,   0, 170, 144, 
      3,   0, 228, 128,   4,   0, 
      0,   5,   3,   0,  15, 128, 
     28,  32, 228, 160,   0,   0, 
    255, 176,   4,   0, 255, 144, 
      3,   0, 228, 128,   4,   0, 
      0,   5,   2,   0,  15, 128, 
     27,  32, 228, 160,   0,   0, 
    255, 176,   4,   0, 255, 144, 
      2,   0, 228, 128,   8,   0, 
      0,   3,   1,   0,   2, 128, 
      1,   0, 228, 144,   2,   0, 
    228, 128,   9,   0,   0,   3, 
      0,   0,   2, 128,   0,   0, 
    228, 144,   2,   0, 228, 128, 
      8,   0,   0,   3,   1,   0, 
      4, 128,   1,   0, 228, 144, 
      3,   0, 228, 128,   9,   0, 
      0,   3,   0,   0,   4, 128, 
      0,   0, 228, 144,   3,   0, 
    228, 128,   8,   0,   0,   3, 
      2,   0,   1, 128,   1,   0, 
    228, 128,  19,   0, 228, 160, 
      8,   0,   0,   3,   2,   0, 
      2, 128,   1,   0, 228, 128, 
     20,   0, 228, 160,   8,   0, 
      0,   3,   2,   0,   4, 128, 
      1,   0, 228, 128,  21,   0, 
    228, 160,  36,   0,   0,   2, 
      1,   0,   7, 128,   2,   0, 
    228, 128,   8,   0,   0,   3, 
      2,   0,   1, 128,   3,   0, 
    228, 161,   1,   0, 228, 128, 
      8,   0,   0,   3,   2,   0, 
      2, 128,   4,   0, 228, 161, 
      1,   0, 228, 128,   8,   0, 
      0,   3,   2,   0,   4, 128, 
      5,   0, 228, 161,   1,   0, 
    228, 128,  13,   0,   0,   3, 
      3,   0,   7, 128,   2,   0, 
    228, 128, 246,   0,  85, 160, 
      5,   0,   0,   3,   2,   0, 
      7, 128,   2,   0, 228, 128, 
      3,   0, 228, 128,   5,   0, 
      0,   3,   4,   0,   7, 128, 
      2,   0,  85, 128,   7,   0, 
    228, 160,   4,   0,   0,   4, 
      2,   0,  11, 128,   2,   0, 
      0, 128,   6,   0, 164, 160, 
      4,   0, 164, 128,   4,   0, 
      0,   4,   2,   0,   7, 128, 
      2,   0, 170, 128,   8,   0, 
    228, 160,   2,   0, 244, 128, 
      1,   0,   0,   2,   4,   0, 
      7, 128,   0,   0, 228, 160, 
      4,   0,   0,   4,   0,   0, 
      7, 224,   2,   0, 228, 128, 
      4,   0, 228, 128,   1,   0, 
    228, 160,   1,   0,   0,   2, 
      0,   0,   8, 128,   0,   0, 
    255, 144,   9,   0,   0,   3, 
      2,   0,   1, 128,   0,   0, 
    228, 128,  15,   0, 228, 160, 
      9,   0,   0,   3,   2,   0, 
      2, 128,   0,   0, 228, 128, 
     16,   0, 228, 160,   9,   0, 
      0,   3,   2,   0,   4, 128, 
      0,   0, 228, 128,  17,   0, 
    228, 160,   2,   0,   0,   3, 
      2,   0,   7, 128,   2,   0, 
    228, 129,  12,   0, 228, 160, 
     36,   0,   0,   2,   4,   0, 
      7, 128,   2,   0, 228, 128, 
      2,   0,   0,   3,   2,   0, 
      7, 128,   4,   0, 228, 128, 
      3,   0, 228, 161,  36,   0, 
      0,   2,   5,   0,   7, 128, 
      2,   0, 228, 128,   8,   0, 
      0,   3,   2,   0,   1, 128, 
      5,   0, 228, 128,   1,   0, 
    228, 128,   2,   0,   0,   3, 
      5,   0,   7, 128,   4,   0, 
    228, 128,   4,   0, 228, 161, 
      2,   0,   0,   3,   4,   0, 
      7, 128,   4,   0, 228, 128, 
      5,   0, 228, 161,  36,   0, 
      0,   2,   6,   0,   7, 128, 
      4,   0, 228, 128,   8,   0, 
      0,   3,   2,   0,   4, 128, 
      6,   0, 228, 128,   1,   0, 
    228, 128,  36,   0,   0,   2, 
      4,   0,   7, 128,   5,   0, 
    228, 128,   8,   0,   0,   3, 
      2,   0,   2, 128,   4,   0, 
    228, 128,   1,   0, 228, 128, 
     11,   0,   0,   3,   1,   0, 
      7, 128,   2,   0, 228, 128, 
    246,   0,  85, 160,   5,   0, 
      0,   3,   1,   0,   7, 128, 
      3,   0, 228, 128,   1,   0, 
    228, 128,  15,   0,   0,   2, 
      2,   0,   1, 128,   1,   0, 
      0, 128,  15,   0,   0,   2, 
      2,   0,   2, 128,   1,   0, 
     85, 128,  15,   0,   0,   2, 
      2,   0,   4, 128,   1,   0, 
    170, 128,   5,   0,   0,   3, 
      1,   0,   7, 128,   2,   0, 
    228, 128,   2,   0, 255, 160, 
     14,   0,   0,   2,   1,   0, 
      2, 128,   1,   0,  85, 128, 
      5,   0,   0,   3,   2,   0, 
      7, 128,   1,   0,  85, 128, 
     10,   0, 228, 160,  14,   0, 
      0,   2,   1,   0,   1, 128, 
      1,   0,   0, 128,  14,   0, 
      0,   2,   1,   0,   2, 128, 
      1,   0, 170, 128,   4,   0, 
      0,   4,   1,   0,  13, 128, 
      1,   0,   0, 128,   9,   0, 
    148, 160,   2,   0, 148, 128, 
      4,   0,   0,   4,   1,   0, 
      7, 128,   1,   0,  85, 128, 
     11,   0, 228, 160,   1,   0, 
    248, 128,   5,   0,   0,   3, 
      1,   0,   7, 224,   1,   0, 
    228, 128,   2,   0, 228, 160, 
      9,   0,   0,   3,   0,   0, 
      4, 192,   0,   0, 228, 128, 
     24,   0, 228, 160,   9,   0, 
      0,   3,   1,   0,   1, 128, 
      0,   0, 228, 128,  14,   0, 
    228, 160,  11,   0,   0,   3, 
      1,   0,   1, 128,   1,   0, 
      0, 128, 246,   0,  85, 160, 
     10,   0,   0,   3,   1,   0, 
      8, 224,   1,   0,   0, 128, 
    246,   0, 170, 160,   9,   0, 
      0,   3,   1,   0,   1, 128, 
      0,   0, 228, 128,  22,   0, 
    228, 160,   9,   0,   0,   3, 
      1,   0,   2, 128,   0,   0, 
    228, 128,  23,   0, 228, 160, 
      9,   0,   0,   3,   0,   0, 
      1, 128,   0,   0, 228, 128, 
     25,   0, 228, 160,   4,   0, 
      0,   4,   0,   0,   3, 192, 
      0,   0,   0, 128, 245,   0, 
    228, 160,   1,   0, 228, 128, 
      1,   0,   0,   2,   0,   0, 
      8, 192,   0,   0,   0, 128, 
      1,   0,   0,   2,   0,   0, 
      8, 224,   0,   0, 255, 160, 
      1,   0,   0,   2,   2,   0, 
      3, 224,   2,   0, 228, 144, 
    255, 255,   0,   0,  83,  72, 
     68,  82,  28,  10,   0,   0, 
     64,   0,   1,   0, 135,   2, 
      0,   0,  89,   8,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0, 245,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     95,   0,   0,   3, 114,  16, 
     16,   0,   1,   0,   0,   0, 
     95,   0,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   3,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   4,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3,  50,  32, 
     16,   0,   2,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   3,   0,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   2,   6,   0,   0,   0, 
     38,   0,   0,  11,   0, 208, 
      0,   0, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   3,   0,   0,   0, 
      2,  64,   0,   0,   3,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  56,   0,   0,  10, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  86,  21,  16,   0, 
      4,   0,   0,   0,  70, 142, 
     32,   6,   0,   0,   0,   0, 
     26,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  26,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   6,  16,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  26,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0, 166,  26,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  26,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     16,   0,   0,   7,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,  10, 
    242,   0,  16,   0,   3,   0, 
      0,   0,  86,  21,  16,   0, 
      4,   0,   0,   0,  70, 142, 
     32,   6,   0,   0,   0,   0, 
     27,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   3,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  27,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   6,  16,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   3,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  27,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0, 166,  26,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   3,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  27,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     16,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      3,   0,   0,   0,  17,   0, 
      0,   7,  34,   0,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   3,   0, 
      0,   0,  56,   0,   0,  10, 
    242,   0,  16,   0,   3,   0, 
      0,   0,  86,  21,  16,   0, 
      4,   0,   0,   0,  70, 142, 
     32,   6,   0,   0,   0,   0, 
     28,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   3,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  28,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   6,  16,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   3,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  28,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0, 166,  26,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     50,   0,   0,  12, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  28,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      4,   0,   0,   0,  70,  14, 
     16,   0,   3,   0,   0,   0, 
     16,   0,   0,   7,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   7,  66,   0,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  16,   0,   0,   8, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  70, 130, 
     32,   0,   0,   0,   0,   0, 
     19,   0,   0,   0,  16,   0, 
      0,   8,  34,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   2,   0,   0,   0, 
     70, 130,  32,   0,   0,   0, 
      0,   0,  20,   0,   0,   0, 
     16,   0,   0,   8,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0,  70, 130,  32,   0, 
      0,   0,   0,   0,  21,   0, 
      0,   0,  16,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     68,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    114,   0,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     16,   0,   0,   9,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     70, 130,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     16,   0,   0,   9,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     70, 130,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     16,   0,   0,   9,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     70, 130,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     29,   0,   0,  10, 114,   0, 
     16,   0,   3,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,  10, 114,   0,  16,   0, 
      3,   0,   0,   0,  70,   2, 
     16,   0,   3,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
    128,  63,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    114,   0,  16,   0,   2,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  70,   2, 
     16,   0,   3,   0,   0,   0, 
     56,   0,   0,   8, 114,   0, 
     16,   0,   4,   0,   0,   0, 
     86,   5,  16,   0,   2,   0, 
      0,   0,  70, 130,  32,   0, 
      0,   0,   0,   0,   7,   0, 
      0,   0,  50,   0,   0,  10, 
    178,   0,  16,   0,   2,   0, 
      0,   0,   6,   0,  16,   0, 
      2,   0,   0,   0,  70, 136, 
     32,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,  70,   8, 
     16,   0,   4,   0,   0,   0, 
     50,   0,   0,  10, 114,   0, 
     16,   0,   2,   0,   0,   0, 
    166,  10,  16,   0,   2,   0, 
      0,   0,  70, 130,  32,   0, 
      0,   0,   0,   0,   8,   0, 
      0,   0,  70,   3,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  11, 114,  32,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   2,   0,   0,   0, 
     70, 130,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     70, 130,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   6, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     58, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     58,  16,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  17,   0, 
      0,   8,  34,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,  16,   0,   0,   0, 
     17,   0,   0,   8,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   0,   0,   0,   0,   9, 
    114,   0,  16,   0,   2,   0, 
      0,   0,  70,   2,  16, 128, 
     65,   0,   0,   0,   2,   0, 
      0,   0,  70, 130,  32,   0, 
      0,   0,   0,   0,  12,   0, 
      0,   0,  16,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  70,   2, 
     16,   0,   2,   0,   0,   0, 
     68,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,  11, 
    114,   0,  16,   0,   4,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70, 130,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  16,   0, 
      0,   7, 130,   0,  16,   0, 
      2,   0,   0,   0,  70,   2, 
     16,   0,   4,   0,   0,   0, 
     70,   2,  16,   0,   4,   0, 
      0,   0,  68,   0,   0,   5, 
    130,   0,  16,   0,   2,   0, 
      0,   0,  58,   0,  16,   0, 
      2,   0,   0,   0,  56,   0, 
      0,   7, 114,   0,  16,   0, 
      4,   0,   0,   0, 246,  15, 
     16,   0,   2,   0,   0,   0, 
     70,   2,  16,   0,   4,   0, 
      0,   0,  16,   0,   0,   7, 
     18,   0,  16,   0,   4,   0, 
      0,   0,  70,   2,  16,   0, 
      4,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  11, 114,   0, 
     16,   0,   5,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70, 130, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  50,   0,   0,  11, 
    114,   0,  16,   0,   2,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70, 130,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,  16,   0, 
      0,   7, 130,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   5,   0,   0,   0, 
     70,   2,  16,   0,   5,   0, 
      0,   0,  68,   0,   0,   5, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,   0,  16,   0, 
      5,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   5,   0, 
      0,   0,  16,   0,   0,   7, 
     34,   0,  16,   0,   4,   0, 
      0,   0,  70,   2,  16,   0, 
      5,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     16,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  68,   0, 
      0,   5, 130,   0,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,   0, 
     16,   0,   2,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  16,   0, 
      0,   7,  66,   0,  16,   0, 
      4,   0,   0,   0,  70,   2, 
     16,   0,   2,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  52,   0,   0,  10, 
    114,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      4,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   3,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  47,   0, 
      0,   5, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   8, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0, 246, 143,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  25,   0,   0,   5, 
    114,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   8, 114,   0,  16,   0, 
      2,   0,   0,   0,  86,   5, 
     16,   0,   0,   0,   0,   0, 
     70, 130,  32,   0,   0,   0, 
      0,   0,  10,   0,   0,   0, 
     50,   0,   0,  10, 178,   0, 
     16,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,  70, 136,  32,   0, 
      0,   0,   0,   0,   9,   0, 
      0,   0,  70,   8,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  10, 114,   0,  16,   0, 
      0,   0,   0,   0, 166,  10, 
     16,   0,   0,   0,   0,   0, 
     70, 130,  32,   0,   0,   0, 
      0,   0,  11,   0,   0,   0, 
     70,   3,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
    114,  32,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  70, 130, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  17,  32, 
      0,   8, 130,  32,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,  14,   0,   0,   0, 
     54,   0,   0,   5,  50,  32, 
     16,   0,   2,   0,   0,   0, 
     70,  16,  16,   0,   2,   0, 
      0,   0,  17,   0,   0,   8, 
     18,  32,  16,   0,   3,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
     22,   0,   0,   0,  17,   0, 
      0,   8,  34,  32,  16,   0, 
      3,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,  23,   0,   0,   0, 
     17,   0,   0,   8,  66,  32, 
     16,   0,   3,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,  24,   0, 
      0,   0,  17,   0,   0,   8, 
    130,  32,  16,   0,   3,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
     25,   0,   0,   0,  62,   0, 
      0,   1,  73,  83,  71,  78, 
    184,   0,   0,   0,   5,   0, 
      0,   0,   8,   0,   0,   0, 
    128,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
    140,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   7,   7,   0,   0, 
    147,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
    156,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,  15,  15,   0,   0, 
    169,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   4,   0, 
      0,   0,  15,  15,   0,   0, 
     83,  86,  95,  80, 111, 115, 
    105, 116, 105, 111, 110,   0, 
     78,  79,  82,  77,  65,  76, 
      0,  84,  69,  88,  67,  79, 
     79,  82,  68,   0,  66,  76, 
     69,  78,  68,  73,  78,  68, 
     73,  67,  69,  83,   0,  66, 
     76,  69,  78,  68,  87,  69, 
     73,  71,  72,  84,   0, 171, 
    171, 171,  79,  83,  71,  78, 
    132,   0,   0,   0,   4,   0, 
      0,   0,   8,   0,   0,   0, 
    104,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
    104,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
    110,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,  12,   0,   0, 
    119,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  15,   0,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171
};