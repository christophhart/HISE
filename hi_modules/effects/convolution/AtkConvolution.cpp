

#if 0

ConvolutionFilter::ConvolutionFilter() : split_position(0), splitSize(0), processor(IppFFT::DataType::ComplexFloat)
{
}


void ConvolutionFilter::setImpulse(float *impulseData, int impulseSize)
{
	//this->impulse = std::move(impulseData);

	impulseBuffer.setSize(1, impulseSize);

	FloatVectorOperations::copy(impulseBuffer.getWritePointer(0), impulseData, impulseSize);

	setup();
}

void ConvolutionFilter::setSplitSize(int split_size)
{
	this->splitSize = split_size;
	setup();
}


void ConvolutionFilter::setup()
{
	if (impulseBuffer.getNumSamples() == 0 || splitSize == 0)
	{
		return;
	}

	auto numSplitBuffers = (impulseBuffer.getNumSamples() + splitSize - 1) / splitSize;

	partialFrequencyInputBufferList.assign(numSplitBuffers - 1, AudioSampleBuffer(1, splitSize * 2));

	//partial_frequency_input.assign(nb_splits - 1, std::vector<std::complex<float> >(split_size * 2, 0));
	// Pad with zeros so the convolution is easier created.
	//impulse.resize((partial_frequency_input.size() + 1) * split_size, 0);

	impulseBuffer.setSize(1, numSplitBuffers * splitSize, true, true);

	tempBuffer.setSize(1, splitSize * 2);
	tempBuffer.clear();

	//temp_out_buffer.assign(splitSize * 2, 0);

	//processor.set_size(split_size * 2);

	// The size is twice as big than the impulse, less
	//partial_frequency_impulse.assign(splitSize * 2 * (numSplitBuffers - 1), 0);

	partialFrequencyImpulseBuffer.setSize(numSplitBuffers - 1, splitSize * 2);
	partialFrequencyImpulseBuffer.clear();

	for (int i = 0; i < numSplitBuffers - 1; ++i)
	{
		FloatVectorOperations::copy(partialFrequencyImpulseBuffer.getWritePointer(i), impulseBuffer.getReadPointer(0) + i * splitSize, splitSize);
		processor.complexFFT(partialFrequencyImpulseBuffer.getWritePointer(i), splitSize);
	}

	int bruteLength;

	ippsConvolveGetBufferSize(splitSize - 1, impulseBuffer.getNumSamples() , IppDataType::ipp32f, IppAlgType::ippAlgAuto, &bruteLength);

	bruteBuffer2(bruteLength);

	bruteBuffer.setSize(1, splitSize);

	bruteBuffer.clear();

	resultBuffer.setSize(1, splitSize * 2);
	resultBuffer.clear();

	invFFTBuffer.setSize(1, splitSize * 2);
	invFFTBuffer.clear();

	input_delay = splitSize - 1;
}

void ConvolutionFilter::calculatePartialConvolutions()
{
	// Move the temp buffer content from the first split window to the beginning and zero pad the rest...

	FloatVectorOperations::copy(tempBuffer.getWritePointer(0), tempBuffer.getReadPointer(0, splitSize), splitSize);
	FloatVectorOperations::clear(tempBuffer.getWritePointer(0, splitSize), splitSize);

#if 0
	for (int i = 0; i < splitSize; ++i)
	{
		temp_out_buffer[i] = temp_out_buffer[i + splitSize];
		temp_out_buffer[i + splitSize] = 0;
	}
#endif

	

	//result.assign(2 * splitSize, 0);
	//float* result_ptr_orig = reinterpret_cast<float*>(result.data());

	

	//const float* partial_frequency_impulse_ptr_orig = reinterpret_cast<const float*>(partial_frequency_impulse.data());

	// offset in the impulse frequencies
	//int offset = 0;

	int j = 0;

	for (const auto &buffer: partialFrequencyInputBufferList)
	{
		float* resultData = resultBuffer.getWritePointer(0);
		const float* a = buffer.getReadPointer(0);
		const float* b = partialFrequencyInputBuffer.getReadPointer(j);

		j++;

		// Complex multiply conjugate, replace with IPP version...

		// Add the frequency result of this partial FFT
		for (int64_t i = 0; i < splitSize; ++i)
		{
			float br1 = *(a++);
			float bi1 = *(a++);
			float pr1 = *(b++);
			float pi1 = *(b++);
			float br2 = *(a++);
			float bi2 = *(a++);
			float pr2 = *(b++);
			float pi2 = *(b++);
			*(resultData++) += br1*pr1 - bi1*pi1;
			*(resultData++) += br1*pi1 + pr1*bi1;
			*(resultData++) += br2*pr2 - bi2*pi2;
			*(resultData++) += br2*pi2 + pr2*bi2;
		}
		//offset += 4 * splitSize;
	}

	//std::vector<float> ifft_result(2 * splitSize, 0);

	processor.complexInverseFFT(resultBuffer.getWritePointer(0), 2*splitSize);

	//processor.process_backward(result.data(), ifft_result.data(), 2 * split_size);
	
	FloatVectorOperations::addWithMultiply(tempBuffer.getWritePointer(0), resultBuffer.getReadPointer(0), (float)(splitSize * 2), splitSize * 2);
	
#if 0
	for (int i = 0; i < 2 * split_size; ++i)
	{
		temp_out_buffer[i] += ifft_result[i] * split_size * 2;
	}
#endif
}


void ConvolutionFilter::pushNewChunk(float *inputData, int numSamples)
{
	/*
	if (partial_frequency_input.empty())
		return;
	partial_frequency_input.pop_back();
	std::vector<std::complex<double> > chunk(2 * split_size);
	processor.process_forward(converted_inputs[0] + position - split_size, chunk.data(), split_size);

	partial_frequency_input.push_front(std::move(chunk));
	compute_convolutions();
	*/

	if (partialFrequencyInputBufferList.empty())
	{
		return;
	}

	//if (partial_frequency_input.empty())
	//	return;
	

	
	partialFrequencyInputBufferList.pop_back();
	
	//partial_frequency_input.pop_back();
	std::vector<std::complex<double> > chunk(2 * splitSize);

	partialFrequencyInputBufferList.push_front(AudioSampleBuffer(1, 2 * splitSize));
	
	FloatVectorOperations::copy(partialFrequencyInputBufferList.begin()->getWritePointer(0), inputData, numSamples);

	

	processor.complexFFT(partialFrequencyInputBufferList.begin()->getWritePointer(0), 2*splitSize);

	//processor.process_forward(converted_inputs[0] + inputData - split_size, chunk.data(), split_size);

	//partial_frequency_input.push_front(std::move(chunk));
	calculatePartialConvolutions();
}



void ConvolutionFilter::processBruteConvolution(float *input, int numSamples) 
{
	const float* impulse_ptr = impulseBuffer.getReadPointer(0);
	float*  output = bruteBuffer.getWritePointer(0);

	

	ippsConvolve_32f(impulse_ptr, input_delay + 1, input, numSamples, output, IppAlgType::ippAlgAuto, bruteBuffer2.getNativeDataPointer());
	
	FloatVectorOperations::add(output, tempBuffer.getWritePointer(0, split_position), numSamples);

#if 0
	// Direct Convolution, replace with IPP when save
	for (int j = 0; j < input_delay + 1; ++j)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			output[i] += impulse_ptr[j] * input[i - j];
		}
	}
#endif

	FloatVectorOperations::copy(input, output, numSamples);
}

void ConvolutionFilter::processBlock(float *data, int numSamples)
{
	int processed_size = 0;
	do
	{
		// We can only process split_size elements at a time, but if we already have some elements in the buffer,
		// we need to take them into account.
		int size_to_process = jmin<int>(splitSize-split_position, numSamples - processed_size);

		processBruteConvolution(data, size_to_process);

		split_position += size_to_process;
		processed_size += size_to_process;

		data += size_to_process;

		if (split_position == splitSize)
		{
			pushNewChunk(data, size_to_process);
			split_position = 0;
		}
	} while (processed_size != numSamples);
}


#endif