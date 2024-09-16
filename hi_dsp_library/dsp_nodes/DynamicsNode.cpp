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

namespace scriptnode {
namespace dynamics {

using namespace hise;
using namespace juce;



void updown_comp::RMSDetector::prepare(PrepareSpecs ps)
{
	int bufferSize = roundToInt(ps.sampleRate * 0.03);
			
	if (data.size() != bufferSize)
	{
		memory.setSize(bufferSize);
		data.referTo(memory);
		coeff = 1.0f / bufferSize;

		reset();
	}
}

void updown_comp::RMSDetector::processFrame(span<float, 1>& peak)
{
	if (!enabled)
		return;

	auto oldValue = data[arrayIndex];
	auto newValue = (double)(peak[0]);
	newValue *= newValue;

	data[arrayIndex] = newValue;

	if (++arrayIndex >= data.size())
		arrayIndex = 0;
			
	sum -= oldValue;
	sum += newValue;
	sum = jmax(0.0, sum);
	auto r = sum * coeff;
	r = hmath::sqrt(r);
	peak[0] = r;
}

void updown_comp::RMSDetector::setEnabled(double v)
{
	enabled = v > 0.5;
}

void updown_comp::RMSDetector::reset()
{
	FloatVectorOperations::clear(data.begin(), data.size());
	arrayIndex = 0;
	sum = 0.0;
}

void updown_comp::updown_envelope_follower::prepare(PrepareSpecs ps)
{
	sampleRate = ps.sampleRate;
	updateCoefficients();
}

void updown_comp::updown_envelope_follower::reset()
{
	lastValue = 0.0f;
}

void updown_comp::updown_envelope_follower::processFrame(span<float, 1>& input)
{
	double inputDouble = (double)input[0];

	auto isHigher = (inputDouble > lastValue);
	auto coeffToUse = isHigher ? atk : rel;

	// skip the smoothing when the value is below the low threshold
	coeffToUse *= (double)(int)!(inputDouble < lo_t && isHigher);

	lastValue = coeffToUse * (lastValue - inputDouble) + inputDouble;

	input[0] = (float)lastValue;
}

void updown_comp::updown_envelope_follower::setLowThreshold(double v)
{
	lo_t = v;
}

void updown_comp::updown_envelope_follower::setAttack(double attack_)
{
	attack = attack_;
	updateCoefficients();
}

void updown_comp::updown_envelope_follower::setRelease(double release_)
{
	release = release_;
	updateCoefficients();
}

void updown_comp::updown_envelope_follower::updateCoefficients()
{
	if (sampleRate > 0.0)
	{
		atk = exp(log(0.01) / (attack * sampleRate * 0.001));
		rel = exp(log(0.01) / (release * sampleRate * 0.001));
	}
}

void updown_comp::prepare(PrepareSpecs specs)
{
	display_buffer_base<true>::prepare(specs);

	for (auto& s : state)
		s.prepare(specs.sampleRate, 50.0);

	rmsDetector.prepare(specs);
	envelopeFollower.prepare(specs);
}

void updown_comp::reset()
{
	rmsDetector.reset();
	envelopeFollower.reset();

	for (auto& s : state)
		s.reset();
}

float updown_comp::getGainReduction(float input)
{
	

	const auto lo_t = state[(int)Parameters::LowThreshold].advance();
	const auto hi_t = state[(int)Parameters::HighThreshold].advance();

	auto k = state[(int)Parameters::Knee].advance();

	const auto lo_w = jmin((hi_t - lo_t) * 0.5f, k);
	const auto hi_w = jmin((hi_t - lo_t) * 0.5f, k);

	const auto hi_r = state[(int)Parameters::HighRatio].advance();
	const auto lo_r = state[(int)Parameters::LowRatio].advance();

	if (hmath::abs(input - lo_t) < (lo_w * 0.5f))
	{
		return input - (1.0f / lo_r - 1.0f) * hmath::sqr(input - lo_t - lo_w * 0.5f) / (2.0f * lo_w);
	}
	else if (input < (lo_t - lo_w*0.5f))
	{
		auto v = jmax(0.0f, lo_t + (input - lo_t) / lo_r);

		const auto noise_floor = Decibels::decibelsToGain(-82.0f);

		if (input < noise_floor)
		{
			auto alpha = input / noise_floor;
			auto invAlpha = 1.0f - alpha;

			return input * invAlpha + v * alpha;
		}
		else
			return v;
	}
	else if (hmath::abs(input - hi_t) < (hi_w / 2)) // apply soft knee
	{
		return input + (1.0f / hi_r - 1.0f) * hmath::sqr(input - hi_t + hi_w * 0.5f) / (2.0f * hi_w);
	}
	else if (input > (hi_t + hi_w*0.5f))
	{
		return jmin(1.0f, hi_t + (input - hi_t) / hi_r);
	}
	else
	{
		return input;
	}
}

void updown_comp::createParameters(ParameterDataList& data)
{
	{
		parameter::data p("LowThreshold", { -100.0, 0.0 });
		registerCallback<(int)Parameters::LowThreshold>(p);
		p.setSkewForCentre(-18.0);
		p.setDefaultValue(-100.0);
		data.add(std::move(p));
	}
	{
		parameter::data p("LowRatio", { 0.2, 100.0 });
		registerCallback<(int)Parameters::LowRatio>(p);
		p.setSkewForCentre(1.0);
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
	{
		// Create a parameter like this
		parameter::data p("HighThreshold", { -100.0, 0.0 });
		registerCallback<(int)Parameters::HighThreshold>(p);
		p.setSkewForCentre(-6.0);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
	{
		parameter::data p("HighRatio", { 0.2, 100.0 });
		registerCallback<(int)Parameters::HighRatio>(p);
		p.setSkewForCentre(1.0);
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
	{
		parameter::data p("Knee", { 0.0, 0.3 });
		registerCallback<(int)Parameters::Knee>(p);
		p.setSkewForCentre(0.15);
		p.setDefaultValue(0.15);
		data.add(std::move(p));
	}
	{
		// Create a parameter like this
		parameter::data p("Attack", { 0.0, 1000.0 });
		registerCallback<(int)Parameters::Attack>(p);
		p.setSkewForCentre(50.0);
		p.setDefaultValue(50.0);
		data.add(std::move(p));
	}
	{
		// Create a parameter like this
		parameter::data p("Release", { 0.0, 1000.0 });
		registerCallback<(int)Parameters::Release>(p);
		p.setSkewForCentre(50.0);
		p.setDefaultValue(50.0);
		data.add(std::move(p));
	}
	{
		parameter::data p("RMS", { 0.0, 1.0 });
		registerCallback<(int)Parameters::RMS>(p);
		p.setParameterValueNames({ "Off", "On" });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
}

void updown_comp::calculateGraph(block values)
{
	for(auto& v: values)
	{
		v = getGainReduction(v);
	}
}
} // dynamics
} // scriptnode
