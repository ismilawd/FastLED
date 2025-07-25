/// @file noise.cpp
/// Functions to generate and fill arrays with noise.

#include <string.h>
#include "fl/array.h"


/// Disables pragma messages and warnings
#define FASTLED_INTERNAL
#include "FastLED.h"


#include "fl/memfill.h"
// Compiler throws a warning about stack usage possibly being unbounded even
// though bounds are checked, silence that so users don't see it
#pragma GCC diagnostic push
#if defined(__GNUC__) && (__GNUC__ >= 6)
  #pragma GCC diagnostic ignored "-Wstack-usage="
#else
  #pragma GCC diagnostic ignored "-Wunknown-warning-option"
#endif



/// Reads a single byte from the p array
#define NOISE_P(x) FL_PGM_READ_BYTE_NEAR(noise_detail::p + x)

FASTLED_NAMESPACE_BEGIN

namespace noise_detail {

FL_PROGMEM static uint8_t const p[] = {
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
     57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
     74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
     60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
     65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
    200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
     52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
     81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,
    151};


// Start Doxygen define hiding
/// @cond

#if FASTLED_NOISE_ALLOW_AVERAGE_TO_OVERFLOW == 1
#define AVG15(U,V) (((U)+(V)) >> 1)
#else
// See if we should use the inlined avg15 for AVR with MUL instruction
#if defined(__AVR__) && (LIB8_ATTINY == 0)
#define AVG15(U,V) (noise_detail::avg15_inline_avr_mul((U),(V)))
// inlined copy of avg15 for AVR with MUL instruction; cloned from math8.h
// Forcing this inline in the 3-D 16bit noise produces a 12% speedup overall,
// at a cost of just +8 bytes of net code size.
static int16_t inline __attribute__((always_inline))  avg15_inline_avr_mul( int16_t i, int16_t j)
{
    asm volatile(
                 /* first divide j by 2, throwing away lowest bit */
                 "asr %B[j]          \n\t"
                 "ror %A[j]          \n\t"
                 /* now divide i by 2, with lowest bit going into C */
                 "asr %B[i]          \n\t"
                 "ror %A[i]          \n\t"
                 /* add j + C to i */
                 "adc %A[i], %A[j]   \n\t"
                 "adc %B[i], %B[j]   \n\t"
                 : [i] "+r" (i)
                 : [j] "r"  (j) );
    return i;
}
#else
#define AVG15(U,V) (avg15((U),(V)))
#endif
#endif

// See fastled_config.h for notes on this; 
// "#define FASTLED_NOISE_FIXED 1" is the correct value
#if FASTLED_NOISE_FIXED == 0
#define EASE8(x)  (FADE(x) )
#define EASE16(x) (FADE(x) )
#else
#define EASE8(x)  (ease8InOutQuad(x) )
#define EASE16(x) (ease16InOutQuad(x))
#endif
//
// #define FADE_12
#define FADE_16

#ifdef FADE_12
#define FADE logfade12
#define LERP(a,b,u) lerp15by12(a,b,u)
#else
#define FADE(x) scale16(x,x)
#define LERP(a,b,u) lerp15by16(a,b,u)
#endif

// end Doxygen define hiding
/// @endcond

} // namespace noise_detail

static int16_t inline __attribute__((always_inline))  grad16(uint8_t hash, int16_t x, int16_t y, int16_t z) {
#if 0
    switch(hash & 0xF) {
        case  0: return (( x) + ( y))>>1;
        case  1: return ((-x) + ( y))>>1;
        case  2: return (( x) + (-y))>>1;
        case  3: return ((-x) + (-y))>>1;
        case  4: return (( x) + ( z))>>1;
        case  5: return ((-x) + ( z))>>1;
        case  6: return (( x) + (-z))>>1;
        case  7: return ((-x) + (-z))>>1;
        case  8: return (( y) + ( z))>>1;
        case  9: return ((-y) + ( z))>>1;
        case 10: return (( y) + (-z))>>1;
        case 11: return ((-y) + (-z))>>1;
        case 12: return (( y) + ( x))>>1;
        case 13: return ((-y) + ( z))>>1;
        case 14: return (( y) + (-x))>>1;
        case 15: return ((-y) + (-z))>>1;
    }
#else
    hash = hash&15;
    int16_t u = hash<8?x:y;
    int16_t v = hash<4?y:hash==12||hash==14?x:z;
    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return AVG15(u,v);
#endif
}

static int16_t inline __attribute__((always_inline)) grad16(uint8_t hash, int16_t x, int16_t y) {
    hash = hash & 7;
    int16_t u,v;
    if(hash < 4) { u = x; v = y; } else { u = y; v = x; }
    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return AVG15(u,v);
}

static int16_t inline __attribute__((always_inline)) grad16(uint8_t hash, int16_t x) {
    hash = hash & 15;
    int16_t u,v;
    if(hash > 8) { u=x;v=x; }
    else if(hash < 4) { u=x;v=1; }
    else { u=1;v=x; }
    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return AVG15(u,v);
}

// selectBasedOnHashBit performs this:
//   result = (hash & (1<<bitnumber)) ? a : b
// but with an AVR asm version that's smaller and quicker than C
// (and probably not worth including in lib8tion)
static int8_t inline __attribute__((always_inline)) selectBasedOnHashBit(uint8_t hash, uint8_t bitnumber, int8_t a, int8_t b) {
	int8_t result;
#if !defined(__AVR__)
	result = (hash & (1<<bitnumber)) ? a : b;
#else
	asm volatile(
		"mov %[result],%[a]          \n\t"
		"sbrs %[hash],%[bitnumber]   \n\t"
		"mov %[result],%[b]          \n\t"
		: [result] "=r" (result)
		: [hash] "r" (hash),
		  [bitnumber] "M" (bitnumber),
          [a] "r" (a),
		  [b] "r" (b)
		);
#endif
	return result;
}

static int8_t  inline __attribute__((always_inline)) grad8(uint8_t hash, int8_t x, int8_t y, int8_t z) {
    // Industry-standard 3D Perlin noise gradient implementation
    // Uses proper 12 edge vectors of a cube for maximum range coverage
    
    switch(hash & 0xF) {
        case  0: return avg7( x,  y);  // (1,1,0)
        case  1: return avg7(-x,  y);  // (-1,1,0)
        case  2: return avg7( x, -y);  // (1,-1,0)
        case  3: return avg7(-x, -y);  // (-1,-1,0)
        case  4: return avg7( x,  z);  // (1,0,1)
        case  5: return avg7(-x,  z);  // (-1,0,1)
        case  6: return avg7( x, -z);  // (1,0,-1)
        case  7: return avg7(-x, -z);  // (-1,0,-1)
        case  8: return avg7( y,  z);  // (0,1,1)
        case  9: return avg7(-y,  z);  // (0,-1,1)
        case 10: return avg7( y, -z);  // (0,1,-1)
        case 11: return avg7(-y, -z);  // (0,-1,-1)
        // Repeat first 4 for hash values 12-15 (proper wrap-around)
        case 12: return avg7( x,  y);  // (1,1,0)
        case 13: return avg7(-x,  y);  // (-1,1,0)
        case 14: return avg7( x, -y);  // (1,-1,0)
        case 15: return avg7(-x, -y);  // (-1,-1,0)
    }
    return 0; // Should never reach here
}

static int8_t inline __attribute__((always_inline)) grad8(uint8_t hash, int8_t x, int8_t y)
{
    // since the tests below can be done bit-wise on the bottom
    // three bits, there's no need to mask off the higher bits
    //  hash = hash & 7;

    int8_t u,v;
    if( hash & 4) {
        u = y; v = x;
    } else {
        u = x; v = y;
    }

    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return avg7(u,v);
}

static int8_t inline __attribute__((always_inline)) grad8(uint8_t hash, int8_t x)
{
    // since the tests below can be done bit-wise on the bottom
    // four bits, there's no need to mask off the higher bits
    //	hash = hash & 15;

    int8_t u,v;
    if(hash & 8) {
        u=x; v=x;
    } else {
    if(hash & 4) {
        u=1; v=x;
    } else {
        u=x; v=1;
    }
    }

    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return avg7(u,v);
}


#ifdef FADE_12
uint16_t logfade12(uint16_t val) {
    return scale16(val,val)>>4;
}

static int16_t inline __attribute__((always_inline)) lerp15by12( int16_t a, int16_t b, fract16 frac)
{
    //if(1) return (lerp(frac,a,b));
    int16_t result;
    if( b > a) {
        uint16_t delta = b - a;
        uint16_t scaled = scale16(delta,frac<<4);
        result = a + scaled;
    } else {
        uint16_t delta = a - b;
        uint16_t scaled = scale16(delta,frac<<4);
        result = a - scaled;
    }
    return result;
}
#endif

static int8_t inline __attribute__((always_inline)) lerp7by8( int8_t a, int8_t b, fract8 frac)
{
    // int8_t delta = b - a;
    // int16_t prod = (uint16_t)delta * (uint16_t)frac;
    // int8_t scaled = prod >> 8;
    // int8_t result = a + scaled;
    // return result;
    int8_t result;
    if( b > a) {
        uint8_t delta = b - a;
        uint8_t scaled = scale8( delta, frac);
        result = a + scaled;
    } else {
        uint8_t delta = a - b;
        uint8_t scaled = scale8( delta, frac);
        result = a - scaled;
    }
    return result;
}

int16_t inoise16_raw(uint32_t x, uint32_t y, uint32_t z)
{
    // Find the unit cube containing the point
    uint8_t X = (x>>16)&0xFF;
    uint8_t Y = (y>>16)&0xFF;
    uint8_t Z = (z>>16)&0xFF;

    // Hash cube corner coordinates
    uint8_t A = NOISE_P(X)+Y;
    uint8_t AA = NOISE_P(A)+Z;
    uint8_t AB = NOISE_P(A+1)+Z;
    uint8_t B = NOISE_P(X+1)+Y;
    uint8_t BA = NOISE_P(B) + Z;
    uint8_t BB = NOISE_P(B+1)+Z;

    // Get the relative position of the point in the cube
    uint16_t u = x & 0xFFFF;
    uint16_t v = y & 0xFFFF;
    uint16_t w = z & 0xFFFF;

    // Get a signed version of the above for the grad function
    int16_t xx = (u >> 1) & 0x7FFF;
    int16_t yy = (v >> 1) & 0x7FFF;
    int16_t zz = (w >> 1) & 0x7FFF;
    uint16_t N = 0x8000L;

    u = EASE16(u); v = EASE16(v); w = EASE16(w);

    // skip the log fade adjustment for the moment, otherwise here we would
    // adjust fade values for u,v,w
    int16_t X1 = LERP(grad16(NOISE_P(AA), xx, yy, zz), grad16(NOISE_P(BA), xx - N, yy, zz), u);
    int16_t X2 = LERP(grad16(NOISE_P(AB), xx, yy-N, zz), grad16(NOISE_P(BB), xx - N, yy - N, zz), u);
    int16_t X3 = LERP(grad16(NOISE_P(AA+1), xx, yy, zz-N), grad16(NOISE_P(BA+1), xx - N, yy, zz-N), u);
    int16_t X4 = LERP(grad16(NOISE_P(AB+1), xx, yy-N, zz-N), grad16(NOISE_P(BB+1), xx - N, yy - N, zz - N), u);

    int16_t Y1 = LERP(X1,X2,v);
    int16_t Y2 = LERP(X3,X4,v);

    int16_t ans = LERP(Y1,Y2,w);

    return ans;
}

int16_t inoise16_raw(uint32_t x, uint32_t y, uint32_t z, uint32_t t) {
    // 1. Extract the integer (grid) parts.
    uint8_t X = (x >> 16) & 0xFF;
    uint8_t Y = (y >> 16) & 0xFF;
    uint8_t Z = (z >> 16) & 0xFF;
    uint8_t T = (t >> 16) & 0xFF;

    // 2. Extract the fractional parts.
    uint16_t u = x & 0xFFFF;
    uint16_t v = y & 0xFFFF;
    uint16_t w = z & 0xFFFF;
    uint16_t s = t & 0xFFFF;

    // 3. Easing of the fractional parts.
    u = EASE16(u);
    v = EASE16(v);
    w = EASE16(w);
    s = EASE16(s);

    uint16_t N = 0x8000L; // fixed-point half-scale

    // 4. Precompute fixed-point versions for the gradient evaluations.
    int16_t xx = (u >> 1) & 0x7FFF;
    int16_t yy = (v >> 1) & 0x7FFF;
    int16_t zz = (w >> 1) & 0x7FFF;

    // 5. Hash the 3D cube corners (the “base” for both t slices).
    uint8_t A = NOISE_P(X) + Y;
    uint8_t AA = NOISE_P(A) + Z;
    uint8_t AB = NOISE_P(A + 1) + Z;
    uint8_t B = NOISE_P(X + 1) + Y;
    uint8_t BA = NOISE_P(B) + Z;
    uint8_t BB = NOISE_P(B + 1) + Z;

    // 6. --- Lower t Slice (using T) ---
    uint8_t AAA = NOISE_P(AA) + T;
    uint8_t AAB = NOISE_P(AA + 1) + T;
    uint8_t ABA = NOISE_P(AB) + T;
    uint8_t ABB = NOISE_P(AB + 1) + T;
    uint8_t BAA = NOISE_P(BA) + T;
    uint8_t BAB = NOISE_P(BA + 1) + T;
    uint8_t BBA = NOISE_P(BB) + T;
    uint8_t BBB = NOISE_P(BB + 1) + T;

    int16_t L1 = LERP(grad16(AAA, xx, yy, zz),     grad16(BAA, xx - N, yy, zz), u);
    int16_t L2 = LERP(grad16(ABA, xx, yy - N, zz),  grad16(BBA, xx - N, yy - N, zz), u);
    int16_t L3 = LERP(grad16(AAB, xx, yy, zz - N),  grad16(BAB, xx - N, yy, zz - N), u);
    int16_t L4 = LERP(grad16(ABB, xx, yy - N, zz - N),  grad16(BBB, xx - N, yy - N, zz - N), u);

    int16_t Y1 = LERP(L1, L2, v);
    int16_t Y2 = LERP(L3, L4, v);
    int16_t noise_lower = LERP(Y1, Y2, w);

    // 7. --- Upper t Slice (using T+1) ---
    uint8_t Tupper = T + 1;
    uint8_t AAA_u = NOISE_P(AA) + Tupper;
    uint8_t AAB_u = NOISE_P(AA + 1) + Tupper;
    uint8_t ABA_u = NOISE_P(AB) + Tupper;
    uint8_t ABB_u = NOISE_P(AB + 1) + Tupper;
    uint8_t BAA_u = NOISE_P(BA) + Tupper;
    uint8_t BAB_u = NOISE_P(BA + 1) + Tupper;
    uint8_t BBA_u = NOISE_P(BB) + Tupper;
    uint8_t BBB_u = NOISE_P(BB + 1) + Tupper;

    int16_t U1 = LERP(grad16(AAA_u, xx, yy, zz),     grad16(BAA_u, xx - N, yy, zz), u);
    int16_t U2 = LERP(grad16(ABA_u, xx, yy - N, zz),  grad16(BBA_u, xx - N, yy - N, zz), u);
    int16_t U3 = LERP(grad16(AAB_u, xx, yy, zz - N),  grad16(BAB_u, xx - N, yy, zz - N), u);
    int16_t U4 = LERP(grad16(ABB_u, xx, yy - N, zz - N),  grad16(BBB_u, xx - N, yy - N, zz - N), u);

    int16_t V1 = LERP(U1, U2, v);
    int16_t V2 = LERP(U3, U4, v);
    int16_t noise_upper = LERP(V1, V2, w);

    // 8. Final interpolation in the t dimension.
    int16_t noise4d = LERP(noise_lower, noise_upper, s);

    return noise4d;
}

uint16_t inoise16(uint32_t x, uint32_t y, uint32_t z, uint32_t t) {
    int32_t ans = inoise16_raw(x,y,z,t);
    ans = ans + 19052L;
    uint32_t pan = ans;
    // pan = (ans * 220L) >> 7.  That's the same as:
    // pan = (ans * 440L) >> 8.  And this way avoids a 7X four-byte shift-loop on AVR.
    // Identical math, except for the highest bit, which we don't care about anyway,
    // since we're returning the 'middle' 16 out of a 32-bit value anyway.
    pan *= 440L;
    return (pan>>8);

    // return scale16by8(pan,220)<<1;
    // return ((inoise16_raw(x,y,z)+19052)*220)>>7;
    // return scale16by8(inoise16_raw(x,y,z)+19052,220)<<1;
}

uint16_t inoise16(uint32_t x, uint32_t y, uint32_t z) {
    int32_t ans = inoise16_raw(x,y,z);
    ans = ans + 19052L;
    uint32_t pan = ans;
    // pan = (ans * 220L) >> 7.  That's the same as:
    // pan = (ans * 440L) >> 8.  And this way avoids a 7X four-byte shift-loop on AVR.
    // Identical math, except for the highest bit, which we don't care about anyway,
    // since we're returning the 'middle' 16 out of a 32-bit value anyway.
    pan *= 440L;
    return (pan>>8);

    // // return scale16by8(pan,220)<<1;
    // return ((inoise16_raw(x,y,z)+19052)*220)>>7;
    // return scale16by8(inoise16_raw(x,y,z)+19052,220)<<1;
}

int16_t inoise16_raw(uint32_t x, uint32_t y)
{
    // Find the unit cube containing the point
    uint8_t X = x>>16;
    uint8_t Y = y>>16;

    // Hash cube corner coordinates
    uint8_t A = NOISE_P(X)+Y;
    uint8_t AA = NOISE_P(A);
    uint8_t AB = NOISE_P(A+1);
    uint8_t B = NOISE_P(X+1)+Y;
    uint8_t BA = NOISE_P(B);
    uint8_t BB = NOISE_P(B+1);

    // Get the relative position of the point in the cube
    uint16_t u = x & 0xFFFF;
    uint16_t v = y & 0xFFFF;

    // Get a signed version of the above for the grad function
    int16_t xx = (u >> 1) & 0x7FFF;
    int16_t yy = (v >> 1) & 0x7FFF;
    uint16_t N = 0x8000L;

    u = EASE16(u); v = EASE16(v);

    int16_t X1 = LERP(grad16(NOISE_P(AA), xx, yy), grad16(NOISE_P(BA), xx - N, yy), u);
    int16_t X2 = LERP(grad16(NOISE_P(AB), xx, yy-N), grad16(NOISE_P(BB), xx - N, yy - N), u);

    int16_t ans = LERP(X1,X2,v);

    return ans;
}

uint16_t inoise16(uint32_t x, uint32_t y) {
    int32_t ans = inoise16_raw(x,y);
    ans = ans + 17308L;
    uint32_t pan = ans;
    // pan = (ans * 242L) >> 7.  That's the same as:
    // pan = (ans * 484L) >> 8.  And this way avoids a 7X four-byte shift-loop on AVR.
    // Identical math, except for the highest bit, which we don't care about anyway,
    // since we're returning the 'middle' 16 out of a 32-bit value anyway.
    pan *= 484L;
    return (pan>>8);

    // return (uint32_t)(((int32_t)inoise16_raw(x,y)+(uint32_t)17308)*242)>>7;
    // return scale16by8(inoise16_raw(x,y)+17308,242)<<1;
}

int16_t inoise16_raw(uint32_t x)
{
    // Find the unit cube containing the point
    uint8_t X = x>>16;

    // Hash cube corner coordinates
    uint8_t A = NOISE_P(X);
    uint8_t AA = NOISE_P(A);
    uint8_t B = NOISE_P(X+1);
    uint8_t BA = NOISE_P(B);

    // Get the relative position of the point in the cube
    uint16_t u = x & 0xFFFF;

    // Get a signed version of the above for the grad function
    int16_t xx = (u >> 1) & 0x7FFF;
    uint16_t N = 0x8000L;

    u = EASE16(u);

    int16_t ans = LERP(grad16(NOISE_P(AA), xx), grad16(NOISE_P(BA), xx - N), u);

    return ans;
}

uint16_t inoise16(uint32_t x) {
    return ((uint32_t)((int32_t)inoise16_raw(x) + 17308L)) << 1;
}

int8_t inoise8_raw(uint16_t x, uint16_t y, uint16_t z)
{
    // Find the unit cube containing the point
    uint8_t X = x>>8;
    uint8_t Y = y>>8;
    uint8_t Z = z>>8;

    // Hash cube corner coordinates
    uint8_t A = NOISE_P(X)+Y;
    uint8_t AA = NOISE_P(A)+Z;
    uint8_t AB = NOISE_P(A+1)+Z;
    uint8_t B = NOISE_P(X+1)+Y;
    uint8_t BA = NOISE_P(B) + Z;
    uint8_t BB = NOISE_P(B+1)+Z;

    // Get the relative position of the point in the cube
    uint8_t u = x;
    uint8_t v = y;
    uint8_t w = z;

    // Get a signed version of the above for the grad function
    int8_t xx = ((uint8_t)(x)>>1) & 0x7F;
    int8_t yy = ((uint8_t)(y)>>1) & 0x7F;
    int8_t zz = ((uint8_t)(z)>>1) & 0x7F;
    uint8_t N = 0x80;

    u = EASE8(u); v = EASE8(v); w = EASE8(w);

    int8_t X1 = lerp7by8(grad8(NOISE_P(AA), xx, yy, zz), grad8(NOISE_P(BA), xx - N, yy, zz), u);
    int8_t X2 = lerp7by8(grad8(NOISE_P(AB), xx, yy-N, zz), grad8(NOISE_P(BB), xx - N, yy - N, zz), u);
    int8_t X3 = lerp7by8(grad8(NOISE_P(AA+1), xx, yy, zz-N), grad8(NOISE_P(BA+1), xx - N, yy, zz-N), u);
    int8_t X4 = lerp7by8(grad8(NOISE_P(AB+1), xx, yy-N, zz-N), grad8(NOISE_P(BB+1), xx - N, yy - N, zz - N), u);

    int8_t Y1 = lerp7by8(X1,X2,v);
    int8_t Y2 = lerp7by8(X3,X4,v);

    int8_t ans = lerp7by8(Y1,Y2,w);

    return ans;
}

uint8_t inoise8(uint16_t x, uint16_t y, uint16_t z) {
    //return scale8(76+(inoise8_raw(x,y,z)),215)<<1;
    int8_t n = inoise8_raw( x, y, z);  // -64..+64
    n+= 64;                            //   0..128
    uint8_t ans = qadd8( n, n);        //   0..255
    return ans;
}

int8_t inoise8_raw(uint16_t x, uint16_t y)
{
    // Find the unit cube containing the point
    uint8_t X = x>>8;
    uint8_t Y = y>>8;

    // Hash cube corner coordinates
    uint8_t A = NOISE_P(X)+Y;
    uint8_t AA = NOISE_P(A);
    uint8_t AB = NOISE_P(A+1);
    uint8_t B = NOISE_P(X+1)+Y;
    uint8_t BA = NOISE_P(B);
    uint8_t BB = NOISE_P(B+1);

    // Get the relative position of the point in the cube
    uint8_t u = x;
    uint8_t v = y;

    // Get a signed version of the above for the grad function
    int8_t xx = ((uint8_t)(x)>>1) & 0x7F;
    int8_t yy = ((uint8_t)(y)>>1) & 0x7F;
    uint8_t N = 0x80;

    u = EASE8(u); v = EASE8(v);

    int8_t X1 = lerp7by8(grad8(NOISE_P(AA), xx, yy), grad8(NOISE_P(BA), xx - N, yy), u);
    int8_t X2 = lerp7by8(grad8(NOISE_P(AB), xx, yy-N), grad8(NOISE_P(BB), xx - N, yy - N), u);

    int8_t ans = lerp7by8(X1,X2,v);

    return ans;
    // return scale8((70+(ans)),234)<<1;
}



uint8_t inoise8(uint16_t x, uint16_t y) {
  //return scale8(69+inoise8_raw(x,y),237)<<1;
    int8_t n = inoise8_raw( x, y);  // -64..+64
    n+= 64;                         //   0..128
    uint8_t ans = qadd8( n, n);     //   0..255
    return ans;
}

// output range = -64 .. +64
int8_t inoise8_raw(uint16_t x)
{
  // Find the unit cube containing the point
  uint8_t X = x>>8;

  // Hash cube corner coordinates
  uint8_t A = NOISE_P(X);
  uint8_t AA = NOISE_P(A);
  uint8_t B = NOISE_P(X+1);
  uint8_t BA = NOISE_P(B);

  // Get the relative position of the point in the cube
  uint8_t u = x;

  // Get a signed version of the above for the grad function
  int8_t xx = ((uint8_t)(x)>>1) & 0x7F;
  uint8_t N = 0x80;

  u = EASE8( u);
  
  int8_t ans = lerp7by8(grad8(NOISE_P(AA), xx), grad8(NOISE_P(BA), xx - N), u);

  return ans;
}

uint8_t inoise8(uint16_t x) {
    int8_t n = inoise8_raw(x);    //-64..+64
    n += 64;                      // 0..128
    uint8_t ans = qadd8(n,n);     // 0..255
    return ans;
}

// High-resolution 8-bit noise functions using inoise16 internally
uint8_t inoise8_hires(uint16_t x, uint16_t y, uint16_t z) {
    // Convert to 32-bit coordinates for inoise16 (scale by 256 for better precision)
    uint32_t xx = ((uint32_t)x) << 8;
    uint32_t yy = ((uint32_t)y) << 8;
    uint32_t zz = ((uint32_t)z) << 8;
    
    // Get 16-bit noise value
    uint16_t noise16 = inoise16(xx, yy, zz);
    
    // Scale down to 8-bit (shift right by 8)
    return noise16 >> 8;
}

uint8_t inoise8_hires(uint16_t x, uint16_t y) {
    // Convert to 32-bit coordinates for inoise16 (scale by 256 for better precision)
    uint32_t xx = ((uint32_t)x) << 8;
    uint32_t yy = ((uint32_t)y) << 8;
    
    // Get 16-bit noise value
    uint16_t noise16 = inoise16(xx, yy);
    
    // Scale down to 8-bit (shift right by 8)
    return noise16 >> 8;
}

uint8_t inoise8_hires(uint16_t x) {
    // Convert to 32-bit coordinates for inoise16 (scale by 256 for better precision)
    uint32_t xx = ((uint32_t)x) << 8;
    
    // Get 16-bit noise value
    uint16_t noise16 = inoise16(xx);
    
    // Scale down to 8-bit (shift right by 8)
    return noise16 >> 8;
}

// struct q44 {
//   uint8_t i:4;
//   uint8_t f:4;
//   q44(uint8_t _i, uint8_t _f) {i=_i; f=_f; }
// };

// uint32_t mul44(uint32_t v, q44 mulby44) {
//     return (v *mulby44.i)  + ((v * mulby44.f) >> 4);
// }
//
// uint16_t mul44_16(uint16_t v, q44 mulby44) {
//     return (v *mulby44.i)  + ((v * mulby44.f) >> 4);
// }

void fill_raw_noise8(uint8_t *pData, uint8_t num_points, uint8_t octaves, uint16_t x, int scale, uint16_t time) {
  uint32_t _xx = x;
  uint32_t scx = scale;
  for(int o = 0; o < octaves; ++o) {
    for(int i = 0,xx=_xx; i < num_points; ++i, xx+=scx) {
          pData[i] = qadd8(pData[i],inoise8(xx,time)>>o);
    }

    _xx <<= 1;
    scx <<= 1;
  }
}

void fill_raw_noise16into8(uint8_t *pData, uint8_t num_points, uint8_t octaves, uint32_t x, int scale, uint32_t time) {
  uint32_t _xx = x;
  uint32_t scx = scale;
  for(int o = 0; o < octaves; ++o) {
    for(int i = 0,xx=_xx; i < num_points; ++i, xx+=scx) {
      uint32_t accum = (inoise16(xx,time))>>o;
      accum += (pData[i]<<8);
      if(accum > 65535) { accum = 65535; }
      pData[i] = accum>>8;
    }

    _xx <<= 1;
    scx <<= 1;
  }
}

/// Fill a 2D 8-bit buffer with noise, using inoise8() 
/// @param pData the array of data to fill with noise values
/// @param width the width of the 2D buffer
/// @param height the height of the 2D buffer
/// @param octaves the number of octaves to use for noise. More octaves = more noise.
/// @param freq44 starting octave frequency
/// @param amplitude noise amplitude
/// @param skip how many noise maps to skip over, incremented recursively per octave
/// @param x x-axis coordinate on noise map (1D)
/// @param scalex the scale (distance) between x points when filling in noise
/// @param y y-axis coordinate on noise map (2D)
/// @param scaley the scale (distance) between y points when filling in noise
/// @param time the time position for the noise field
/// @todo Why isn't this declared in the header (noise.h)?
void fill_raw_2dnoise8(uint8_t *pData, int width, int height, uint8_t octaves, q44 freq44, fract8 amplitude, int skip, uint16_t x, int16_t scalex, uint16_t y, int16_t scaley, uint16_t time) {
  if(octaves > 1) {
    fill_raw_2dnoise8(pData, width, height, octaves-1, freq44, amplitude, skip+1, x*freq44, freq44 * scalex, y*freq44, freq44 * scaley, time);
  } else {
    // amplitude is always 255 on the lowest level
    amplitude=255;
  }

  scalex *= skip;
  scaley *= skip;

  fract8 invamp = 255-amplitude;
  uint16_t xx = x;
  for(int i = 0; i < height; ++i, y+=scaley) {
    uint8_t *pRow = pData + (i*width);
    xx = x;
    for(int j = 0; j < width; ++j, xx+=scalex) {
      uint8_t noise_base = inoise8(xx,y,time);
      noise_base = (0x80 & noise_base) ? (noise_base - 127) : (127 - noise_base);
      noise_base = scale8(noise_base<<1,amplitude);
      if(skip == 1) {
        pRow[j] = scale8(pRow[j],invamp) + noise_base;
      } else {
        for(int ii = i; ii<(i+skip) && ii<height; ++ii) {
          uint8_t *pRow = pData + (ii*width);
          for(int jj=j; jj<(j+skip) && jj<width; ++jj) {
            pRow[jj] = scale8(pRow[jj],invamp) + noise_base;
          }
        }
      }
    }
  }
}

void fill_raw_2dnoise8(uint8_t *pData, int width, int height, uint8_t octaves, uint16_t x, int scalex, uint16_t y, int scaley, uint16_t time) {
  fill_raw_2dnoise8(pData, width, height, octaves, q44(2,0), 128, 1, x, scalex, y, scaley, time);
}

void fill_raw_2dnoise16(uint16_t *pData, int width, int height, uint8_t octaves, q88 freq88, fract16 amplitude, int skip, uint32_t x, int32_t scalex, uint32_t y, int32_t scaley, uint32_t time) {
  if(octaves > 1) {
    fill_raw_2dnoise16(pData, width, height, octaves-1, freq88, amplitude, skip, x *freq88 , scalex *freq88, y * freq88, scaley * freq88, time);
  } else {
    // amplitude is always 255 on the lowest level
    amplitude=65535;
  }

  scalex *= skip;
  scaley *= skip;
  fract16 invamp = 65535-amplitude;
  for(int i = 0; i < height; i+=skip, y+=scaley) {
    uint16_t *pRow = pData + (i*width);
    for(int j = 0,xx=x; j < width; j+=skip, xx+=scalex) {
      uint16_t noise_base = inoise16(xx,y,time);
      noise_base = (0x8000 & noise_base) ? noise_base - (32767) : 32767 - noise_base;
      noise_base = scale16(noise_base<<1, amplitude);
      if(skip==1) {
        pRow[j] = scale16(pRow[j],invamp) + noise_base;
      } else {
        for(int ii = i; ii<(i+skip) && ii<height; ++ii) {
          uint16_t *pRow = pData + (ii*width);
          for(int jj=j; jj<(j+skip) && jj<width; ++jj) {
            pRow[jj] = scale16(pRow[jj],invamp) + noise_base;
          }
        }
      }
    }
  }
}

/// Unused
/// @todo Remove?
int32_t nmin=11111110;
/// Unused
/// @todo Remove?
int32_t nmax=0;

void fill_raw_2dnoise16into8(uint8_t *pData, int width, int height, uint8_t octaves, q44 freq44, fract8 amplitude, int skip, uint32_t x, int32_t scalex, uint32_t y, int32_t scaley, uint32_t time) {
  if(octaves > 1) {
    fill_raw_2dnoise16into8(pData, width, height, octaves-1, freq44, amplitude, skip+1, x*freq44, scalex *freq44, y*freq44, scaley * freq44, time);
  } else {
    // amplitude is always 255 on the lowest level
    amplitude=255;
  }

  scalex *= skip;
  scaley *= skip;
  uint32_t xx;
  fract8 invamp = 255-amplitude;
  for(int i = 0; i < height; i+=skip, y+=scaley) {
    uint8_t *pRow = pData + (i*width);
    xx = x;
    for(int j = 0; j < width; j+=skip, xx+=scalex) {
      uint16_t noise_base = inoise16(xx,y,time);
      noise_base = (0x8000 & noise_base) ? noise_base - (32767) : 32767 - noise_base;
      noise_base = scale8(noise_base>>7,amplitude);
      if(skip==1) {
        pRow[j] = qadd8(scale8(pRow[j],invamp),noise_base);
      } else {
        for(int ii = i; ii<(i+skip) && ii<height; ++ii) {
          uint8_t *pRow = pData + (ii*width);
          for(int jj=j; jj<(j+skip) && jj<width; ++jj) {
            pRow[jj] = scale8(pRow[jj],invamp) + noise_base;
          }
        }
      }
    }
  }
}

void fill_raw_2dnoise16into8(uint8_t *pData, int width, int height, uint8_t octaves, uint32_t x, int scalex, uint32_t y, int scaley, uint32_t time) {
  fill_raw_2dnoise16into8(pData, width, height, octaves, q44(2,0), 171, 1, x, scalex, y, scaley, time);
}

void fill_noise8(CRGB *leds, int num_leds,
            uint8_t octaves, uint16_t x, int scale,
            uint8_t hue_octaves, uint16_t hue_x, int hue_scale,
            uint16_t time) {

    if (num_leds <= 0) return;

    for (int j = 0; j < num_leds; j += 255) {
        const int LedsRemaining = num_leds - j;
        const int LedsPer = LedsRemaining > 255 ? 255 : LedsRemaining;  // limit to 255 max

        if (LedsPer <= 0) continue;
        FASTLED_STACK_ARRAY(uint8_t, V, LedsPer);
        FASTLED_STACK_ARRAY(uint8_t, H, LedsPer);

        fl::memfill(V, 0, LedsPer);
        fl::memfill(H, 0, LedsPer);

        fill_raw_noise8(V, LedsPer, octaves, x, scale, time);
        fill_raw_noise8(H, LedsPer, hue_octaves, hue_x, hue_scale, time);

        for (int i = 0; i < LedsPer; ++i) {
            leds[i + j] = CHSV(H[i], 255, V[i]);
        }
    }
}

void fill_noise16(CRGB *leds, int num_leds,
            uint8_t octaves, uint16_t x, int scale,
            uint8_t hue_octaves, uint16_t hue_x, int hue_scale,
            uint16_t time, uint8_t hue_shift) {

    if (num_leds <= 0) return;

    for (int j = 0; j < num_leds; j += 255) {
        const int LedsRemaining = num_leds - j;
        const int LedsPer = LedsRemaining > 255 ? 255 : LedsRemaining;  // limit to 255 max
        if (LedsPer <= 0) continue;
        FASTLED_STACK_ARRAY(uint8_t, V, LedsPer);
        FASTLED_STACK_ARRAY(uint8_t, H, LedsPer);

        fl::memfill(V, 0, LedsPer);
        fl::memfill(H, 0, LedsPer);

        fill_raw_noise16into8(V, LedsPer, octaves, x, scale, time);
        fill_raw_noise8(H, LedsPer, hue_octaves, hue_x, hue_scale, time);

        for (int i = 0; i < LedsPer; ++i) {
            leds[i + j] = CHSV(H[i] + hue_shift, 255, V[i]);
        }
    }
}

void fill_2dnoise8(CRGB *leds, int width, int height, bool serpentine,
            uint8_t octaves, uint16_t x, int xscale, uint16_t y, int yscale, uint16_t time,
            uint8_t hue_octaves, uint16_t hue_x, int hue_xscale, uint16_t hue_y, uint16_t hue_yscale,uint16_t hue_time,bool blend) {
  const size_t array_size = (size_t)height * width;
  if (array_size <= 0) return;
  FASTLED_STACK_ARRAY(uint8_t, V, array_size);
  FASTLED_STACK_ARRAY(uint8_t, H, array_size);

  fl::memfill(V,0,height*width);
  fl::memfill(H,0,height*width);

  fill_raw_2dnoise8((uint8_t*)V,width,height,octaves,x,xscale,y,yscale,time);
  fill_raw_2dnoise8((uint8_t*)H,width,height,hue_octaves,hue_x,hue_xscale,hue_y,hue_yscale,hue_time);

  int w1 = width-1;
  int h1 = height-1;
  for(int i = 0; i < height; ++i) {
    int wb = i*width;
    for(int j = 0; j < width; ++j) {
      CRGB led(CHSV(H[(h1-i)*width + (w1-j)], 255, V[i*width + j]));

      int pos = j;
      if(serpentine && (i & 0x1)) {
        pos = w1-j;
      }

      if(blend) {
        // Safer blending to avoid potential undefined behavior
        CRGB temp = leds[wb+pos];
        temp.nscale8(128); // Scale by 50%
        led.nscale8(128);
        leds[wb+pos] = temp + led;
      } else {
        leds[wb+pos] = led;
      }
    }
  }
}


void fill_2dnoise16(CRGB *leds, int width, int height, bool serpentine,
            uint8_t octaves, uint32_t x, int xscale, uint32_t y, int yscale, uint32_t time,
            uint8_t hue_octaves, uint16_t hue_x, int hue_xscale, uint16_t hue_y, uint16_t hue_yscale,uint16_t hue_time, bool blend, uint16_t hue_shift) {

  FASTLED_STACK_ARRAY(uint8_t, V, height*width);
  FASTLED_STACK_ARRAY(uint8_t, H, height*width);
  
  fl::memfill(V,0,height*width);
  fl::memfill(H,0,height*width);

  fill_raw_2dnoise16into8((uint8_t*)V,width,height,octaves,q44(2,0),171,1,x,xscale,y,yscale,time);
  // fill_raw_2dnoise16into8((uint8_t*)V,width,height,octaves,x,xscale,y,yscale,time);
  // fill_raw_2dnoise8((uint8_t*)V,width,height,hue_octaves,x,xscale,y,yscale,time);
  fill_raw_2dnoise8((uint8_t*)H,width,height,hue_octaves,hue_x,hue_xscale,hue_y,hue_yscale,hue_time);


  int w1 = width-1;
  int h1 = height-1;
  hue_shift >>= 8;

  for(int i = 0; i < height; ++i) {
    int wb = i*width;
    for(int j = 0; j < width; ++j) {
      CRGB led(CHSV(hue_shift + (H[(h1-i)*width + (w1-j)]), 196, V[i*width + j]));

      int pos = j;
      if(serpentine && (i & 0x1)) {
        pos = w1-j;
      }

      if(blend) {
        leds[wb+pos] >>= 1; leds[wb+pos] += (led>>=1);
      } else {
        leds[wb+pos] = led;
      }
    }
  }
}

FASTLED_NAMESPACE_END

#pragma GCC diagnostic pop
