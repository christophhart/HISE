#define USE_C_IMPLEMENTATION 1
#define __GNUC__ 5

#include <tcclib.h>
#include <TccLibrary.h>
#include <math.h>

long long __tcc_cvt_ftol(long double x)
{
	unsigned c0, c1;
	long long ret;
	__asm__ __volatile__("fnstcw %0" : "=m" (c0));
	c1 = c0 | 0x0C00;
	__asm__ __volatile__("fldcw %0" : : "m" (c1));
	__asm__ __volatile__("fistpll %0"  : "=m" (ret));
	__asm__ __volatile__("fldcw %0" : : "m" (c0));
	return ret;
}


#include "TccLibrary.cpp"