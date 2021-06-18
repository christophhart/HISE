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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode 
{
using namespace juce;
using namespace hise;


namespace faders
{
	struct switcher
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto numParameters = (double)(numElements);
			auto indexToActivate = jmin(numElements - 1, (int)(normalisedInput * numParameters));

			return (double)(indexToActivate == Index);
		}
	};

	struct overlap
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			if (isPositiveAndBelow(Index, numElements))
			{
				switch (numElements)
				{
				case 2:
				{
					switch (Index)
					{
					case 0: return jlimit(0.0, 1.0, 2.0 - 2.0 * normalisedInput);
					case 1: return jlimit(0.0, 1.0, 2.0 * normalisedInput);
					}
				}
				case 3:
				{
					switch (Index)
					{
					case 0: return jlimit(0.0, 1.0, 3.0 - 3.0 * normalisedInput);
					case 1: return jlimit(0.0, 1.0, 3.0 * normalisedInput);
					}

				}
				case 4:
				{
					if (Index != 1)
						return 0.0;

					auto v = 2.0 - hmath::abs(-4.0 * (normalisedInput + 0.66));

					v = jmax(0.0, v - 1.0);

					return jlimit(0.0, 1.0, v);
				}
				}
			}

			return 0.0;
		}
	};

	struct harmonics
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			return normalisedInput * (double)(Index + 1);
		}
	};

	struct linear
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			if (numElements == 1)
				return 1.0 - normalisedInput;
			else
			{
				const double u = (double)numElements - 1.0;
				const double offset = (1.0 - (double)Index) / u;
				auto v = 1.0 - Math.abs(1.0 - u * (normalisedInput + offset));
				return jlimit(0.0, 1.0, v);
			}

			return 0.0;
		}

		hmath Math;
	};

	struct squared
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto v = lf.getFadeValue<Index>(numElements, normalisedInput);
			return v * v;
		}

		linear lf;
	};

	struct rms
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto v = lf.getFadeValue<Index>(numElements, normalisedInput);
			return hmath::sqrt(v);
		}

		linear lf;
	};


}


namespace smoothers
{
	struct base
	{
		void setSmoothingTime(double t)
		{
			if (smoothingTimeMs != t)
			{
				smoothingTimeMs = t;
				refreshSmoothingTime();
			}
		}

		virtual float get() const = 0;
		virtual void reset() = 0;
		virtual void set(double v) = 0;
		virtual float advance() = 0;

		virtual void prepare(PrepareSpecs ps)
		{
			currentBlockRate = ps.sampleRate / (double)ps.blockSize;
			refreshSmoothingTime();
		}

		virtual void refreshSmoothingTime() = 0;

		virtual HISE_EMPTY_INITIALISE;

		double currentBlockRate = 0.0;
		double smoothingTimeMs = 0.0;
	};

	struct no : public base
	{
		float get() const final override
		{
			return v;
		}

		void reset() final override {};

		void set(double nv) final override
		{
			v = nv;
		}

		float advance() final override
		{
			return v;
		}

		void refreshSmoothingTime() final override {};

		float v = 0.0f;
	};

	struct low_pass : public base
	{
		float get() const final override
		{
			return lastValue;
		}

		void reset() final override
		{
			isSmoothing = false;
			lastValue = target;
			s.resetToValue(target);
		}

		float advance() final override
		{
			if (isSmoothing)
			{
				auto thisValue = s.smooth(target);
				isSmoothing = std::abs(thisValue - target) > 0.001f;
				lastValue = thisValue;
				return thisValue;
			}

			return target;
		}

		void set(double targetValue) final override
		{
			auto tf = (float)targetValue;

			if (tf != target)
			{
				target = tf;
				isSmoothing = target != lastValue;
			}
		}

		void refreshSmoothingTime() final override
		{
			s.prepareToPlay(currentBlockRate);
			s.setSmoothingTime(smoothingTimeMs);
		}

	private:

		bool isSmoothing = false;
		float lastValue = 0.0f;
		float target = 0.0f;
		Smoother s;
	};

	struct linear_ramp : public base
	{
		void reset()final override
		{
			d.reset();
		}

		float advance() final override
		{
			return d.advance();
		}

		float get() const final override
		{
			return d.get();
		}

		void set(double newValue) final override
		{
			d.set(newValue);
		}

		void refreshSmoothingTime() final override
		{
			d.prepare(currentBlockRate, smoothingTimeMs);
		}

	private:

		sdouble d;
	};
}


namespace control
{
namespace pimpl
{
struct combined_parameter_base
{
	virtual ~combined_parameter_base() {};

	struct Data
	{
		double getPmaValue() const { dirty = false; return value * mulValue + addValue; }
		double getPamValue() const { dirty = false; return (value + addValue) * mulValue; }

		double value = 0.0;
		double mulValue = 1.0;
		double addValue = 0.0;
		mutable bool dirty = false;
	};

	virtual Data getUIData() const = 0;

	NormalisableRange<double> currentRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(combined_parameter_base);
};

/** Subclass this interface whenever you define a node that has a mode property that will be used
	as second template argument.

	Some nodes offer a compile-time-swappable logic that can be selected using the `Mode` property
	(most likely with a ModePropertyComboBox on the interface). The text of the combobox will be used
	to determine the template argument at the second position with the formula:

	node<somethingelse, namespace_id::text_as_underscore>

	In order for the cpp_generator to pick this up correctly, use this class as base-class and supply
	the node id (usually done by getStaticId()) and a namespace string in this constructor.

	Be aware that the classes inside this namespace have to match the text entries of the mode combobox
	(as lowercase strings).

	A neat way to use this class is the SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR macro:

	SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR(myClass, ParameterClass, "my_namespace");
*/
struct templated_mode
{
	virtual ~templated_mode() {};

	templated_mode(const Identifier& nodeId, const juce::String& modeNamespace)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(nodeId, PropertyIds::HasModeTemplateArgument);
		cppgen::CustomNodeProperties::setModeNamespace(nodeId, modeNamespace);
	}
};

/** Use this baseclass for nodes that do not process the signal. */
struct no_processing
{
	virtual ~no_processing() {};

	static constexpr bool isPolyphonic() { return false; };
	virtual HISE_EMPTY_INITIALISE;
	virtual HISE_EMPTY_PREPARE;

	static constexpr bool isNormalisedModulation() { return true; };
	bool handleModulation(double& d) { return false; };

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_RESET;
};

template <class ParameterType> struct parameter_node_base
{
	parameter_node_base(const Identifier& id)
	{
		// This forces any control node to bypass the wrap::mod wrapper...
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsControlNode);
	}

	virtual ~parameter_node_base() {};

	/** This method can be used to connect a target to the combined output of this
		node.
	*/
	template <int I, class T> void connect(T& t)
	{
		this->p.template getParameter<0>().template connect<I>(t);
	}

	auto& getParameter()
	{
		return p;
	}

	ParameterType p;
};

template <class ParameterType> struct duplicate_parameter_node_base : public parameter_node_base<ParameterType>,
																	  public wrap::duplicate_sender::Listener
{
	duplicate_parameter_node_base(const Identifier& id):
		parameter_node_base<ParameterType>(id)
	{
		this->getParameter().setParentNumVoiceListener(this);
	}

	virtual ~duplicate_parameter_node_base()
	{
		this->getParameter().setParentNumVoiceListener(nullptr);
	}
};

}
	
}



}
