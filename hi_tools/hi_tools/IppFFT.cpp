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

namespace hise { using namespace juce;


IppFFT::IppFFT(DataType typeToUse, int maxPowerOfTwo /*= IPP_FFT_MAX_POWER_OF_TWO*/, const int flagToUse /*= IPP_FFT_NODIV_BY_ANY*/) :
type(typeToUse),
maxOrder(jmin<int>(maxPowerOfTwo, IPP_FFT_MAX_POWER_OF_TWO)),
flag(flagToUse)
{
	
	

	for (int i = 0; i < maxOrder; i++)
	{
		specBuffers.add(new Buffer());
		workingBuffers.add(new Buffer());
	}

	for (int i = 0; i < IPP_FFT_MAX_POWER_OF_TWO; i++)
	{
		realDoubleSpecs[i] = nullptr;
		realFloatSpecs[i] = nullptr;
		complexDoubleSpecs[i] = nullptr;
		complexFloatSpecs[i] = nullptr;
	}

	for (int i = 1; i < maxOrder; i++)
	{
		initFFT(i);
	}

	//additionalWorkingBuffer = new Buffer(pow(2, maxPowerOfTwo) * sizeof(float));
}

IppFFT::~IppFFT()
{
	for (int i = 0; i < IPP_FFT_MAX_POWER_OF_TWO; i++)
	{
		if (realDoubleSpecs[i] != nullptr)
		{
			realDoubleSpecs[i] = nullptr;
		}

		if (realFloatSpecs[i] != nullptr)
		{
			realFloatSpecs[i] = nullptr;
		}

		if (complexDoubleSpecs[i] != nullptr)
		{
			complexDoubleSpecs[i] = nullptr;
		}

		if (complexFloatSpecs[i] != nullptr)
		{
			complexFloatSpecs[i] = nullptr;
		}
	}

	specBuffers.clear();
	workingBuffers.clear();

	additionalWorkingBuffer = nullptr;
}

void IppFFT::realFFTInplace(float *data, int size) const
{
	jassert(type == DataType::RealFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_RToPerm_32f_I((Ipp32f*)data, realFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::realFFTInplace(double *data, int size) const
{
	jassert(type == DataType::RealDouble);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_RToPerm_64f_I((Ipp64f*)data, realDoubleSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::realFFTInverseInplace(float *data, int size) const
{
	jassert(type == DataType::RealFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_PermToR_32f_I((Ipp32f*)data, realFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::realFFTInverseInplace(double *data, int size) const
{
	jassert(type == DataType::RealDouble);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_PermToR_64f_I((Ipp64f*)data, realDoubleSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFTInplace(float *data, int size) const
{
	jassert(type == DataType::ComplexFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_CToC_32fc_I((Ipp32fc*)data, complexFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFTInplace(double *data, int size) const
{
	jassert(type == DataType::ComplexDouble);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_CToC_64fc_I((Ipp64fc*)data, complexDoubleSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFTInverseInplace(float *data, int size) const
{
	jassert(type == DataType::ComplexFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_CToC_32fc_I((Ipp32fc*)data, complexFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFTInverseInplace(double *data, int size) const
{
	jassert(type == DataType::ComplexDouble);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_CToC_64fc_I((Ipp64fc*)data, complexDoubleSpecs[N], workingBuffers[N]->getData());

	}
}

void IppFFT::realFFT(const float *in, float* out, int size) const
{
	jassert(type == DataType::RealFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_RToPerm_32f((const Ipp32f*)in, (Ipp32f*)out, realFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::realFFTInverse(const float *in, float* out, int size) const
{
	jassert(type == DataType::RealFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_PermToR_32f((const Ipp32f*)in, (Ipp32f*)out, realFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFT(const float *in, float* out, int size) const
{
	jassert(type == DataType::ComplexFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTFwd_CToC_32fc((const Ipp32fc*)in, (Ipp32fc*)out, complexFloatSpecs[N], workingBuffers[N]->getData());
	}
}

void IppFFT::complexFFTInverse(const float* in, float *out, int size) const
{
	jassert(type == DataType::ComplexFloat);

	const int N = getPowerOfTwo(size);

	if (N > 0)
	{
		ippsFFTInv_CToC_32fc((const Ipp32fc*)in, (Ipp32fc*)out, complexFloatSpecs[N], workingBuffers[N]->getData());
	}
}

int IppFFT::getPowerOfTwo(int size) const
{
	if (!isPowerOfTwo(size)) return -1;

	const int N = (int)(log(size) / log(2));

	if (isPositiveAndBelow(N, maxOrder))
	{
		return N;
	}
	else
	{
		jassertfalse;
	}

	return -1;
}


void IppFFT::initFFT(int N)
{
	int sizeSpec = 0;
	int sizeInit = 0;
	int sizeBuffer = 0;

	getSizes(N, sizeSpec, sizeInit, sizeBuffer);

	specBuffers[N]->setSize(sizeSpec);
	workingBuffers[N]->setSize(sizeBuffer);

	ScopedPointer<Buffer> initBuffer = new Buffer(sizeInit);

	initSpec(N, specBuffers[N]->getData(), initBuffer->getData());

	initBuffer = nullptr;

}

void IppFFT::getSizes(int FFTOrder, int &sizeSpec, int &sizeInit, int &sizeBuffer)
{
	IppStatus status;

	switch (type)
	{
	default:
	case DataType::ComplexFloat: status = ippsFFTGetSize_C_32fc(FFTOrder, flag, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer); break;
	case DataType::ComplexDouble: status = ippsFFTGetSize_C_64fc(FFTOrder, flag, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer); break;
	case DataType::RealFloat: status = ippsFFTGetSize_R_32f(FFTOrder, flag, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer); break;
	case DataType::RealDouble: status = ippsFFTGetSize_R_64f(FFTOrder, flag, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer); break;
	}

	jassert(status == ippStsNoErr);
}


void IppFFT::initSpec(int N, Ipp8u *specData, Ipp8u *initData)
{
	IppStatus status = ippStsNoErr;

	switch (type)
	{
	case DataType::ComplexFloat: status = ippsFFTInit_C_32fc(&(complexFloatSpecs[N]), N, flag, ippAlgHintNone, specData, initData); break;
	case DataType::ComplexDouble: status = ippsFFTInit_C_64fc(&complexDoubleSpecs[N], N, flag, ippAlgHintNone, specData, initData); break;
	case DataType::RealFloat: status = ippsFFTInit_R_32f(&realFloatSpecs[N], N, flag, ippAlgHintNone, specData, initData); break;
	case DataType::RealDouble: status = ippsFFTInit_R_64f(&realDoubleSpecs[N], N, flag, ippAlgHintNone, specData, initData); break;
	}

	if (status != ippStsNoErr)
	{
        const String error ="ERROR at initialisizing " + String(N) + ": " + String(status);
        
		DBG(error);
	}
}

IppFFT::Buffer::Buffer(uint32 len /*= 0*/) :
size(len)
{
	setSize(len);
}

IppFFT::Buffer::~Buffer()
{
	if (data != nullptr)
	{
		ippFree(data);
		data = nullptr;
	}
}

void IppFFT::Buffer::setSize(uint32 len)
{
	if (data != nullptr)
	{
		ippFree(data);
	}

	if (len != 0)
	{
		data = ippsMalloc_8u(len);
		memset(data, 0, sizeof(Ipp8u)*len);
	}
	else
	{
		data = nullptr;
	}
}

void IppFFT::Buffer::releaseData()
{
	if (data != nullptr)
	{
		ippFree(data);
		data = nullptr;
	}
}

} // namespace hise