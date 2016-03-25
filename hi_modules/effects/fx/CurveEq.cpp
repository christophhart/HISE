
float CurveEq::getAttribute(int index) const
{
	if(index == -1) return 0.0f;

	const int filterIndex = index / BandParameter::numBandParameters;
	const BandParameter parameter = (BandParameter)(index % numBandParameters);

	StereoFilter *filter = filterBands[filterIndex];

	jassert(filter != nullptr);

	if (filter != nullptr)
	{
		switch (parameter)
		{
		case BandParameter::Gain:	 return Decibels::gainToDecibels((float)filter->getGain());
		case BandParameter::Freq:	 return (float)filter->getFrequency();
		case BandParameter::Q:		 return (float)filter->getQ();
		case BandParameter::Type:	 return (float)filter->getFilterType();
		case BandParameter::Enabled: return filter->isEnabled() ? 1.0f : 0.0f;
		case numBandParameters:
		default:                     return 0.0f;
		}
	}
	else
	{
		debugError(const_cast<CurveEq*>(this), "Invalid attribute index: " + String(index));
		return 0.0f;
	}

	

	
}

void CurveEq::setInternalAttribute(int index, float newValue)
{
	if (index == -1) return;

	const int filterIndex = index / BandParameter::numBandParameters;
	const BandParameter parameter = (BandParameter)(index % numBandParameters);

	StereoFilter *filter = filterBands[filterIndex];

	jassert(filter != nullptr);

	if (filter != nullptr)
	{
		switch (parameter)
		{
		case BandParameter::Gain:	filter->setGain(Decibels::decibelsToGain(newValue)); break;
		case BandParameter::Freq:	filter->setFrequency(newValue); break;
		case BandParameter::Q:		filter->setQ(newValue); break;
		case BandParameter::Type:	filter->setType((int)newValue); break;
		case BandParameter::Enabled:filter->setEnabled(newValue >= 0.5f); break;
		case numBandParameters:
		default:                    break;
		}
	}
	else
	{
		debugError(this, "Invalid attribute index: " + String(index));
	}
}

ProcessorEditorBody *CurveEq::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new CurveEqEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif
}