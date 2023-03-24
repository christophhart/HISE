#include <iostream>

namespace faust {

    // llvm_dsp
    int llvm_dsp::getNumInputs()
    { return getNumInputsCDSPInstance((::llvm_dsp*)this); }
       
    int llvm_dsp::getNumOutputs()
    { return getNumOutputsCDSPInstance((::llvm_dsp*)this); }
        
    void llvm_dsp::buildUserInterface(UI* ui_interface)
    {
	    // buildUserInterfaceCDSPInstance((::llvm_dsp*)this, (::UI*) ui_interface);  // TODO look at UIGlue
    }
       
    int llvm_dsp::getSampleRate()
    { return getSampleRateCDSPInstance((::llvm_dsp*)this); }
        
    void llvm_dsp::init(int sample_rate)
    { initCDSPInstance((::llvm_dsp*)this, sample_rate); }
       
    void llvm_dsp::instanceInit(int sample_rate)
    { instanceInitCDSPInstance((::llvm_dsp*)this, sample_rate); }
    
    void llvm_dsp::instanceConstants(int sample_rate)
    { instanceConstantsCDSPInstance((::llvm_dsp*)this, sample_rate); }
    
    void llvm_dsp::instanceResetUserInterface()
    { instanceResetUserInterfaceCDSPInstance((::llvm_dsp*)this); }
        
    void llvm_dsp::instanceClear()
    { instanceClearCDSPInstance((::llvm_dsp*)this); }
        
    llvm_dsp* llvm_dsp::clone()
    { return (llvm_dsp*)cloneCDSPInstance((::llvm_dsp*)this); }
        
    void llvm_dsp::metadata(Meta* m)
    {
	    // metadataCDSPInstance((::llvm_dsp*)this, (::Meta*) m); // TODO Look at MetaGlue
    }
        
    void llvm_dsp::compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
    { computeCDSPInstance((::llvm_dsp*)this, count, inputs, outputs); }

    // // llvm_dsp_factory
    
    llvm_dsp_factory::~llvm_dsp_factory()
    { ::deleteCDSPFactory((::llvm_dsp_factory*)this); }

    // std::string llvm_dsp_factory::getName()
    // { return ((::llvm_dsp_factory*)this)->getName(); }
    
    // undefined reference
    // std::string llvm_dsp_factory::getTarget()
    // { return ((::llvm_dsp_factory*)this)->getTarget(); }
        
    // std::string llvm_dsp_factory::getSHAKey()
    // { return ((::llvm_dsp_factory*)this)->getSHAKey(); }
        
    // std::string llvm_dsp_factory::getDSPCode()
    // { return ((::llvm_dsp_factory*)this)->getDSPCode(); }
        
    // std::string llvm_dsp_factory::getCompileOptions()
    // { return ((::llvm_dsp_factory*)this)->getCompileOptions(); }
        
    // std::vector<std::string> llvm_dsp_factory::getLibraryList()
    // { return ((::llvm_dsp_factory*)this)->getLibraryList(); }
        
    // std::vector<std::string> llvm_dsp_factory::getIncludePathnames()
    // { return ((::llvm_dsp_factory*)this)->getIncludePathnames(); }
    
    // std::vector<std::string> llvm_dsp_factory::getWarningMessages()
    // { return ((::llvm_dsp_factory*)this)->getWarningMessages(); }
    
    llvm_dsp* llvm_dsp_factory::createDSPInstance()
    { return (llvm_dsp*)(createCDSPInstance((::llvm_dsp_factory*)this)); }
        
    // void llvm_dsp_factory::setMemoryManager(dsp_memory_manager* manager)
    // { ((::llvm_dsp_factory*)this)->setMemoryManager((::dsp_memory_manager*) manager); }
        
    // dsp_memory_manager* llvm_dsp_factory::getMemoryManager()
    // { return (dsp_memory_manager*) ((::llvm_dsp_factory*)this)->getMemoryManager(); }

    // LIBFAUST_API llvm_dsp_factory* getDSPFactoryFromSHAKey(const std::string& sha_key)
    // { return (llvm_dsp_factory*)::getDSPFactoryFromSHAKey(sha_key); }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromFile(const std::string& filename, int argc, const char* argv[], std::string& error_msg)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromFile(filename, argc, argv, error_msg); }

    LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    {
	    std::cout << "Calling C interface" << std::endl;
	    std::vector<char> buffer;
	    // allocate 4096 bytes as per spec in <faust/dsp/llvm-dsp-c.h>
	    buffer.reserve(4096);
	    // make windows happy: provide the item we are virtually referencing later
	    buffer.push_back(0);
	    auto res = (llvm_dsp_factory*)::createCDSPFactoryFromString(name_app.c_str(), dsp_content.c_str(), argc, argv, target.c_str(), &(buffer[0]), opt_level);
	    error_msg = (const char*)&(buffer[0]);
	    return res;
    }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromSignals(const std::string& name_app, tvec signals_vec, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromSignals(name_app, (::tvec) signals_vec, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromBoxes(const std::string& name_app, Box box, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromBoxes(name_app, (::Box) box, argc, argv, target, error_msg, opt_level); }

    LIBFAUST_API bool deleteDSPFactory(llvm_dsp_factory* factory)
    { return ::deleteCDSPFactory((::llvm_dsp_factory*) factory); }

//     LIBFAUST_API void deleteAllDSPFactories()
//     { ::deleteAllDSPFactories(); }

//     LIBFAUST_API std::vector<std::string> getAllDSPFactories()
//     { return ::getAllDSPFactories(); }

//     LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromBitcode(const std::string& bit_code, std::string& error_msg)
//     { return (llvm_dsp_factory*)::readDSPFactoryFromBitcode(bit_code, error_msg); }
    
//     LIBFAUST_API std::string writeDSPFactoryToBitcode(llvm_dsp_factory* factory)
//     { return ::writeDSPFactoryToBitcode((::llvm_dsp_factory*) factory); }
    
//     LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromBitcodeFile(const std::string& bit_code_path, std::string& error_msg)
//     { return (llvm_dsp_factory*)::readDSPFactoryFromBitcodeFile(bit_code_path, error_msg); }

    // LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromIR(const std::string& ir_code, const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::readDSPFactoryFromIR(ir_code, target, error_msg, opt_level); }

    // LIBFAUST_API std::string writeDSPFactoryToIR(llvm_dsp_factory* factory)
    // { return ::writeDSPFactoryToIR((::llvm_dsp_factory*) factory); }

    // LIBFAUST_API bool writeDSPFactoryToIRFile(llvm_dsp_factory* factory, const std::string& ir_code_path)
    // { return ::writeDSPFactoryToIRFile((::llvm_dsp_factory*) factory, ir_code_path); }

    // LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromIRFile(const std::string& ir_code_path, const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::readDSPFactoryFromIRFile(ir_code_path, target, error_msg, opt_level); }

    // LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromMachine(const std::string& machine_code, const std::string& target, std::string& error_msg)
    // { return (llvm_dsp_factory*)::readDSPFactoryFromMachine(machine_code, target, error_msg); }

    // LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromMachineFile(const std::string& machine_code_path, const std::string& target, std::string& error_msg)
    // { return (llvm_dsp_factory*)::readDSPFactoryFromMachineFile(machine_code_path, target, error_msg); }

    // LIBFAUST_API std::string writeDSPFactoryToMachine(llvm_dsp_factory* factory, const std::string& target)
    // { return ::writeDSPFactoryToMachine((::llvm_dsp_factory*) factory, target); }

    // LIBFAUST_API bool writeDSPFactoryToMachineFile(llvm_dsp_factory* factory, const std::string& machine_code_path, const std::string& target)
    // { return ::writeDSPFactoryToMachineFile((::llvm_dsp_factory*) factory, machine_code_path, target); }

    // LIBFAUST_API bool writeDSPFactoryToObjectcodeFile(llvm_dsp_factory* factory, const std::string& object_code_path, const std::string& target)
    // { return ::writeDSPFactoryToObjectcodeFile((::llvm_dsp_factory*) factory, object_code_path, target); }
}
