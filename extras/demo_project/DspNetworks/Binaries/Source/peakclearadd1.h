#pragma once

#include "JuceHeader.h"

using namespace scriptnode;
using namespace snex;
using namespace snex::Types;

namespace PeakClearAdd1_impl
{
// =============| Node & Parameter type declarations |=============

using peak_mod = parameter::plain<math::add, 0>;
using peak_t = wrap::mod<peak_mod, core::peak>;

using PeakClearAdd1_t_ = container::chain<parameter::empty, 
                                          wrap::fix<2, peak_t>, 
                                          math::clear, 
                                          math::add>;

// =================| Root node initialiser class |=================

struct instance: public PeakClearAdd1_t_
{
	
	struct metadata
	{
		SNEX_METADATA_ID(instance);
		SNEX_METADATA_NUM_CHANNELS(2);
		SNEX_METADATA_ENCODED_PARAMETERS(2)
		{
			0x0000, 0x0000
		};
	};
	
	instance()
	{
		// Node References ----------------------------------------
		
		auto& peak = this->get<0>();  // PeakClearAdd1_impl::peak_t
		auto& clear = this->get<1>(); // math::clear
		auto& add = this->get<2>();   // math::add
		
		// Modulation Connections ---------------------------------
		
		peak.getParameter().connect<0>(add); // peak -> add::Value
		
		// Default Values -----------------------------------------
		
		clear.setParameter<0>(0.); // math::clear::Value
		
		; // add::Value is automated
		
	}
};
}

// ======================| Public Definition |======================

namespace project
{
using PeakClearAdd1 = wrap::node<PeakClearAdd1_impl::instance>;
}


