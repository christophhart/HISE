//----------------------------------------------------------------------------------------
//
//	siv::PerlinNoise
//	Perlin noise library for modern C++
//
//	Copyright (C) 2013-2020 Ryo Suzuki <reputeless@gmail.com>
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------

# pragma once
# include <cstdint>
# include <algorithm>
# include <array>
# ifdef __cpp_concepts
# if __has_include(<concepts>)
# include <concepts>
# endif
# endif
# include <iterator>
# include <numeric>
# include <random>
# include <type_traits>

namespace siv
{
# ifdef __cpp_lib_concepts
	template <std::floating_point Float>
# else
	template <class Float>
# endif
	class BasicPerlinNoise
	{
	public:

		using value_type = Float;

	private:

		std::uint8_t p[512];

		
		static constexpr value_type Fade(value_type t) noexcept
		{
			return t * t * t * (t * (t * 6 - 15) + 10);
		}

		
		static constexpr value_type Lerp(value_type t, value_type a, value_type b) noexcept
		{
			return a + t * (b - a);
		}

		
		static constexpr value_type Grad(std::uint8_t hash, value_type x, value_type y, value_type z) noexcept
		{
			const std::uint8_t h = hash & 15;
			const value_type u = h < 8 ? x : y;
			const value_type v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}

		
		static constexpr value_type Weight(std::int32_t octaves) noexcept
		{
			value_type amp = 1;
			value_type value = 0;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				value += amp;
				amp /= 2;
			}

			return value;
		}

	public:

# if __has_cpp_attribute(nodiscard) >= 201907L
		
# endif
		explicit BasicPerlinNoise(std::uint32_t seed = std::default_random_engine::default_seed)
		{
			reseed(seed);
		}

# ifdef __cpp_lib_concepts
		template <std::uniform_random_bit_generator URNG>
# else
		template <class URNG, std::enable_if_t<!std::is_arithmetic_v<URNG>>* = nullptr>
# endif
# if __has_cpp_attribute(nodiscard) >= 201907L
		
# endif
		explicit BasicPerlinNoise(URNG&& urng)
		{
			reseed(std::forward<URNG>(urng));
		}

		void reseed(std::uint32_t seed)
		{
			for (size_t i = 0; i < 256; ++i)
			{
				p[i] = static_cast<std::uint8_t>(i);
			}

			std::shuffle(std::begin(p), std::begin(p) + 256, std::default_random_engine(seed));

			for (size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i];
			}
		}

# ifdef __cpp_lib_concepts
		template <std::uniform_random_bit_generator URNG>
# else
		template <class URNG, std::enable_if_t<!std::is_arithmetic_v<URNG>>* = nullptr>
# endif
		void reseed(URNG&& urng)
		{
			for (size_t i = 0; i < 256; ++i)
			{
				p[i] = static_cast<std::uint8_t>(i);
			}

			std::shuffle(std::begin(p), std::begin(p) + 256, std::forward<URNG>(urng));

			for (size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i];
			}
		}

		///////////////////////////////////////
		//
		//	Noise [-1, 1]
		//
		
		value_type noise1D(value_type x) const noexcept
		{
			return noise3D(x, 0, 0);
		}

		
		value_type noise2D(value_type x, value_type y) const noexcept
		{
			return noise3D(x, y, 0);
		}

		
		value_type noise3D(value_type x, value_type y, value_type z) const noexcept
		{
			const std::int32_t X = static_cast<std::int32_t>(std::floor(x)) & 255;
			const std::int32_t Y = static_cast<std::int32_t>(std::floor(y)) & 255;
			const std::int32_t Z = static_cast<std::int32_t>(std::floor(z)) & 255;

			x -= std::floor(x);
			y -= std::floor(y);
			z -= std::floor(z);

			const value_type u = Fade(x);
			const value_type v = Fade(y);
			const value_type w = Fade(z);

			const std::int32_t A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
			const std::int32_t B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

			return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z),
				Grad(p[BA], x - 1, y, z)),
				Lerp(u, Grad(p[AB], x, y - 1, z),
					Grad(p[BB], x - 1, y - 1, z))),
				Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1),
					Grad(p[BA + 1], x - 1, y, z - 1)),
					Lerp(u, Grad(p[AB + 1], x, y - 1, z - 1),
						Grad(p[BB + 1], x - 1, y - 1, z - 1))));
		}

		///////////////////////////////////////
		//
		//	Noise [0, 1]
		//
		
		value_type noise1D_0_1(value_type x) const noexcept
		{
			return noise1D(x)
				* value_type(0.5) + value_type(0.5);
		}

		
		value_type noise2D_0_1(value_type x, value_type y) const noexcept
		{
			return noise2D(x, y)
				* value_type(0.5) + value_type(0.5);
		}

		
		value_type noise3D_0_1(value_type x, value_type y, value_type z) const noexcept
		{
			return noise3D(x, y, z)
				* value_type(0.5) + value_type(0.5);
		}

		///////////////////////////////////////
		//
		//	Accumulated octave noise
		//	* Return value can be outside the range [-1, 1]
		//
		
		value_type accumulatedOctaveNoise1D(value_type x, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise1D(x) * amp;
				x *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		
		value_type accumulatedOctaveNoise2D(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise2D(x, y) * amp;
				x *= 2;
				y *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		
		value_type accumulatedOctaveNoise3D(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise3D(x, y, z) * amp;
				x *= 2;
				y *= 2;
				z *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		///////////////////////////////////////
		//
		//	Normalized octave noise [-1, 1]
		//
		
		value_type normalizedOctaveNoise1D(value_type x, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise1D(x, octaves)
				/ Weight(octaves);
		}

		
		value_type normalizedOctaveNoise2D(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise2D(x, y, octaves)
				/ Weight(octaves);
		}

		
		value_type normalizedOctaveNoise3D(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise3D(x, y, z, octaves)
				/ Weight(octaves);
		}

		///////////////////////////////////////
		//
		//	Accumulated octave noise clamped within the range [0, 1]
		//
		
		value_type accumulatedOctaveNoise1D_0_1(value_type x, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise1D(x, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		
		value_type accumulatedOctaveNoise2D_0_1(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise2D(x, y, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		
		value_type accumulatedOctaveNoise3D_0_1(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise3D(x, y, z, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		///////////////////////////////////////
		//
		//	Normalized octave noise [0, 1]
		//
		
		value_type normalizedOctaveNoise1D_0_1(value_type x, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise1D(x, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		
		value_type normalizedOctaveNoise2D_0_1(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise2D(x, y, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		
		value_type normalizedOctaveNoise3D_0_1(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise3D(x, y, z, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		///////////////////////////////////////
		//
		//	Serialization
		//
		void serialize(std::array<std::uint8_t, 256>& s) const noexcept
		{
			for (std::size_t i = 0; i < 256; ++i)
			{
				s[i] = p[i];
			}
		}

		void deserialize(const std::array<std::uint8_t, 256>& s) noexcept
		{
			for (std::size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i] = s[i];
			}
		}

		///////////////////////////////////////
		//
		//	Legacy interface
		//
		[[deprecated("use noise1D() instead")]]
		double noise(double x) const;
		[[deprecated("use noise2D() instead")]]
		double noise(double x, double y) const;
		[[deprecated("use noise3D() instead")]]
		double noise(double x, double y, double z) const;

		[[deprecated("use noise1D_0_1() instead")]]
		double noise0_1(double x) const;
		[[deprecated("use noise2D_0_1() instead")]]
		double noise0_1(double x, double y) const;
		[[deprecated("use noise3D_0_1() instead")]]
		double noise0_1(double x, double y, double z) const;

		[[deprecated("use accumulatedOctaveNoise1D() instead")]]
		double octaveNoise(double x, std::int32_t octaves) const;
		[[deprecated("use accumulatedOctaveNoise2D() instead")]]
		double octaveNoise(double x, double y, std::int32_t octaves) const;
		[[deprecated("use accumulatedOctaveNoise3D() instead")]]
		double octaveNoise(double x, double y, double z, std::int32_t octaves) const;

		[[deprecated("use accumulatedOctaveNoise1D_0_1() instead")]]
		double octaveNoise0_1(double x, std::int32_t octaves) const;
		[[deprecated("use accumulatedOctaveNoise2D_0_1() instead")]]
		double octaveNoise0_1(double x, double y, std::int32_t octaves) const;
		[[deprecated("use accumulatedOctaveNoise3D_0_1() instead")]]
		double octaveNoise0_1(double x, double y, double z, std::int32_t octaves) const;
	};

	using PerlinNoise = BasicPerlinNoise<double>;
}


namespace hise
{
	template <int EdgeLength> struct PerlinCube
	{
		PerlinCube()
		{
			memset(data, 0, sizeof(float) * getNumValues());
		}

		/** Sets the octave. */
		void setOctave(int newNumOctaves)
		{
			if (octaves != newNumOctaves)
			{
				octaves = newNumOctaves;
				generate();
			}
		}

		/** Returns a interpolated noise value for the 3D coordinate. */
		float getInterpolated(float normalisedX, float normalisedY, float normalisedZ)
		{
			auto zIndex = jlimit<int>(0, EdgeLength - 1, normalisedZ * (float)EdgeLength);
			auto xf = (float)EdgeLength * normalisedX;
			auto yf = (float)EdgeLength * normalisedY;

			auto xlow = (int)xf;
			auto xt = xf - (float)xlow;

			auto ylow = (int)yf;
			auto yt = yf - (float)ylow;

			auto xhigh = jlimit(0, EdgeLength - 1, xlow + 1);
			auto yhigh = jlimit(0, EdgeLength - 1, ylow + 1);

			float id[4] = { data[xlow][ylow][zIndex],
							data[xlow][yhigh][zIndex],
							data[xhigh][ylow][zIndex],
							data[xhigh][yhigh][zIndex] };

			auto v1 = Lerp(xt, id[0], id[2]);
			auto v2 = Lerp(xt, id[1], id[3]);

			return Lerp(yt, v1, v2);
		}

		/** Fills an image with the 2D perlin noise at the normalised z slice. */
		void fillImage(Image& img, float z)
		{
			auto w = img.getWidth();
			auto h = img.getHeight();

			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					auto v = getInterpolated((float)x / (float)w, (float)y / (float)h, z);
					uint8 b = (uint8)roundToInt(v * 255.0f);
					img.setPixelAt(x, y, Colour::fromRGB(b, b, b));
				}
			}
		}

	private:

		void generate()
		{
			int index = 0;
			auto data_ = reinterpret_cast<float*>(data);

			for (float x = 0.0f; x < (float)1.0f; x += 1.0 / (float)EdgeLength)
			{
				for (float y = 0.0f; y < (float)1.0f; y += 1.0 / (float)EdgeLength)
				{
					for (float z = 0.0f; z < (float)1.0f; z += 1.0 / (float)EdgeLength)
					{
						data_[index++] = pn.accumulatedOctaveNoise3D_0_1(x, y, z, octaves);
					}
				}
			}
		}

		float Lerp(float t, float a, float b) noexcept
		{
			return a + t * (b - a);
		}

		

		static constexpr int getNumValues() { return EdgeLength * EdgeLength * EdgeLength; }

		float data[EdgeLength][EdgeLength][EdgeLength];

		siv::BasicPerlinNoise<float> pn;
		int octaves = 16;
	};
}