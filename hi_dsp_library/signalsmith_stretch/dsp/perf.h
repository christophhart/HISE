#ifndef SIGNALSMITH_DSP_PERF_H
#define SIGNALSMITH_DSP_PERF_H

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

/** @} */
}} // signalsmith::perf::

#endif // include guard
