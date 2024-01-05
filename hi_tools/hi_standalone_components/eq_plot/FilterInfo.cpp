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
*	Some parts of this file are written by Sean Enderby.
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

//==============================================================================
FilterResponse::FilterResponse (double magnitudeInit, double phaseInit)
{
    magnitudeValue = magnitudeInit;
    phaseValue = phaseInit;
}

FilterResponse::~FilterResponse()
{
}

//===============================================================================
FilterInfo::FilterInfo()
{
    fs = 44100;
    
    numeratorCoeffs.resize (1, 0);
    numeratorCoeffs [0] = 1;
    denominatorCoeffs.resize (1, 0);
    denominatorCoeffs [0] = 1;
    
    numNumeratorCoeffs = 1;
    numDenominatorCoeffs = 1;
    
	enabled = true;

    gainValue = 1;
}

FilterInfo::~FilterInfo()
{
}

void FilterInfo::setSampleRate (double sampleRate)
{
    fs = sampleRate;
}

void FilterInfo::setGain (double gain)
{
    gainValue = gain;
}

FilterResponse FilterInfo::getResponse (double inputFrequency) const
{

    std::complex <double> normalisedFrequency (0, (2 * double_Pi * inputFrequency / fs));
    std::complex <double> z = pow (double_E, normalisedFrequency);
    
    std::complex <double> num (0, 0);
    std::complex <double> den (0, 0);
    
    for (int numOrder = 0; numOrder < numNumeratorCoeffs; numOrder++)
    {
        num += numeratorCoeffs [numOrder] / pow (z, numOrder);
    }
    
    for (int denOrder = 0; denOrder < numDenominatorCoeffs; denOrder++)
    {
        den += denominatorCoeffs [denOrder] / pow (z, denOrder);
    }
    
    std::complex <double> transferFunction = num / den;

    auto mag = abs(transferFunction);

    mag = std::pow(mag, (float)order);

    auto phase = arg(transferFunction);

    return FilterResponse (mag * gainValue, phase);
}

void FilterInfo::zeroCoeffs()
{
    for (int numOrder = 0; numOrder < numNumeratorCoeffs; numOrder++)
    {
        numeratorCoeffs [numOrder] = 0;
    }
    
    for (int denOrder = 1; denOrder < numDenominatorCoeffs; denOrder++)
    {
        denominatorCoeffs [denOrder] = 0;
    }
    
    denominatorCoeffs [0] = 1;
}
 
bool FilterInfo::setCoefficients(int /*filterNum*/, double /*sampleRate*/, std::pair<IIRCoefficients, int> newCoefficients)
{
	numNumeratorCoeffs = 3;
    numDenominatorCoeffs = 3;
        
    numeratorCoeffs.resize (3, 0);
    denominatorCoeffs.resize (3, 0);

	coefficients = newCoefficients.first;
    order = newCoefficients.second;
        
    zeroCoeffs();

	for (int numOrder = 0; numOrder < 3; numOrder++)
    {
        numeratorCoeffs [numOrder] = newCoefficients.first.coefficients[numOrder];
    }
        
    for (int denOrder = 1; denOrder < 3; denOrder++)
    {
        denominatorCoeffs [denOrder] = newCoefficients.first.coefficients [denOrder + 2];
    }
    
    gainValue = 1;
	return true;
}

void FilterInfo::setFilter (double frequency, FilterType filterType)
{
    numNumeratorCoeffs = 3;
    numDenominatorCoeffs = 3;
        
    numeratorCoeffs.resize (3, 0);
    denominatorCoeffs.resize (3, 0);
        
    zeroCoeffs();
        
    switch (filterType)
    {
        case LowPass:
            coefficients = IIRCoefficients::makeLowPass (fs, frequency);
            break;
            
        case HighPass:
            coefficients = IIRCoefficients::makeHighPass (fs, frequency);
            break;
            
        default: break;
    }
        
    for (int numOrder = 0; numOrder < 3; numOrder++)
    {
		
        numeratorCoeffs [numOrder] = coefficients.coefficients[numOrder];
    }
        
    for (int denOrder = 1; denOrder < 3; denOrder++)
    {
        denominatorCoeffs [denOrder] = coefficients.coefficients [denOrder + 2];
    }
    
    gainValue = 1;
}

void FilterInfo::setEqBand (double frequency, double Q, float gain, BandType eqType)
{
    numNumeratorCoeffs = 3;
    numDenominatorCoeffs = 3;
        
    numeratorCoeffs.resize (3, 0);
    denominatorCoeffs.resize (3, 0);
   
	//gain = Decibels::decibelsToGain(gain);

    zeroCoeffs();
        
    switch (eqType)
    {
        case LowShelf:
            coefficients = IIRCoefficients::makeLowShelf (fs, frequency, Q, gain);
            break;
            
        case HighShelf:
            coefficients = IIRCoefficients::makeHighShelf (fs, frequency, Q, gain);
            break;
       
        case Peak:
            coefficients = IIRCoefficients::makePeakFilter (fs, frequency, Q, gain);
            break;
           
        default: break;
    }
        
    for (int numOrder = 0; numOrder < 3; numOrder++)
    {
        numeratorCoeffs [numOrder] = coefficients.coefficients [numOrder];
    }
        
    for (int denOrder = 1; denOrder < 3; denOrder++)
    {

		

        denominatorCoeffs [denOrder] = coefficients.coefficients [denOrder + 2];
    }
    
    gainValue = 1;    
}

void FilterInfo::setCustom (std::vector <double> numCoeffs, std::vector <double> denCoeffs)
{
    numNumeratorCoeffs = (int)numCoeffs.size();
    numDenominatorCoeffs = (int)denCoeffs.size();
    
    numeratorCoeffs = numCoeffs;
    denominatorCoeffs = denCoeffs;
}

bool FilterDataObject::Broadcaster::registerAtObject(ComplexDataUIBase* obj)
{
	if (auto f = dynamic_cast<FilterDataObject*>(obj))
	{
		SimpleReadWriteLock::ScopedWriteLock sl(f->getDataLock());

		for (auto& d : f->internalData)
		{
			if (d.broadcaster == this)
				return false;
		}

		InternalData d;
		d.broadcaster = this;
		f->internalData.insert(d);
        f->getUpdater().sendDisplayChangeMessage(f->internalData.size()-1, sendNotificationAsync, true);
        return true;
	}
	
	return false;
}

bool FilterDataObject::Broadcaster::deregisterAtObject(ComplexDataUIBase* obj)
{
	if (auto f = dynamic_cast<FilterDataObject*>(obj))
	{
		SimpleReadWriteLock::ScopedWriteLock sl(f->getDataLock());

		for (int i = 0; i < f->internalData.size(); i++)
		{
			if (f->internalData[i].broadcaster == this)
			{
				f->internalData.removeElement(i);
				return true;
			}
		}

		return false;
	}
	
	return false;
}

FilterDataObject::FilterDataObject() :
	ComplexDataUIBase()
{
    getUpdater().addEventListener(this);
}

FilterDataObject::~FilterDataObject()
{
    getUpdater().removeEventListener(this);
	internalData.clear();
}

std::pair<juce::IIRCoefficients, int> FilterDataObject::getCoefficients(int index) const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	jassert(isPositiveAndBelow(index, internalData.size()));
	return internalData[index].coefficients;
}

std::pair<juce::IIRCoefficients, int> FilterDataObject::getCoefficientsForBroadcaster(Broadcaster* b) const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	for (const auto& d : internalData)
	{
		if (b == d.broadcaster.get())
			return d.coefficients;
	}

	return {};
}

int FilterDataObject::getNumCoefficients() const
{
	SimpleReadWriteLock::ScopedReadLock sl(getDataLock());

	return internalData.size();
}

void FilterDataObject::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if(t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		for(auto& d: internalData)
		{
			if(auto f = dynamic_cast<scriptnode::data::filter_base*>(d.broadcaster.get()))
			{
                d.coefficients = f->getApproximateCoefficients();
			}
		}
	}
}
} // namespace hise