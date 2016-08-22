/** Wrapper file arount the TCC HISE library.
*
*	This file contains the definitions of all HISE specific TCC functions.
*
*
*/

#define USE_C_IMPLEMENTATION 1
#define TCC_HISE 0
#define TCC_CPP 0

#include <tcclib.h>
#include <stdbool.h>
#include <TccLibrary.h>
#include <kiss_fft/kiss_fft.h>
#include <kiss_fft/kiss_fftr.h>

#if TCC_WIN 
// Somehow this is missing in the command line library..
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
#endif

#include "TccLibrary.cpp"