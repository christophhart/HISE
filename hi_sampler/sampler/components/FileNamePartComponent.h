/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_A08737CC2A7FE8DA__
#define __JUCE_HEADER_A08737CC2A7FE8DA__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;

class PopupLabel;

//[/Headers]



//==============================================================================

class FileNamePartComponent  : public Component,
                               public LabelListener
{
public:
    //==============================================================================
    FileNamePartComponent (const String &token);
    ~FileNamePartComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	/** A selection of useful properties that can be set via the filename token. */
	enum TokenProperties
	{
		VelocityLowHigh = 0, ///< Velocity Low & High + 1: maps the value to both ModulatorSamplerSound::LoVel and ModulatorSamplerSound::HiVel (+1). Use SampleImporter::fillGaps() after this.
		VelocityRange, ///< Use this with 'NumericRange' for velocity information like '63-127'
		VelocitySpread, ///< Spread Velocity to complete range: spreads the velocity evenly to the complete Range. Use the data type number and enter the upper limit into the item list or use the custom data type and enter the values '1 ... numItems'.
        LowVelocity, ///< the lower velocity limit
        HighVelocity, ///< the upper velocity limit
		SingleKey, ///< Set to single Key: maps the value to RootNote, LoKey and HiKey. SampleImporter::fillGaps might come in handy after this.
		Group, ///< RRGroup: moves the sound into the specified group.
		MultiMic, ///< The Multimic index. Using this property to merge multimics will not perform any sanity checks.
		Ignore, ///< Do nothing with this token. Use this for every token that does not contain special information (it is the default value anyway).
		numTokenProperties
	};

	/** Specifies the type of data which this token contains. */
	enum Datatype
	{
		Number, ///< a simple integer number that can be directly read without further processing.
		NumberWithRange, ///< use this for numbers that indicate a range and supply the upper limit into the item list.
		NumericRange, ///< a range in the format '1-63'
		NoteName, ///< if the token is a note name (format: "D#3"), use this data type to get the right midi note number (middle octave is 3).
		Custom, ///< if the token is a custom string, use this data type and fill in all possible items into the item list (with a space separator) and all values (as integer) into the value list.
		FixedValue, ///< discard all information and directly set a value to the fixed number that is entered into the item list.
		Ignored, ///< ignores the token. You should not have to use this directly, as it's the datatype for ignored tokens.
		numDataTypes
	};

    static Datatype getPreferedDataTypeFor(TokenProperties p)
    {
        switch(p)
        {
            case VelocityLowHigh: return Number;
            case VelocitySpread:  return Number;
            case VelocityRange:	  return NumberWithRange;
            case LowVelocity:     return Number;
            case HighVelocity:    return Number;
            case Group:			  return Custom;
			case MultiMic:		  return Custom;
            case SingleKey:		  return NoteName;
            case Ignore:		  return Ignored;
            default:			  jassertfalse; return Ignored;
        }
    }
    
	/** Returns the name for all TokenProperties. */
	static String getSpecialPropertyName(TokenProperties p)
	{
		switch(p)
		{
		case VelocityLowHigh: return "Velocity Value";
		case VelocitySpread:  return "Spread Velocity";
		case VelocityRange:	  return "Velocity Range";
        case LowVelocity:     return "Low Velocity";
        case HighVelocity:    return "High Velocity";
		case Group:			  return "RR Group";
		case MultiMic:		  return "Multi Mic";
		case SingleKey:		  return "Single Key";
		case Ignore:		  return "Ignore Token";
		default:			  jassertfalse; return "";
		}
	}

	/** Returns the Identifier for all TokenProperties. This is used for saving. */
	static String getDataTypeName(Datatype d)
	{
		switch (d)
		{
		case Number: return "Number";
		case NumberWithRange: return "NumberWithRange";
		case NumericRange: return "NumericRange";
		case NoteName: return "NoteName";
		case Custom: return "Custom";
		case FixedValue: return "FixedValue";
		case Ignored: return "Ignored";
		default: jassertfalse; return "";
		}
	}

	StringArray getSortedCustomList() const
	{
		StringArray sorted;

		for (auto v : valueList)
			sorted.add(customList[v-1]);

		return sorted;
	}

	/** extracts the data from the token and writes it into the supplied SamplerSoundBasicData object. */
	void fillDataWithTokenInformation(SampleImporter::SampleCollection &collection, int index, const String &currentToken)
	{
		const int value = getDataValue(currentToken);

		SampleImporter::SamplerSoundBasicData &data = collection.dataList.getReference(index);

		if(value == -1)
		{
			jassert(tokenProperty == Ignore);
			return;
		}

		switch (tokenProperty)
		{
		case SingleKey:			
		{
			auto valueToUse = value % 128;

			data.rootNote = valueToUse;
			data.lowKey = valueToUse;
			data.hiKey = valueToUse;
			
			// just bump the RR amount to avoid 
			// sticking them all at 127
			if (valueToUse != value)
				data.group = value / 128 + 1;

			break;
		}
								
		case Group:				data.group = value;
								break;
		case MultiMic:			data.multiMic = value - 1;
								collection.numMicPositions = customList.size();
								collection.multiMicTokens = getSortedCustomList();
								break;
		case VelocityLowHigh:	data.lowVelocity = value;
								data.hiVelocity = jmin(127, value + 1);
								break;
        case LowVelocity:       data.lowVelocity = value;
                                break;
        case HighVelocity:      data.hiVelocity = value;
                                break;
		case VelocityRange:		{
								StringArray sa = StringArray::fromTokens(currentToken, "-", "");
								jassert(sa.size() == 2);

								data.lowVelocity = sa[0].getIntValue();
								data.hiVelocity = sa[1].getIntValue();
								break;
								}
		case VelocitySpread:	{
								const int upperLimit = customList.size() == 1 ? customList[0].getIntValue() : customList.size();

								if(upperLimit > 0)
								{
									const int lowerIndex = value - 1;
									const int upperIndex = value;

									data.lowVelocity = (lowerIndex * 128) / upperLimit;
									data.hiVelocity = (upperIndex * 128) / upperLimit - 1;
									break;
								}

								data.lowVelocity = 0;
								data.hiVelocity = 127;
								break;
								}
		case Ignore:			break;
		default:				jassertfalse; return;

		}
	}


	/** writes all settings into an xml element which can be stored to save time. */
	XmlElement *exportSettings()
	{
		XmlElement *p = new XmlElement("panel");

		p->setAttribute("Property", getSpecialPropertyName(tokenProperty));
		p->setAttribute("DataType", getDataTypeName(tokenDataType));
		p->setAttribute("Items", itemLabel->getText());
		p->setAttribute("Values", valueLabel->getText());

		return p;
	}

	/** imports the settings from the supplied XmlElement. */
	void importSettings(XmlElement &p);

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int getDataValue(const String &token)
	{
		switch(tokenDataType)
		{
		case Number:	return token.getIntValue();
		case NoteName:	{
						String cleanedToken = token.toUpperCase().removeCharacters(" ");


						for(int i = 0; i < 127; i++)
							if(MidiMessage::getMidiNoteName(i, true, true, 3) == cleanedToken)
								return i;
						}
		case NumericRange: return 1; // don't return 0 or the rest will be skipped.
		case NumberWithRange:
						{
							if(valueRange.isEmpty() || customList.size() != 1) return token.getIntValue();
							else
							{
								const int upperLimit = customList[0].getIntValue();
								const int index = token.getIntValue();
								if(upperLimit != 0)
								{
									const float factor = (float)index / (float)upperLimit;

									return (int)(valueRange.getStart() + factor * valueRange.getLength());
								}
								else return token.getIntValue();
							}


						}
		case Custom:	{
						int index = customList.indexOf(token);
						jassert(index < valueList.size());
						return valueList[index];
						}
		case FixedValue: jassert(valueList.size() == 1);
			             return valueList[0];
		case Ignored:	return -1;
		default:		jassertfalse; return -1;
		}
	};

	void fillCustomList();

	String tokenName;

	TokenProperties tokenProperty;
	Datatype tokenDataType;


	StringArray customList;
	Array<int> valueList;

	Range<int> valueRange;

	Array<SampleImporter::SamplerSoundBasicData> dataList;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> separatorLabel;
    ScopedPointer<Label> partName;
    ScopedPointer<Label> displayGroupLabel;
    ScopedPointer<PopupLabel> propertyLabel;
    ScopedPointer<Label> displayGroupLabel2;
    ScopedPointer<PopupLabel> dataLabel;
    ScopedPointer<Label> displayGroupLabel3;
    ScopedPointer<Label> itemLabel;
    ScopedPointer<Label> displayGroupLabel4;
    ScopedPointer<Label> valueLabel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileNamePartComponent)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_A08737CC2A7FE8DA__
