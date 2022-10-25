/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
#ifndef IPPFFT_H_INCLUDED
#define IPPFFT_H_INCLUDED

namespace hise { using namespace juce;

#define IPP_FFT_MAX_POWER_OF_TWO 16


/** A wrapper around the Intel IPP FFT routines.
*
*	It uses RAII to manage all required buffers & datas. In order to use it, create an instance once and then call the routines:
*
*	IppFFT fft(IppFFT:DataType::ComplexFloat);
*
*	fft.realFFT(data, 512);
*
*	It is not templated for speed, but it throws assertions if you call it on the wrong type.
*/
class IppFFT
{
public:

    using Helpers = FFTHelpers;

	enum class DataType
	{
		ComplexFloat = 0,
		ComplexDouble,
		RealFloat,
		RealDouble
	};

	// =============================================================================================================================

	/** Creates a IPP FFT object. The initialisation time is rather slow, but then it will unleash its power because it allocates all necessary buffers once. */
    IppFFT(DataType typeToUse=DataType::ComplexFloat, int maxPowerOfTwo = IPP_FFT_MAX_POWER_OF_TWO, const int flagToUse=8);
	~IppFFT();

	// ==================================================================================================================================== float FFTs

	/** Real inplace FFT (input is aligned float array, size is power of two.)
	*
	*	Input: d[] = re[0],re[1],..,re[size-1].
	*	Output: d[] = re[0],*re[size/2]*,re[1],im[1],..,re[size/2-1],im[size/2-1].
	*/
	void realFFTInplace(float *data, int size) const;

	/** Real inplace FFT (input is aligned float array, size is power of two.) */
	void realFFTInverseInplace(float *data, int size) const;

	/** Complex inplace FFT (input is aligned Complex<float> array, size is power of two.) */
	void complexFFTInplace(float *data, int size) const;

	/** Complex inverse inplace FFT (input is aligned float array, size is power of two.) */
	void complexFFTInverseInplace(float *data, int size) const;

	/** Real inplace FFT (input is aligned float array, size is power of two.)
	*
	*	Input: d[] = re[0],re[1],..,re[size-1].
	*	Output: d[] = re[0],*re[size/2]*,re[1],im[1],..,re[size/2-1],im[size/2-1].
	*
	*	Use Helpers::toFreqSpectrum() afterwards
	*/
	void realFFT(const float *in, float* out, int size) const;

	/** Real inplace FFT (input is aligned float array, size is power of two.) */
	void realFFTInverse(const float *in, float* out, int size) const;

	/** Complex inplace FFT (input is aligned Complex<float> array, size is power of two.) */
	void complexFFT(const float *in, float* out, int size) const;

	/** Complex inverse inplace FFT (input is aligned float array, size is power of two.) */
	void complexFFTInverse(const float* in, float *out, int size) const;

	


	// ==================================================================================================================================== double FFTs

	/** Real inplace FFT (input is aligned double array, size is power of two.)
	*
	*	Input: data[] = re[0],re[1],..,re[size-1].
	*	Output: data[] = re[0],*re[size/2]*,re[1],im[1],..,re[size/2-1],im[size/2-1].
	*
	*/
	void realFFTInplace(double *data, int size) const;

	/** Real inplace inverse FFT (input is aligned double array, size is power of two.) */
	void realFFTInverseInplace(double *data, int size) const;

	/** Complex inplace FFT (input is aligned Complex<double> array, size is power of two.) */
	void complexFFTInplace(double *data, int size) const;

	/** Complex inverse inplace FFT (input is aligned Complex<double> array, size is power of two.) */
	void complexFFTInverseInplace(double *data, int size) const;

	float *getAdditionalWorkBuffer()
	{
		return (float*)additionalWorkingBuffer->getData();
	}

private:

	// =============================================================================================================================

	class Buffer
	{
	public:

		Buffer(uint32 len = 0);
		~Buffer();

		void setSize(uint32 len);

		void releaseData();

		Ipp8u *getData() { return data; }
		const Ipp8u *getData() const { return data; }

	private:

		Ipp8u *data = nullptr;
		uint32 size = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Buffer)
	};

	// =============================================================================================================================

	/** @internal */
	int getPowerOfTwo(int size) const;
	/** @internal */
	void initSpec(int N, Ipp8u *specData, Ipp8u *initData);
	/** @internal */
	void initFFT(int N);
	/** @internal */
	void getSizes(int FFTOrder, int &sizeSpec, int &sizeInit, int &sizeBuffer);

	const DataType type;
	const int maxOrder;
	const int flag;

	IppsFFTSpec_C_32fc *complexFloatSpecs[IPP_FFT_MAX_POWER_OF_TWO];
	IppsFFTSpec_C_64fc *complexDoubleSpecs[IPP_FFT_MAX_POWER_OF_TWO];
	IppsFFTSpec_R_32f *realFloatSpecs[IPP_FFT_MAX_POWER_OF_TWO];
	IppsFFTSpec_R_64f *realDoubleSpecs[IPP_FFT_MAX_POWER_OF_TWO];

	OwnedArray<Buffer> workingBuffers;
	OwnedArray<Buffer> specBuffers;

	ScopedPointer<Buffer> additionalWorkingBuffer;

	// =============================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IppFFT)
};

} // namespace hise

#endif  // IPPFFT_H_INCLUDED
