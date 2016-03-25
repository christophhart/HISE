/*
  ==============================================================================

    Macros.h
    Created: 31 Aug 2015 1:39:43pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif

#if HI_USE_CONSOLE 
#define debugToConsole(p, x) (p->getMainController()->writeToConsole(x, 0, p))
#define debugError(p, x) (p->getMainController()->writeToConsole(x, 1, p))
#else
#define debugToConsole(p, x)
#define debugError(p, x)
#endif


#if USE_COPY_PROTECTION
#define CHECK_KEY(mainController){ if(FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(mainController)) fp->checkKey();}
#else
#define CHECK_KEY(mainController) {mainController;}
#endif


#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)



#ifdef JUCE_WINDOWS
#define GLOBAL_FONT() (Font("Tahoma", 13.0f, Font::plain))
#define GLOBAL_BOLD_FONT() (Font("Tahoma", 13.0f, Font::bold))
#define GLOBAL_MONOSPACE_FONT() (Font("Consolas", 14.0f, Font::plain))
#else
#define GLOBAL_FONT() (Font("Helvetica", 10.5f, Font::plain))
#define GLOBAL_BOLD_FONT() (Font("Helvetica", 10.5f, Font::bold))
#define GLOBAL_MONOSPACE_FONT() (Font("Menlo", 13.0f, Font::plain))
#endif


#define loadTable(tableVariableName, nameAsString) { const var savedData = v.getProperty(nameAsString, var::null); tableVariableName->restoreData(savedData); }
#define saveTable(tableVariableName, nameAsString) ( v.setProperty(nameAsString, tableVariableName->exportData(), nullptr) )

#if JUCE_DEBUG
#define START_TIMER() (startTimer(150))
#else
#define START_TIMER() (startTimer(30))
#endif

static float constrainTo0To1(float x)
{
	if (x > 1.0f) return 1.0f;
	else if (x < 0.0f) return 0.0f;
	else return x;
}

#define CONSTRAIN_TO_0_1(x){constrainTo0To1(x)}


#endif  // MACROS_H_INCLUDED
