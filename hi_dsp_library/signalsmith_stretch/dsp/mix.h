#ifndef SIGNALSMITH_DSP_MULTI_CHANNEL_H
#define SIGNALSMITH_DSP_MULTI_CHANNEL_H

#include "./common.h"

#include <array>

namespace signalsmith {
namespace mix {
	/**	@defgroup Mix Multichannel mixing
		@brief Utilities for stereo/multichannel mixing operations

		@{
		@file
	*/

	/** @defgroup Matrices Orthogonal matrices
		@brief Some common matrices used for audio
		@ingroup Mix
		@{ */

	/// @brief Hadamard: high mixing levels, N log(N) operations
	template<typename Sample, int size=-1>
	class Hadamard {
	public:
		static_assert(size >= 0, "Size must be positive (or -1 for dynamic)");
		/// Applies the matrix, scaled so it's orthogonal
		template<class Data>
		static void inPlace(Data &&data) {
			unscaledInPlace(data);
			
			Sample factor = scalingFactor();
			for (int c = 0; c < size; ++c) {
				data[c] *= factor;
			}
		}

		/// Scaling factor applied to make it orthogonal
		static Sample scalingFactor() {
			/// TODO: test for C++20, or whatever makes this constexpr.  Maybe a `#define` in `common.h`?
			return std::sqrt(Sample(1)/(size ? size : 1));
		}

		/// Skips the scaling, so it's a matrix full of `1`s
		template<class Data, int startIndex=0>
		static void unscaledInPlace(Data &&data) {
			if (size <= 1) return;
			constexpr int hSize = size/2;

			Hadamard<Sample, hSize>::template unscaledInPlace<Data, startIndex>(data);
			Hadamard<Sample, hSize>::template unscaledInPlace<Data, startIndex + hSize>(data);

			for (int i = 0; i < hSize; ++i) {
				Sample a = data[i + startIndex], b = data[i + startIndex + hSize];
				data[i + startIndex] = (a + b);
				data[i + startIndex + hSize] = (a - b);
			}
		}
	};
	/// @brief Hadamard with dynamic size
	template<typename Sample>
	class Hadamard<Sample, -1> {
		int size;
	public:
		Hadamard(int size) : size(size) {}
		
		/// Applies the matrix, scaled so it's orthogonal
		template<class Data>
		void inPlace(Data &&data) const {
			unscaledInPlace(data);
			
			Sample factor = scalingFactor();
			for (int c = 0; c < size; ++c) {
				data[c] *= factor;
			}
		}

		/// Scaling factor applied to make it orthogonal
		Sample scalingFactor() const {
			return std::sqrt(Sample(1)/(size ? size : 1));
		}

		/// Skips the scaling, so it's a matrix full of `1`s
		template<class Data>
		void unscaledInPlace(Data &&data) const {
			int hSize = size/2;
			while (hSize > 0) {
				for (int startIndex = 0; startIndex < size; startIndex += hSize*2) {
					for (int i = startIndex; i < startIndex + hSize; ++i) {
						Sample a = data[i], b = data[i + hSize];
						data[i] = (a + b);
						data[i + hSize] = (a - b);
					}
				}
				hSize /= 2;
			}
		}
	};
	/// @brief Householder: moderate mixing, 2N operations
	template<typename Sample, int size=-1>
	class Householder {
	public:
		static_assert(size >= 0, "Size must be positive (or -1 for dynamic)");
		template<class Data>
		static void inPlace(Data &&data) {
			if (size < 1) return;
			/// TODO: test for C++20, which makes `std::complex::operator/` constexpr
			const Sample factor = Sample(-2)/Sample(size ? size : 1);

			Sample sum = data[0];
			for (int i = 1; i < size; ++i) {
				sum += data[i];
			}
			sum *= factor;
			for (int i = 0; i < size; ++i) {
				data[i] += sum;
			}
		}
		/// @deprecated The matrix is already orthogonal, but this is here for compatibility with Hadamard
		constexpr static Sample scalingFactor() {
			return 1;
		}
	};
	/// @brief Householder with dynamic size
	template<typename Sample>
	class Householder<Sample, -1> {
		int size;
	public:
		Householder(int size) : size(size) {}
	
		template<class Data>
		void inPlace(Data &&data) const {
			if (size < 1) return;
			const Sample factor = Sample(-2)/Sample(size ? size : 1);

			Sample sum = data[0];
			for (int i = 1; i < size; ++i) {
				sum += data[i];
			}
			sum *= factor;
			for (int i = 0; i < size; ++i) {
				data[i] += sum;
			}
		}
		/// @deprecated The matrix is already orthogonal, but this is here for compatibility with Hadamard
		constexpr static Sample scalingFactor() {
			return 1;
		}
	};
	///  @}
	
	/** @brief Upmix/downmix a stereo signal to an (even) multi-channel signal
		
		When spreading out, it rotates the input by various amounts (e.g. a four-channel signal would produce `(left, right, mid side)`), such that energy is preserved for each pair.
		
		When mixing together, it uses the opposite rotations, such that upmix → downmix produces the same stereo signal (when scaled by `.scalingFactor1()`.
	*/
	template<typename Sample, int channels>
	class StereoMultiMixer {
		static_assert((channels/2)*2 == channels, "StereoMultiMixer must have an even number of channels");
		static_assert(channels > 0, "StereoMultiMixer must have a positive number of channels");
		static constexpr int hChannels = channels/2;
		std::array<Sample, channels> coeffs;
	public:
		StereoMultiMixer() {
			coeffs[0] = 1;
			coeffs[1] = 0;
			for (int i = 1; i < hChannels; ++i) {
				double phase = M_PI*i/channels;
				coeffs[2*i] = std::cos(phase);
				coeffs[2*i + 1] = std::sin(phase);
			}
		}
		
		template<class In, class Out>
		void stereoToMulti(In &input, Out &output) const {
			output[0] = input[0];
			output[1] = input[1];
			for (int i = 2; i < channels; i += 2) {
				output[i] = input[0]*coeffs[i] + input[1]*coeffs[i + 1];
				output[i + 1] = input[1]*coeffs[i] - input[0]*coeffs[i + 1];
			}
		}
		template<class In, class Out>
		void multiToStereo(In &input, Out &output) const {
			output[0] = input[0];
			output[1] = input[1];
			for (int i = 2; i < channels; i += 2) {
				output[0] += input[i]*coeffs[i] - input[i + 1]*coeffs[i + 1];
				output[1] += input[i + 1]*coeffs[i] + input[i]*coeffs[i + 1];
			}
		}
		/// Scaling factor for the downmix, if channels are phase-aligned
		static constexpr Sample scalingFactor1() {
			return 2/Sample(channels);
		}
		/// Scaling factor for the downmix, if channels are independent
		static Sample scalingFactor2() {
			return std::sqrt(scalingFactor1());
		}
	};
	
	/// A cheap (polynomial) almost-energy-preserving crossfade
	/// Maximum energy error: 1.06%, average 0.64%, curves overshoot by 0.3%
	/// See: http://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
	template<typename Sample, typename Result>
	void cheapEnergyCrossfade(Sample x, Result &toCoeff, Result &fromCoeff) {
		Sample x2 = 1 - x;
		// Other powers p can be approximated by: k = -6.0026608 + p*(6.8773512 - 1.5838104*p)
		Sample A = x*x2, B = A*(1 + (Sample)1.4186*A);
		Sample C = (B + x), D = (B + x2);
		toCoeff = C*C;
		fromCoeff = D*D;
	}

/** @} */
}} // signalsmith::delay::
#endif // include guard
