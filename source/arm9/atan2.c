#include <stdio.h>
#include <stdint.h>
#include <nds.h>
#include <math.h>

uint32_t reciprocaldivf32(uint32_t a, uint32_t b);
uint32_t reciprocaldivf32_asm(uint32_t a, uint32_t b);
#define FRAC_BITS (12) //up to 16 should be permissible

int32_t fxarctan(int32_t x)
{
//Might be better as ARM_CODE in assembly with SMulWx?
//input is a 3.12 signed fixedpoint number in the range [-1,1]
    int32_t s=((int32_t)x*x)>>FRAC_BITS;
    int16_t c1=-0.0464964749f *(1<<FRAC_BITS);
    int16_t c2= 0.15931422f*(1<<FRAC_BITS);
    int16_t c3=-0.327622764f*(1<<FRAC_BITS);
    int32_t t=(c1 *(int32_t) s)>>FRAC_BITS;
    t+=c2;   
    t=((int32_t)t*s)>>FRAC_BITS;
    t+=c3;
    t=((int32_t)t*s)>>FRAC_BITS;
    t=((int32_t)t*x)>>FRAC_BITS;
    t+=x;
//output is a 3.12 signed fixedpoint number
    return t;
}



ARM_CODE int32_t fxatan2(int32_t y, int32_t x)
{
//should probably be arm_code?
//input is two signed 32-bit numbers
    uint32_t xt=x;
    uint32_t yt=y;
    if (x<0)
        xt=-xt;

    if(y<0)
        yt=-yt;

    uint32_t a  = reciprocaldivf32_asm ((xt<yt ? xt : yt) , (xt<yt ? yt: xt) );
    
    int32_t r = fxarctan((int32_t)a);
    uint32_t pi=(M_PI*(1<<FRAC_BITS));
    if (yt > xt)
        r = (pi>>1) - r;

    if (x < 0)
        r = pi - r;

    if (y < 0)
        r = -r;
    //output is a signed fixed point 4.12 number
    return r;
}
