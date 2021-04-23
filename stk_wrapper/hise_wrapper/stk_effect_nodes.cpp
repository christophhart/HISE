
/*
  ==============================================================================

  The Synthesis ToolKit in C++ (STK) is a set of open source audio
  signal processing and algorithmic synthesis classes written in the
  C++ programming language. STK was designed to facilitate rapid
  development of music synthesis and audio processing software, with
  an emphasis on cross-platform functionality, realtime control,
  ease of use, and educational example code.  STK currently runs
  with realtime support (audio and MIDI) on Linux, Macintosh OS X,
  and Windows computer platforms. Generic, non-realtime support has
  been tested under NeXTStep, Sun, and other platforms and should
  work with any standard C++ compiler.

  STK WWW site: http://ccrma.stanford.edu/software/stk/include/

  The Synthesis ToolKit in C++ (STK)
  Copyright (c) 1995-2011 Perry R. Cook and Gary P. Scavone

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  Any person wishing to distribute modifications to the Software is
  asked to send the modifications to the original developer so that
  they can be incorporated into the canonical version.  This is,
  however, not a binding provision of this license.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  ==============================================================================
*/

/** This is a modified version of the STK that is compatible with HISE.
	Any changes made to the original source code are available under public domain. */

namespace stk
{



	void nodes::banded_wg::prepare(PrepareSpecs ps)
	{
		voices.prepare(ps);

		for (auto& v : voices)
			v.setSampleRate(ps.sampleRate);

		reset();
	}

	void nodes::banded_wg::reset()
	{
		for (auto& v : voices)
			v.clear();
	}

	void nodes::banded_wg::handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			voices.get().setFrequency(e.getFrequency());
			

		if (e.isNoteOff())
			voices.get().noteOff(e.getFloatVelocity());
	}

	void nodes::banded_wg::setDynamics(double newValue)
	{
		for (auto& v : voices)
			v.controlChange(100, newValue * 128.0);
	}

	void nodes::banded_wg::setPressure(double newPressure)
	{
		for (auto& v : voices)
			v.controlChange(2, newPressure * 128.0);
	}

	void nodes::banded_wg::setFreqRatio(double ratio)
	{
		
	}

	void nodes::banded_wg::setPosition(double newValue)
	{
		for (auto& v : voices)
			v.controlChange(4, newValue * 128.0);
	}

	void nodes::banded_wg::setActive(double a)
	{
		if (a > 0.5)
		{
			for (auto& v : voices)
				v.startBowing(a-0.5 * 2.0, JUCE_LIVE_CONSTANT(0.0f));
		}
		else
		{
			for (auto& v : voices)
				v.stopBowing(a * 2.0);
		}

		
	}

	void nodes::banded_wg::createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(banded_wg, Dynamics);
			p.setRange({ 0.0, 1.0, 0.01 });
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(banded_wg, Pressure);
			p.setRange({ 0.0, 1.0, 0.01 });
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(banded_wg, Position);
			p.setRange({ 0.0, 1.0, 0.01 });
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(banded_wg, Active);
			p.setRange({ 0.0, 1.0, 1.0 });
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(banded_wg, FreqRatio);
			p.setRange({ 1.0, 16.0, 1.0 });
			data.add(std::move(p));
		}
	}

}
