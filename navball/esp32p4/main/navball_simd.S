/* ESP32-P4 PIE (128-bit SIMD, 8 x int16 lanes) kernel of the navball renderer.
 *
 * void navball_simd_idx(const int16_t *xq, const int16_t *zq, int16_t *idx, int n8, const int16_t *k)
 *   xq  : x / r in Q15, one per pixel            (16-byte aligned)
 *   zq  : sqrt(1 - (x^2 + y^2) / r^2) in Q15     (16-byte aligned)
 *   idx : out, shade table index per pixel       (16-byte aligned)
 *   n8  : number of 8-pixel groups (> 0)
 * (PIE instructions only accept x8..x15 = s0, s1, a0..a5 as scalar / address registers.)
 *   k   : constants, read in this order (int16 each, broadcast to the 8 lanes):
 *         R3 R5 BY ZERO K1024 M4096 R0 R2 AY R6 R8 CYY C30 HALF EPS M2048
 *
 * Per pixel (same maths as the scalar loop in navball.c):
 *   vy  = R3*x + R5*z + BY                  (Q15 products: (a*b) >> 15, SAR = 15)
 *   idx = |vy| >> 5  |  (vy < 0 ? 4096 : 0)
 *   a   = |R0*x + R2*z + AY|,  b = |R6*x + R8*z + CYY|
 *   lo  = min(a, b), hi = max(a, b), d = |lo*cos30 - hi/2|
 *   idx |= (lo < EPS || d < EPS) ? 2048 : 0  (meridian)
 * The C reference of this kernel is navball_simd_idx_ref() in navball.c. */

    .text
    .align 4
    .global navball_simd_idx
    .type navball_simd_idx, @function
navball_simd_idx:
    li      a5, 15
    esp.movx.w.sar a5
.Lloop:
    mv      a5, a4
    esp.vld.128.ip   q0, a0, 16          /* x */
    esp.vld.128.ip   q1, a1, 16          /* z */
    /* vy */
    esp.vldbc.16.ip  q7, a5, 2           /* R3 */
    esp.vmul.s16     q2, q0, q7
    esp.vldbc.16.ip  q7, a5, 2           /* R5 */
    esp.vmul.s16     q3, q1, q7
    esp.vadd.s16     q2, q2, q3
    esp.vldbc.16.ip  q7, a5, 2           /* BY */
    esp.vadd.s16     q2, q2, q7          /* q2 = vy */
    esp.vldbc.16.ip  q7, a5, 2           /* ZERO */
    esp.vcmp.lt.s16  q4, q2, q7          /* q4 = vy < 0 */
    esp.vabs.16      q2, q2              /* |vy| */
    esp.vldbc.16.ip  q7, a5, 2           /* K1024: (v * 1024) >> 15 = v >> 5 */
    esp.vmul.s16     q2, q2, q7
    esp.vldbc.16.ip  q7, a5, 2           /* M4096 */
    esp.andq         q4, q4, q7
    esp.orq          q2, q2, q4          /* q2 = shade index */
    /* a = |vx| */
    esp.vldbc.16.ip  q7, a5, 2           /* R0 */
    esp.vmul.s16     q3, q0, q7
    esp.vldbc.16.ip  q7, a5, 2           /* R2 */
    esp.vmul.s16     q4, q1, q7
    esp.vadd.s16     q3, q3, q4
    esp.vldbc.16.ip  q7, a5, 2           /* AY */
    esp.vadd.s16     q3, q3, q7
    esp.vabs.16      q3, q3
    /* b = |vz| */
    esp.vldbc.16.ip  q7, a5, 2           /* R6 */
    esp.vmul.s16     q5, q0, q7
    esp.vldbc.16.ip  q7, a5, 2           /* R8 */
    esp.vmul.s16     q4, q1, q7
    esp.vadd.s16     q5, q5, q4
    esp.vldbc.16.ip  q7, a5, 2           /* CYY */
    esp.vadd.s16     q5, q5, q7
    esp.vabs.16      q5, q5
    /* fold into [0, 45] deg: lo / hi, distance to the 30 deg meridian */
    esp.vmax.s16     q6, q3, q5          /* hi */
    esp.vmin.s16     q3, q3, q5          /* lo */
    esp.vldbc.16.ip  q7, a5, 2           /* C30 */
    esp.vmul.s16     q5, q3, q7
    esp.vldbc.16.ip  q7, a5, 2           /* HALF */
    esp.vmul.s16     q6, q6, q7
    esp.vsub.s16     q5, q5, q6
    esp.vabs.16      q5, q5              /* |d| */
    esp.vldbc.16.ip  q7, a5, 2           /* EPS */
    esp.vcmp.lt.s16  q3, q3, q7          /* lo < eps */
    esp.vcmp.lt.s16  q5, q5, q7          /* |d| < eps */
    esp.orq          q3, q3, q5
    esp.vldbc.16.ip  q7, a5, 2           /* M2048 */
    esp.andq         q3, q3, q7
    esp.orq          q2, q2, q3
    esp.vst.128.ip   q2, a2, 16
    addi    a3, a3, -1
    bnez    a3, .Lloop
    ret
    .size navball_simd_idx, . - navball_simd_idx
