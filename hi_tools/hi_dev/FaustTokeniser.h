/*
  ==============================================================================

    FaustCodeTokenizer.h
    Author:  Oliver Larkin

    Note: This file was taken from the Amati project repository. Original file:
    https://github.com/glocq/Amati/blob/master/Source/FaustCodeTokenizer.h
 
    This project is released under the GPL license so this file can't be
    part of a proprietary derivative of HISE.
 
    Therefore it is not included in builds with the USE_BACKEND macro disabled.
  ==============================================================================
*/

#pragma once

class FaustTokeniser   : public juce::CodeTokeniser
{
public:
  //==============================================================================
  FaustTokeniser();
  ~FaustTokeniser();
  
  static juce::StringArray getAllFaustKeywords();
    
  //==============================================================================
  int readNextToken (juce::CodeDocument::Iterator&) override;
  juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
  
  /** The token values returned by this tokeniser. */
  enum TokenType
  {
    tokenType_error = 0,
    tokenType_comment,
    tokenType_primitive,
    tokenType_operator,
    tokenType_identifier,
    tokenType_integer,
    tokenType_float,
    tokenType_string,
    tokenType_bracket,
    tokenType_punctuation
  };
  
private:
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FaustTokeniser)
};


