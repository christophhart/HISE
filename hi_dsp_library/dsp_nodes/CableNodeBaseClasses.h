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




namespace control
{

namespace pimpl
{

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

/** Use this base class when you have a node that uses unnormalised modulation. This will cause the
    C++ generator to ignore the parameter range. 
*/
struct no_mod_normalisation
{
	virtual ~no_mod_normalisation() {};

	static constexpr bool isNormalisedModulation() { return false; }

	no_mod_normalisation(const Identifier& nodeId)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(nodeId, PropertyIds::UseUnnormalisedModulation);
	}
};

/** Use this baseclass for nodes that do not process the signal. */
struct no_processing
{
	virtual ~no_processing() {};

	static constexpr bool isPolyphonic() { return false; };
	virtual SN_EMPTY_INITIALISE;
	virtual SN_EMPTY_PREPARE;

	static constexpr bool isNormalisedModulation() { return true; };
	bool handleModulation(double& d) { return false; };

	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_PROCESS;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_RESET;
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

template <typename DataType> struct combined_parameter_base
{
	using InternalDataType = DataType;

	virtual ~combined_parameter_base() {};

	virtual DataType getUIData() const = 0;

	NormalisableRange<double> currentRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(combined_parameter_base);
};

template <class ParameterType> struct duplicate_parameter_node_base : public parameter_node_base<ParameterType>,
																	  public wrap::clone_manager::Listener
{
	duplicate_parameter_node_base(const Identifier& id):
		parameter_node_base<ParameterType>(id)
	{
		//this->getParameter().setParentNumVoiceListener(this);
	}

	virtual ~duplicate_parameter_node_base()
	{
//		this->getParameter().setParentNumVoiceListener(nullptr);
	}
};

}
	
}


}

