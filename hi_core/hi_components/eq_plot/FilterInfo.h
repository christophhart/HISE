#ifndef HI_FILTER_INFO_H_INCLUDED
#define HI_FILTER_INFO_H_INCLUDED

#include <complex>
#include <vector>

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

	void setCoefficients(int filterNum, double sampleRate, IIRCoefficients newCoefficients);

	bool isEnabled()
	{
		return enabled;
	}

	void setEnabled(bool shouldBeEnabled)
	{
		enabled = shouldBeEnabled;
	};
    
private:
    double fs;
    int numNumeratorCoeffs, numDenominatorCoeffs;

    std::vector <double> numeratorCoeffs;
    std::vector <double> denominatorCoeffs;
    double gainValue;

	bool enabled;
};

#endif