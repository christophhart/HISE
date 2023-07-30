#ifndef SIGNALSMITH_DSP_PERF_H
#define SIGNALSMITH_DSP_PERF_H

#if USE_VDSP_COMPLEX_MUL && JUCE_MAC
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

    template <typename V>
    SIGNALSMITH_INLINE V complexReal(const std::complex<V>& c) {
        return ((V*)(&c))[0];
    }
    template <typename V>
    SIGNALSMITH_INLINE V complexImag(const std::complex<V>& c) {
        return ((V*)(&c))[1];
    }



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


#if ENABLE_JUCE_VECTOR_OPS
    template <typename V> using SimdType = juce::dsp::SIMDRegister<V>;

    template <typename V> using SimdComplexType = std::complex<SimdType<V>>;

    template <typename V> static SimdComplexType<V> loadWithStride(const std::complex<V>* ptr, int simdIndex, int stride)
    {
        constexpr auto SimdSize = SimdType<V>::size();

        V re[SimdSize];
        V im[SimdSize];

        for (int i = 0; i < SimdSize; i++)
        {
            re[i] = ptr[(simdIndex * SimdSize + i) * stride].real();
            im[i] = ptr[(simdIndex * SimdSize + i) * stride].imag();
        }

        return { SimdType<V>::fromRawArray(re), SimdType<V>::fromRawArray(im) };
    }

    template <typename V> static void writeWithStride(std::complex<V>* ptr, int simdIndex, int stride, const SimdComplexType<V>& value)
    {
        constexpr auto SimdSize = SimdType<V>::size();

        V re[SimdSize];
        V im[SimdSize];

        value.real().copyToRawArray(re);
        value.imag().copyToRawArray(im);

        for (int i = 0; i < SimdSize; i++)
        {
            ptr[(simdIndex * SimdSize + i) * stride].real(re[i]);
            ptr[(simdIndex * SimdSize + i) * stride].imag(im[i]);
        }
    }
#endif


    template <bool conjugateSecond=false, typename V> void mulVec(const std::complex<V>* a, int strideA, const std::complex<V>* b, int strideB, std::complex<V>* c, int strideC, int numElements)
    {
        TRACE_EVENT("dsp", "mulConj");
        
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

        
#if ENABLE_JUCE_VECTOR_OPS


        for (int i = 0; i < numElements / SimdType<V>::size(); i++)
        {
            auto a_ = loadWithStride(a, i, strideA);
            auto b_ = loadWithStride(b, i, strideA);

            auto c_ = mul<conjugateSecond, SimdType<V>>(a_, b_);

            writeWithStride(c, i, strideC, c_);
        }

#else
        for(int i = 0; i < numElements; i++)
        {
            c[i * strideC] = mul<conjugateSecond, V>(a[i*strideA], b[i*strideB]);
        }
#endif
#endif
    }

/** @} */
}} // signalsmith::perf::

#endif // include guard
