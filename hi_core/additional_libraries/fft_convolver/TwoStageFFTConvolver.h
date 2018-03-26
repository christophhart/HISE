// ==================================================================================
// Copyright (c) 2017 HiFi-LoFi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ==================================================================================

#ifndef _FFTCONVOLVER_TWOSTAGEFFTCONVOLVER_H
#define _FFTCONVOLVER_TWOSTAGEFFTCONVOLVER_H

#include "FFTConvolver.h"
#include "Utilities.h"


namespace fftconvolver
{ 

/**
* @class TwoStageFFTConvolver
* @brief FFT convolver using two different block sizes
*
* The 2-stage convolver consists internally of two convolvers:
*
* - A head convolver, which processes the only the begin of the impulse response.
*
* - A tail convolver, which processes the rest and major amount of the impulse response.
*
* Using a short block size for the head convolver and a long block size for
* the tail convolver results in much less CPU usage, while keeping the
* calculation time of each processing call short.
*
* Furthermore, this convolver class provides virtual methods which provide the
* possibility to move the tail convolution into the background (e.g. by using
* multithreading, see startBackgroundProcessing()/waitForBackgroundProcessing()).
*
* As well as the basic FFTConvolver class, the 2-stage convolver is suitable
* for real-time processing which means that no "unpredictable" operations like
* allocations, locking, API calls, etc. are performed during processing (all
* necessary allocations and preparations take place during initialization).
*/
class TwoStageFFTConvolver
{  
public:
  TwoStageFFTConvolver();  
  virtual ~TwoStageFFTConvolver();
  
  /**
  * @brief Initialization the convolver
  * @param headBlockSize The head block size
  * @param tailBlockSize the tail block size
  * @param ir The impulse response
  * @param irLen Length of the impulse response in samples
  * @return true: Success - false: Failed
  */
  bool init(size_t headBlockSize, size_t tailBlockSize, const Sample* ir, size_t irLen);

  /**
  * @brief Convolves the the given input samples and immediately outputs the result
  * @param input The input samples
  * @param output The convolution result
  * @param len Number of input/output samples
  */
  void process(const Sample* input, Sample* output, size_t len);

  /**
  * @brief Resets the convolver and discards the set impulse response
  */
  void reset();
  
protected:
  /**
  * @brief Method called by the convolver if work for background processing is available
  *
  * The default implementation just calls doBackgroundProcessing() to perform the "bulk"
  * convolution. However, if you want to perform the majority of work in some background
  * thread (which is recommended), you can overload this method and trigger the execution
  * of doBackgroundProcessing() really in some background thread.
  */
  virtual void startBackgroundProcessing();

  /**
  * @brief Called by the convolver if it expects the result of its previous call to startBackgroundProcessing()
  *
  * After returning from this method, all background processing has to be completed.
  */
  virtual void waitForBackgroundProcessing();

  /**
  * @brief Actually performs the background processing work
  */
  void doBackgroundProcessing();

private:
  size_t _headBlockSize;
  size_t _tailBlockSize;
  FFTConvolver _headConvolver;
  FFTConvolver _tailConvolver0;
  SampleBuffer _tailOutput0;
  SampleBuffer _tailPrecalculated0;
  FFTConvolver _tailConvolver;
  SampleBuffer _tailOutput;
  SampleBuffer _tailPrecalculated;
  SampleBuffer _tailInput;
  size_t _tailInputFill;
  size_t _precalculatedPos;
  SampleBuffer _backgroundProcessingInput;

  // Prevent uncontrolled usage
  TwoStageFFTConvolver(const TwoStageFFTConvolver&);
  TwoStageFFTConvolver& operator=(const TwoStageFFTConvolver&);
};
  
} // End of namespace fftconvolver

#endif // Header guard
