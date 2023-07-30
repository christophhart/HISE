#ifndef SIGNALSMITH_DSP_PERF_H
#define SIGNALSMITH_DSP_PERF_H

#if USE_VDSP_COMPLEX_MUL
#define Point DummyPoint
#define Component DummyComponent
#define MemoryBlock DummyMB
#include <Accelerate/Accelerate.h>
#undef Point
#undef Component
#undef MemoryBlock
#endif

#include <complex>

namespace signalsmith {
namespace perf {
	/**	@defgroup Performance Performance helpers
		@brief Nothing serious, just some `#defines` and helpers
		
		@{
		@file
	*/
		
	/// *Really* insist that a function/method is inlined (mostly for performance in DEBUG builds)
	#ifndef SIGNALSMITH_INLINE
	#ifdef __GNUC__
	#define SIGNALSMITH_INLINE __attribute__((always_inline)) inline
	#elif defined(__MSVC__)
	#define SIGNALSMITH_INLINE __forceinline inline
	#else
	#define SIGNALSMITH_INLINE inline
	#endif
	#endif




    template<bool flipped, typename V>
    SIGNALSMITH_INLINE std::complex<V> add(const std::complex<V> &a, const std::complex<V> &b) {
        V aReal = complexReal(a), aImag = complexImag(a);
        V bReal = complexReal(b), bImag = complexImag(b);
        return flipped ? std::complex<V>{
            aReal + bImag,
            aImag - bReal
        } : std::complex<V>{
            aReal - bImag,
            aImag + bReal
        };
    }

	/** @brief Complex-multiplication (with optional conjugate second-arg), without handling NaN/Infinity
		The `std::complex` multiplication has edge-cases around NaNs which slow things down and prevent auto-vectorisation.
	*/
	template <bool conjugateSecond=false, typename V>
	SIGNALSMITH_INLINE static std::complex<V> mul(const std::complex<V> &a, const std::complex<V> &b) {
		return conjugateSecond ? std::complex<V>{
			b.real()*a.real() + b.imag()*a.imag(),
			b.real()*a.imag() - b.imag()*a.real()
		} : std::complex<V>{
			a.real()*b.real() - a.imag()*b.imag(),
			a.real()*b.imag() + a.imag()*b.real()
		};
	}

    template <bool conjugateSecond=false, typename V> void mulVec(const std::complex<V>* a, int strideA, const std::complex<V>* b, int strideB, std::complex<V>* c, int strideC, int numElements)
    {
        TRACE_DSP();
        
#if USE_VDSP_COMPLEX_MUL
        
        strideA *= 2;
        strideB *= 2;
        strideC *= 2;
        
        auto unconstA = const_cast<std::complex<V>*>(a);
        auto unconstB = const_cast<std::complex<V>*>(b);
        
        DSPSplitComplex a_ = { reinterpret_cast<V*>(unconstA), reinterpret_cast<V*>(unconstA) + 1 };
        DSPSplitComplex b_ = { reinterpret_cast<V*>(unconstB), reinterpret_cast<V*>(unconstB) + 1 };
        DSPSplitComplex c_ = { reinterpret_cast<V*>(c), reinterpret_cast<V*>(c) + 1 };
        
        vDSP_zvmul(&a_, strideA, &b_, strideB, &c_, strideC, numElements, !conjugateSecond ? 1 : -1);
        
        
#else
        for(int i = 0; i < numElements; i++)
        {
            c[i * strideC] = mul<conjugateSecond, V>(a[i*strideA], b[i*strideB]);
        }
#endif
    }

/** @} */
}} // signalsmith::perf::

#endif // include guard
