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

#ifndef HI_FILTER_INFO_H_INCLUDED
#define HI_FILTER_INFO_H_INCLUDED

namespace hise { using namespace juce;

struct FilterCoefficientData
{
	typedef double (*PlotFunction)(void*, bool, double);

	IIRCoefficients first;
	int second;

	bool hasCustomFunction() const
	{
		return customFunction != nullptr;
	}

	double getFilterPlotValue(bool getMagnitude, double xAxisNormalised) const
	{
		if(hasCustomFunction())
		{
			if(obj == nullptr)
				return customFunction(const_cast<FilterCoefficientData*>(this), getMagnitude, xAxisNormalised);
			else
				return customFunction(obj, getMagnitude, xAxisNormalised);
		}

		return getFilterPlotValueForIIRCoefficients(this, getMagnitude, xAxisNormalised);
	}

	static double getFilterPlotValueForIIRCoefficients(const void* obj, bool getMagnitude, double xAxisNormalised);

	void* obj = nullptr;
	PlotFunction customFunction = nullptr;
};

#ifndef double_E
#define double_E 2.71828183
#endif
//====================================================================================
enum FilterType
{
	LowPass,
	HighPass
};
    
enum BandType
{
    LowShelf,
    HighShelf,
    Peak
};

//==============================================================================
class FilterResponse
{
public:
    FilterResponse (double magnitudeInit, double phaseInit);
    ~FilterResponse();
    
    double magnitudeValue, phaseValue;
};

//============================================================================
class FilterInfo : public IIRFilter
{
public:    
    FilterInfo();
    ~FilterInfo();
    
    void setSampleRate (double sampleRate);
    void setGain (double gain);
    
    FilterResponse getResponse (double inputFrequency) const;
    
    void zeroCoeffs();
    
    void setFilter (double frequency, FilterType filterType);
    void setEqBand (double frequency, double Q, float gain, BandType eqType);
    
    void setCustom (std::vector <double> numCoeffs, std::vector <double> denCoeffs);

	bool setCoefficients(int filterNum, double sampleRate, FilterCoefficientData newCoefficients);

	Array<double> toDoubleArray() const;
	void fromDoubleArray(Array<double>& d);

	bool isEnabled()
	{
		return enabled;
	}

	void setEnabled(bool shouldBeEnabled)
	{
		enabled = shouldBeEnabled;
	};
    
private:

	FilterCoefficientData cd;

	int order = 1;

    double fs;
    int numNumeratorCoeffs, numDenominatorCoeffs;

    std::vector <double> numeratorCoeffs;
    std::vector <double> denominatorCoeffs;
    double gainValue;

	bool enabled;
};

/** This data object holds a number of IIR coefficients and manages the notification / external management. */
struct FilterDataObject : public ComplexDataUIBase,
						  public ComplexDataUIUpdaterBase::EventListener
{
public:


	using CoefficientData = FilterCoefficientData;

	struct Broadcaster
	{
		virtual ~Broadcaster() {};

		bool registerAtObject(ComplexDataUIBase* obj);
		bool deregisterAtObject(ComplexDataUIBase* obj);

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Broadcaster);
	};

	struct InternalData
	{
		bool operator==(const InternalData& other) const { return broadcaster == other.broadcaster; }
		WeakReference<Broadcaster> broadcaster;
		CoefficientData coefficients;
	};

	using Ptr = ReferenceCountedObjectPtr<FilterDataObject>;

	FilterDataObject();;

	~FilterDataObject();

	bool fromBase64String(const String& b64) override { return true; };
	String toBase64String() const override { return ""; };

	void setSampleRate(double sr)
	{
		if (sampleRate != sr)
		{
			sampleRate = sr;
			getUpdater().sendDisplayChangeMessage(sampleRate, sendNotificationAsync);
		}
	}

	CoefficientData getCoefficients(int index) const;

	CoefficientData getCoefficientsForBroadcaster(Broadcaster* b) const;

	void sendUpdateFromBroadcaster(Broadcaster* b)
	{
		float idx = 0.0f;
		for(const auto& d: internalData)
		{
			if(d.broadcaster == b)
			{
				getUpdater().sendDisplayChangeMessage(idx, sendNotificationAsync, true);
				return;
			}

			idx += 1.0f;
		}
	}

	double getSamplerate() const { return sampleRate; }

	int getNumCoefficients() const;

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

private:
	
	double sampleRate = -1.0;

	UnorderedStack<InternalData> internalData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FilterDataObject);
};

} // namespace hise

#endif