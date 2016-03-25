#include "LuaLink.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProtoplugDir.h"
#include <map>

// exports for scripts
extern "C" {
#include "ProtoplugExports.h"
}

// could be avoided by namespacing every callback into a userdata (eg. theme api)
std::map<protolua::lua_State*, LuaLink*> globalStates;

// adapted from Lua 5.1 lbaselib.c
static int LuaWriteLine (protolua::lua_State *L) {
	LuaLink *luli = globalStates[L];
	if (!luli) return 0;
	String logLine;
	int n = luli->ls->gettop();  // number of arguments
	int i;
	luli->ls->getglobal("tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		luli->ls->pushvalue(-1);  // function to be called
		luli->ls->pushvalue(i);   // value to print
		luli->ls->pcall(1, 1, 0);
		s = luli->ls->tostring(-1);  // get result
		if (i>1) logLine += "\t";
		logLine += (s == NULL) ? "<cannot convert to string>" : s;
		luli->ls->pop(1);  // pop result
	}
	luli->addToLog(logLine);
	return 0;
}

static int LuaSetParam (protolua::lua_State *L) {
	LuaLink *luli = globalStates[L];
	if (!luli) return 0;
	luli->pfx->setParameterNotifyingHost((int)luli->ls->tonumber(1), (float)luli->ls->tonumber(2));
	return 0;
}

LuaLink::LuaLink(LuaProtoplugJuceAudioProcessor *_pfx)
{
	ls = nullptr;
	guiThreadRunning = workable = iLuaLoaded = 0;
	pfx = _pfx;
	customGui = 0;
	
	libFolder = ProtoplugDir::Instance()->getScriptsDir().getFullPathName();
	code = File(libFolder).getChildFile("default.lua").loadFileAsString();
	// unfortunately we can't know whether the host will call setStateInformation or not,
	// so the default script is always compiled even if we're loading something else
	compile();
}

LuaLink::~LuaLink()
{
	if (ls != nullptr)
	{

		//ls->close();

		ls = nullptr;
	}
	

}

void LuaLink::initProtoplugDir() {
	libFolder = ProtoplugDir::Instance()->getScriptsDir().getFullPathName();
	if (code.isEmpty())
		code = File(libFolder).getChildFile("default.lua").loadFileAsString();
}

void LuaLink::addToLog(String buf, bool isInput /*= false*/) {
	buf = buf.replace("\t", "    "); // TODO this does not line up like real tabs, obviously...
	if (log.length()>4000)
		log = log.substring(log.length()-3000);
	Time t = Time::getCurrentTime();
	if (isInput)
		log += String::formatted("\n%02i:%02i > ", t.getMinutes(), t.getSeconds());
	else
		log += String::formatted("\n%02i:%02i - ", t.getMinutes(), t.getSeconds());
	log += buf;
	ped = pfx->getProtoEditor();
	if (ped)
		ped->msg_UpdateLog = true;
}

void LuaLink::compile() {
	// first, write the code to a temp file in case of crash
	File kk = File::getCurrentWorkingDirectory().getChildFile("protoplug last compile.lua");
    if (kk.create().wasOk())
        kk.replaceWithText(code);

	// save to keep user's data across compilations
	String sd = save();
	if (sd.isNotEmpty())
		saveData = sd;

	// wait for running functions to finish and prevent new ones from happening
	workable = iLuaLoaded = 0;
	cs.enter();
	cs.exit();

	if (ls != nullptr) {
		preClose();
		globalStates.erase(ls->l);
		//delete ls;
		ls=nullptr;
	}

	ls = new protolua::LuaState(ProtoplugDir::Instance()->getLibDir());
	if (ls->failed) {
		addToLog(ls->errmsg);
		ls = nullptr;
		return;
	}
	ls->openlibs();
	ls->pushcclosure(LuaWriteLine, 0);
	ls->setglobal("print");
	ls->pushcclosure(LuaSetParam, 0);
	ls->setglobal("plugin_setParameter");

	// set globals
	ls->pushlightuserdata((void *)&pfx->params);
	ls->setglobal("plugin_params");
	ls->pushlightuserdata((void *)&customGui);
	ls->setglobal("gui_component");
	ls->pushstring(File::getSpecialLocation(File::currentExecutableFile).getFullPathName().getCharPointer().getAddress());
	ls->setglobal("protoplug_path");
	ls->pushstring(ProtoplugDir::Instance()->getDir().getFullPathName().getCharPointer().getAddress());
	ls->setglobal("protoplug_dir");
	ls->pushnumber(1);
	ls->setglobal("protoplug_version");
	ls->pushlightuserdata((AudioProcessor *)pfx);
	ls->setglobal("plugin_effect");

	// add plugin location to the include paths
	String addPath = ProtoplugDir::Instance()->getDir().getFullPathName();
	ls->getglobal( "package" );
	ls->getfield(-1, "path" );
	String newpack, pack = ls->tolstring(-1,0 );
	newpack << addPath << "/?.lua;" << pack;
	ls->pop(1 );
	ls->pushstring( newpack.getCharPointer().getAddress());
	ls->setfield(-2, "path" );
	ls->pop( 1 );

	// compile
	int error = ls->loadbuffer(code.getCharPointer().getAddress(), code.length(), "Lua Script");
	if (error)
	{
		addToLog(ls->tolstring(-1,0));
		return;
	}

	// now callbacks might happen : associate the lua state to this plugin instance
	globalStates[ls->l] = this;
	addToLog("compile successful");

	// run whole script
	int result = ls->pcall(0, 0, 0);
	if (result) {
		addToLog(ls->tostring(-1));
		globalStates.erase(ls->l);
		ls = nullptr;   // Cya, Lua
		return;
	}

	// call Init
	ls->getglobal( "script_init");
	if (ls->isfunction(-1)){
		result = ls->pcall( 0, 0, 0);
		if (result) {
			addToLog(String("error calling script_init() : ")+ls->tostring(-1));
			globalStates.erase(ls->l);
			ls = nullptr;
			return;
		}
	} else
		ls->pop( 1); // pop nil
	
	// bask in the glory of success
	workable = 1;

	if (saveData.isNotEmpty())
		load(saveData);

	return;
}

bool LuaLink::runString(String toRun) {
	if (!workable)
		return false;
	ls->loadstring(toRun.getCharPointer().getAddress());
	int result = ls->pcall(0, 0, 0);
	if (result) {
		addToLog(ls->tostring(-1));
		return false;
		// the rest still works, right?
		/*globalStates.erase(ls->l);
		delete ls;   // Cya, Lua
		ls=0;
		return;*/
	}
	return true;
}

void LuaLink::runStringInteractive(String toRun) {
	if (!workable)
		return;
	addToLog(toRun, true);
	if (!iLuaLoaded) {
		if (runString("require 'include/iluaembed'"))
			iLuaLoaded = true;
		else
			return;
	}
	callVoidOverride("ilua_runline"	, LUA_TSTRING, toRun.getCharPointer().getAddress(), 0);
}

void LuaLink::stackDump () {
	if (!workable)
		return;
	String dump = "Lua State Stack Dump : ";
    int i;
	int top = ls->gettop();
    for (i = 1; i <= top; i++) {
		int t = ls->type(i);
		switch (t) {
			case LUA_TSTRING:
				dump <<"`"<<ls->tostring(i)<<"'";
				break;
			case LUA_TBOOLEAN:
				dump<<"bool";
				break;
			case LUA_TNUMBER:
				dump << ls->tonumber(i);
				break;
			default:
				dump << ls->ltypename(t);
			break;
		}
		dump << "  "; // separator
    }
	dump << "<end of stack>";
    addToLog(dump);
}

bool LuaLink::startOverride(const char *fname)
{
	if (!workable) 
		return false;
	ls->getglobal( fname);
	if (!ls->isfunction( -1)) {
		ls->pop( 1); // pop nil or you will die
		return false;
	}
	return true;
}

int LuaLink::startVarargOverride(const char *fname, va_list _args)
{
	if (!startOverride(fname))
		return -1;
	int numArgs = 0;
	int type = 0;
	while (type = va_arg(_args, int)) {
		numArgs++;
		switch (type) {
		case LUA_TBOOLEAN:
			ls->pushboolean(va_arg(_args, bool));
			break;
		case LUA_TLIGHTUSERDATA:
			ls->pushlightuserdata(va_arg(_args, void*));
			break;
		case LUA_TNUMBER:
			ls->pushnumber(va_arg(_args, double));
			break;
		case LUA_TSTRING:
			ls->pushstring(va_arg(_args, char*));
			break;
		default:
			juce_breakDebugger;
		}
	}
	return numArgs;
}

int LuaLink::safepcall(const char *fname, int nargs, int nresults, int errfunc)
{
	int result = ls->pcall(nargs, nresults, errfunc);
	if (result) {
		addToLog(String("error calling ")+fname+String("() : ")+ls->tostring(-1));
		workable = 0;
		globalStates.erase(ls->l);
		ls = nullptr;
	}
	return result;
}

bool LuaLink::callVoidOverride(const char *fname, ...)
{
	const GenericScopedLock<CriticalSection> lok(cs);
    va_list args;
    va_start(args, fname);
	int numArgs = startVarargOverride(fname, args);
    va_end(args);
	if (numArgs<0)
		return false; // state or function does not exist
	safepcall (fname, numArgs, 0, 0);
	return true;
}

String LuaLink::callStringOverride(const char *fname, ...)
{
	const GenericScopedLock<CriticalSection> lok(cs);
    va_list args;
    va_start(args, fname);
	int numArgs = startVarargOverride(fname, args);
    va_end(args);
	if (numArgs<0)
		return String::empty; // state or function does not exist
	if (safepcall (fname, numArgs, 1, 0))
		return String::empty; // function crashed
	return safetostring();
}

bool LuaLink::callBoolOverride(const char *fname, ...)
{
	const GenericScopedLock<CriticalSection> lok(cs);
    va_list args;
    va_start(args, fname);
	int numArgs = startVarargOverride(fname, args);
    va_end(args);
	if (numArgs<0)
		return false; // state or function does not exist
	if (safepcall (fname, numArgs, 1, 0))
		return false; // function crashed
	return safetobool();
}

bool LuaLink::safetobool()
{
	if (!ls->isboolean(-1)) {
		ls->settop(0);
		return false; // there is no bool
	}
	bool ret = ls->toboolean(-1)!=0;
	ls->settop(0);
	return ret;
}

String LuaLink::safetostring()
{
	if (!ls->isstring(-1)) {
		ls->settop(0);
		return String::empty; // there is no string
	}
	String ret = ls->tostring(-1);
	ls->settop(0);
	return ret;
}

double LuaLink::getTailLengthSeconds()
{
	const GenericScopedLock<CriticalSection> lok(cs);
	if (!workable) 
		return 0.0;

	ls->getglobal( "plugin_getTailLengthSeconds");
	if (!ls->isfunction(-1)) {
		ls->pop( 1); // pop nil or you will die
		return 0.0;
	}
	int result = ls->pcall( 0, 1, 0);
	if (result) {
		addToLog(String("error calling plugin_getTailLengthSeconds() : ")+ls->tostring(-1));
		workable = 0;
		globalStates.erase(ls->l);
		ls = nullptr;   // Cya, Lua
		return 0.0;
	}
	if (!ls->isnumber(-1)) {
		ls->pop( 1);
		return 0.0;
	}
	double ret = ls->tonumber(-1);
	ls->pop( 1); // pop returned value
	return ret;
}

void LuaLink::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages, AudioPlayHead* ph)
{
	bool res = callVoidOverride("plugin_processBlock"	, LUA_TNUMBER, (double)buffer.getNumSamples(),
									LUA_TLIGHTUSERDATA, buffer.getArrayOfChannels(),
									LUA_TLIGHTUSERDATA, &midiMessages,
									LUA_TLIGHTUSERDATA, ph,
									LUA_TNUMBER, pfx->getSampleRate(),
									0);
#ifdef _PROTOGEN
	if (!res) {
		for (int channel = 0; channel < pfx->getNumOutputChannels(); ++channel)
		{
			buffer.clear (channel, 0, buffer.getNumSamples());
		}
	}
#endif
}

String LuaLink::getParameterName (int index)
{ return callStringOverride("plugin_getParameterName", LUA_TNUMBER, (double)index, 0); }

String LuaLink::getParameterText (int index)
{ return callStringOverride("plugin_getParameterText", LUA_TNUMBER, (double)index, 0); }

bool LuaLink::parameterText2Double (int index, String text, double &d)
{
	const GenericScopedLock<CriticalSection> lok(cs);
	if (startOverride("plugin_parameterText2Double")) {
		ls->pushnumber(index);
		ls->pushstring(text.getCharPointer().getAddress());
		safepcall ("plugin_parameterText2Double", 2, 1, 0);
		if (!ls->isnumber(-1)) {
			ls->settop(0);
			return false; // there is no number
		}
		double ret = ls->tonumber(-1);
		ls->settop(0);
		d = ret;
		return true;
	} else return false; // there is no function
}

void LuaLink::paramChanged (int index)
{ callVoidOverride("plugin_paramChanged"	, LUA_TNUMBER, (double)index, 0); }

String LuaLink::save ()
{ return callStringOverride("script_saveData", 0); }

void LuaLink::load(String loadData)
{ callVoidOverride("script_loadData"	, LUA_TSTRING, loadData.getCharPointer().getAddress(), 0); }

void LuaLink::preClose ()
{ callVoidOverride("script_preClose", 0); }

void LuaLink::paint (Graphics& g)
{ 
	if (!callVoidOverride("gui_paint"	, LUA_TLIGHTUSERDATA, &g, 0))
	{
		g.fillAll();
		g.setColour(Colours::grey);
		g.drawText("Override gui.paint to paint a gui here !", g.getClipBounds(), Justification::centred, false);
	}
}

void LuaLink::resized ()
{ callVoidOverride("gui_resized", 0); }

	
void LuaLink::mouseOverride (const char *fname, const MouseEvent& event)
{ 
	const GenericScopedLock<CriticalSection> lok(cs);
	if (startOverride(fname)) {
		exMouseEvent exEvent = MouseEvent2Struct(event);
		ls->pushlightuserdata(&exEvent);
		/* pass by value experimental version - allows Lua to manage the data,
		 * but not in the spirit of LuaJIT (i think) and probably slower
		exMouseEvent *em = (exMouseEvent *)ls->newuserdata(sizeof(exMouseEvent));
		em->initFrom(event);*/
		safepcall (fname, 1, 0, 0);
	}
}

void LuaLink::mouseMove (const MouseEvent& event)
{ mouseOverride ("gui_mouseMove", event); }

void LuaLink::mouseEnter (const MouseEvent& event)
{ mouseOverride ("gui_mouseEnter", event); }

void LuaLink::mouseExit (const MouseEvent& event)
{ mouseOverride ("gui_mouseExit", event); }
	
void LuaLink::mouseDown (const MouseEvent& event)
{ mouseOverride ("gui_mouseDown", event); }

void LuaLink::mouseDrag (const MouseEvent& event)
{ mouseOverride ("gui_mouseDrag", event); }

void LuaLink::mouseUp (const MouseEvent& event)
{ mouseOverride ("gui_mouseUp", event); }

void LuaLink::mouseDoubleClick (const MouseEvent& event)
{ mouseOverride ("gui_mouseDoubleClick", event); }

void LuaLink::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{
	const GenericScopedLock<CriticalSection> lok(cs);
	if (startOverride("gui_mouseWheelMove")) {
		exMouseEvent exEvent = MouseEvent2Struct(event);
		ls->pushlightuserdata(&exEvent);
		exMouseWheelDetails exWheel = MouseWheelDetails2Struct(wheel);
		ls->pushlightuserdata(&exWheel);
		// pass by value version (see above)
		//((exMouseEvent *)ls->newuserdata(sizeof(exMouseEvent)))->init(event);
		//((exMouseWheelDetails*)ls->newuserdata(sizeof(exMouseWheelDetails)))->init(wheel);
		safepcall ("gui_mouseWheelMove", 2, 0, 0);
	}
}

bool LuaLink::keyPressed (const KeyPress &key, Component *originatingComponent)
{
	const GenericScopedLock<CriticalSection> lok(cs);
	if (startOverride("gui_keyPressed")) {
		exKeyPress exKey = KeyPress2Struct(key);
		ls->pushlightuserdata(&exKey);
		ls->pushlightuserdata(originatingComponent);
		safepcall ("gui_keyPressed", 2, 0, 0);
		return safetobool();
	} else return false;
}

bool LuaLink::keyStateChanged (bool isKeyDown, Component *originatingComponent)
{ 
	const GenericScopedLock<CriticalSection> lok(cs);
	if (startOverride("gui_keyStateChanged")) {
		ls->pushboolean(isKeyDown);
		ls->pushlightuserdata(originatingComponent);
		safepcall ("gui_keyStateChanged", 2, 1, 0);
		return safetobool();
	} else return false;
}

void LuaLink::modifierKeysChanged (const ModifierKeys &modifiers)
{ callVoidOverride("gui_modifierKeysChanged", LUA_TNUMBER, (double)modifiers.getRawFlags(), 0); }

void LuaLink::focusGained (Component::FocusChangeType cause)
{ callVoidOverride("gui_focusGained", LUA_TNUMBER, (double)cause, 0); }

void LuaLink::focusLost (Component::FocusChangeType cause)
{ callVoidOverride("gui_focusLost", LUA_TNUMBER, (double)cause, 0); }

