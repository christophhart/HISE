/************************** BEGIN interpreter-dsp.h *****************************
FAUST Architecture File
Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
---------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

EXCEPTION : As a special exception, you may create a larger work
that contains this FAUST architecture section and distribute
that work under terms of your choice, so long as this FAUST
architecture section is not modified.
***************************************************************************/

#ifndef INTERPRETER_DSP_WRAP_H
#define INTERPRETER_DSP_WRAP_H

#ifdef _WIN32
#define DEPRECATED(fun) __declspec(deprecated) fun
#else
#define DEPRECATED(fun) fun __attribute__ ((deprecated));
#endif

#include <string>
#include <vector>

/*!
 \addtogroup interpretercpp C++ interface for compiling Faust code with the INTERPRETER backend.
 Note that the API is not thread safe: use 'startMTInterpreterDSPFactories/stopMTInterpreterDSPFactories' to use it in a multi-thread context.
 @{
 */
 
/**
 * Get the library version.
 * 
 * @return the library version as a static string.
 */
extern "C" LIBFAUST_API const char* getCLibFaustVersion();

namespace faust {
/**
 * DSP instance class with methods.
 */
class LIBFAUST_API interpreter_dsp : public dsp {
    
    private:
    
        // interpreter_dsp objects are allocated using interpreter_dsp_factory::createDSPInstance();
        interpreter_dsp() {}
    
    public:
        
        int getNumInputs();
       
        int getNumOutputs();
        
        void buildUserInterface(UI* ui_interface);
       
        int getSampleRate();
        
        void init(int sample_rate);
       
        void instanceInit(int sample_rate);
    
        void instanceConstants(int sample_rate);
    
        void instanceResetUserInterface();
        
        void instanceClear();
        
        interpreter_dsp* clone();
        
        void metadata(Meta* m);
        
        void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs);
    
};

/**
 * DSP factory class.
 */

class LIBFAUST_API interpreter_dsp_factory : public dsp_factory {

     public:
    
        virtual ~interpreter_dsp_factory();
        
        /**
         *  Return factory name:
         *  either the name declared in DSP with [declare name "foo"] syntax
         *  or 'filename' (if createInterpreterDSPFactoryFromFile is used)
         *  or 'name_app' (if createInterpreterDSPFactoryFromString is used)
        */
        std::string getName();
    
        /* Return factory INTERPRETER target (like 'i386-apple-macosx10.6.0:opteron')*/
        std::string getTarget();
        
        /* Return factory SHA key */
        std::string getSHAKey();
        
        /* Return factory expanded DSP code */
        std::string getDSPCode();
        
        /* Return factory compile options */
        std::string getCompileOptions();
        
        /* Get the Faust DSP factory list of library dependancies */
        std::vector<std::string> getLibraryList();
        
        /* Get the list of all used includes */
        std::vector<std::string> getIncludePathnames();
        
        /* Create a new DSP instance, to be deleted with C++ 'delete' */
        interpreter_dsp* createDSPInstance();
        
        /* Set a custom memory manager to be used when creating instances */
        void setMemoryManager(dsp_memory_manager* manager);
        
        /* Return the currently set custom memory manager */
        dsp_memory_manager* getMemoryManager();

};

/**
 * Get the target (triple + CPU) of the machine.
 *
 * @return the target as a string.
 */
LIBFAUST_API std::string getDSPMachineTarget();

/**
 * Get the Faust DSP factory associated with a given SHA key (created from the 'expanded' DSP source), 
 * if already allocated in the factories cache and increment it's reference counter. You will have to explicitly
 * use deleteInterpreterDSPFactory to properly decrement reference counter when the factory is no more needed.
 *
 * @param sha_key - the SHA key for an already created factory, kept in the factory cache
 *
 * @return a DSP factory if one is associated with the SHA key, otherwise a null pointer.
 */
LIBFAUST_API interpreter_dsp_factory* getInterpreterDSPFactoryFromSHAKey(const std::string& sha_key);

/**
 * Create a Faust DSP factory from a DSP source code as a file. Note that the library keeps an internal cache of all 
 * allocated factories so that the compilation of the same DSP code (that is same source code and 
 * same set of 'normalized' compilations options) will return the same (reference counted) factory pointer. You will have to explicitly
 * use deleteInterpreterDSPFactory to properly decrement the reference counter when the factory is no more needed.
 * 
 * @param filename - the DSP filename
 * @param argc - the number of parameters in argv array 
 * @param argv - the array of parameters (Warning : aux files generation options will be filtered (-svg, ...) --> use generateAuxFiles)
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value' 
 * since the maximum value may change with new INTERPRETER versions)
 *
 * @return a DSP factory on success, otherwise a null pointer.
 */ 
LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromFile(const std::string& filename,
                                                        int argc, const char* argv[],
                                                        std::string& error_msg);

/**
 * Create a Faust DSP factory from a DSP source code as a string. Note that the library keeps an internal cache of all 
 * allocated factories so that the compilation of the same DSP code (that is same source code and 
 * same set of 'normalized' compilations options) will return the same (reference counted) factory pointer. You will have to explicitly
 * use deleteInterpreterDSPFactory to properly decrement reference counter when the factory is no more needed.
 * 
 * @param name_app - the name of the Faust program
 * @param dsp_content - the Faust program as a string
 * @param argc - the number of parameters in argv array
 * @param argv - the array of parameters (Warning : aux files generation options will be filtered (-svg, ...) --> use generateAuxFiles)
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value' 
 * since the maximum value may change with new INTERPRETER versions)
 *
 * @return a DSP factory on success, otherwise a null pointer.
 */ 
LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromString(const std::string& name_app,
                                                        const std::string& dsp_content,
                                                        int argc, const char* argv[],
                                                        std::string& error_msg);

/**
 * Create a Faust DSP factory from a vector of output signals.
 * It has to be used with the signal API defined in libfaust-signal.h.
 *
 * @param name_app - the name of the Faust program
 * @param signals_vec - the vector of output signals
 * @param argc - the number of parameters in argv array
 * @param argv - the array of parameters
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value'
 * since the maximum value may change with new INTERPRETER versions)
 *
 * @return a DSP factory on success, otherwise a null pointer.
 */
// LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromSignals(const std::string& name_app, tvec signals_vec,
//                                                         int argc, const char* argv[],
//                                                         const std::string& target,
//                                                         std::string& error_msg,
//                                                         int opt_level = -1);

/**
 * Create a Faust DSP factory from a box expression.
 * It has to be used with the box API defined in libfaust-box.h.
 *
 * @param name_app - the name of the Faust program
 * @param box - the box expression
 * @param argc - the number of parameters in argv array
 * @param argv - the array of parameters
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value'
 * since the maximum value may change with new INTERPRETER versions)
 *
 * @return a DSP factory on success, otherwise a null pointer.
 */
// LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromBoxes(const std::string& name_app, Box box,
//                                                         int argc, const char* argv[],
//                                                         const std::string& target,
//                                                         std::string& error_msg,
//                                                         int opt_level = -1);

/**
 * Delete a Faust DSP factory, that is decrements it's reference counter, possibly really deleting the internal pointer. 
 * Possibly also delete DSP pointers associated with this factory, if they were not explicitly deleted with C++ delete.
 * Beware: all kept factories and DSP pointers (in local variables...) thus become invalid.
 * 
 * @param factory - the DSP factory
 *
 * @return true if the factory internal pointer was really deleted, and false if only 'decremented'.
 */                                 
LIBFAUST_API bool deleteInterpreterDSPFactory(interpreter_dsp_factory* factory);

/**
 * Get the Faust DSP factory list of library dependancies.
 *
 * @deprecated : use factory getInterpreterDSPFactoryLibraryList method.
 *
 * @param factory - the DSP factory
 * 
 * @return the list as a vector of strings.
 */
DEPRECATED(std::vector<std::string> getInterpreterDSPFactoryLibraryList(interpreter_dsp_factory* factory));

/**
 * Delete all Faust DSP factories kept in the library cache. Beware: all kept factory and DSP pointers (in local variables...) thus become invalid.
 */                                 
LIBFAUST_API void deleteAllInterpreterDSPFactories();

/**
 * Return Faust DSP factories of the library cache as a vector of their SHA keys.
 * 
 * @return the Faust DSP factories.
 */                                 
LIBFAUST_API std::vector<std::string> getAllInterpreterDSPFactories();

/**
 * Start multi-thread access mode (since by default the library is not 'multi-thread' safe).
 * 
 * @return true if 'multi-thread' safe access is started.
 */ 
// extern "C" LIBFAUST_API bool startMTDSPFactories();

/**
 * Stop multi-thread access mode.
 */ 
// extern "C" LIBFAUST_API void stopMTDSPFactories();

/**
 * Create a Faust DSP factory from a base64 encoded INTERPRETER bitcode string. Note that the library keeps an internal cache of all 
 * allocated factories so that the compilation of the same DSP code (that is the same INTERPRETER bitcode string) will return 
 * the same (reference counted) factory pointer. You will have to explicitly use deleteInterpreterDSPFactory to properly 
 * decrement reference counter when the factory is no more needed.
 * 
 * @param bit_code - the INTERPRETER bitcode string
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value'
 * since the maximum value may change with new INTERPRETER versions). A higher value than the one used when calling createInterpreterDSPFactory can possibly be used.
 *
 * @return the DSP factory on success, otherwise a null pointer.
 */
LIBFAUST_API interpreter_dsp_factory* readInterpreterDSPFactoryFromBitcode(const std::string& bit_code, const std::string& target, std::string& error_msg, int opt_level = -1);

/**
 * Write a Faust DSP factory into a base64 encoded INTERPRETER bitcode string.
 * 
 * @param factory - the DSP factory
 *
 * @return the INTERPRETER bitcode as a string.
 */
LIBFAUST_API std::string writeInterpreterDSPFactoryToBitcode(interpreter_dsp_factory* factory);

/**
 * Create a Faust DSP factory from a INTERPRETER bitcode file. Note that the library keeps an internal cache of all 
 * allocated factories so that the compilation of the same DSP code (that is the same INTERPRETER bitcode file) will return 
 * the same (reference counted) factory pointer. You will have to explicitly use deleteInterpreterDSPFactory to properly 
 * decrement reference counter when the factory is no more needed.
 * 
 * @param bit_code_path - the INTERPRETER bitcode file pathname
 * @param target - the INTERPRETER machine target: like 'i386-apple-macosx10.6.0:opteron',
 *                 using an empty string takes the current machine settings,
 *                 and i386-apple-macosx10.6.0:generic kind of syntax for a generic processor
 * @param error_msg - the error string to be filled
 * @param opt_level - INTERPRETER IR to IR optimization level (from -1 to 4, -1 means 'maximum possible value'
 * since the maximum value may change with new INTERPRETER versions). A higher value than the one used when calling 
 * createInterpreterDSPFactory can possibly be used.
 * 
 * @return the DSP factory on success, otherwise a null pointer.
 */
LIBFAUST_API interpreter_dsp_factory* readInterpreterDSPFactoryFromBitcodeFile(const std::string& bit_code_path, const std::string& target, std::string& error_msg, int opt_level = -1);


/*!
 @}
 */
} // namespace faust
#endif
/************************** END interpreter-dsp.h **************************/
