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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

    
namespace core
{
        namespace panner_impl
        {
            // Template Alias Definition =======================================================
            
            using panner_ = wrap::frame<2, container::multi<fix<1, core::gain>, fix<1, core::gain>>>;
            
            struct instance : public no_mod<instance, panner_>
            {
                SET_HISE_NODE_ID("panner");
                
                void createParameters(Array<ParameterData>& data)
                {
                    // Node Registration ===============================================================
                    registerNode(get<0>(obj), "gain2");
                    registerNode(get<1>(obj), "gain3");
                    
                    // Parameter Initalisation =========================================================
                    setParameterDefault("gain2.Gain", -6.0206);
                    setParameterDefault("gain2.Smoothing", 20.0);
                    setParameterDefault("gain3.Gain", -6.0206);
                    setParameterDefault("gain3.Smoothing", 20.0);
                    
                    // Parameter Callbacks =============================================================
                    {
                        ParameterData p("Balance", { 0.0, 1.0, 0.01, 1.0 });
                        
                        auto param_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });
                        auto param_target2 = getParameter("gain3.Gain", { -100.0, 0.0, 0.1, 5.42227 });
                        
                        param_target1.addConversion(ConverterIds::DryAmount);
                        param_target2.addConversion(ConverterIds::WetAmount);
                        
                        p.db = [param_target1, param_target2, outer = p.range](double newValue)
                        {
                            auto normalised = outer.convertTo0to1(newValue);
                            param_target1(normalised);
                            param_target2(normalised);
                        };
                        
                        data.add(std::move(p));
                    }
                }
                
            };
            
        }
        
        using panner = panner_impl::instance;
    }
    

class HiseFxNodeFactory : public NodeFactory
{
public:

	HiseFxNodeFactory(DspNetwork* network);;
	Identifier getId() const override { return "core"; }
};

}
