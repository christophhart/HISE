namespace faust {


    // llvm_dsp
    // int llvm_dsp::getNumInputs()
    // { return ((::llvm_dsp*)this)->getNumInputs(); }
       
    // int llvm_dsp::getNumOutputs()
    // { return ((::llvm_dsp*)this)->getNumOutputs(); }
        
    // void llvm_dsp::buildUserInterface(UI* ui_interface)
    // { ((::llvm_dsp*)this)->buildUserInterface((::UI*) ui_interface); }
       
    // int llvm_dsp::getSampleRate()
    // { return ((::llvm_dsp*)this)->getSampleRate(); }
        
    // void llvm_dsp::init(int sample_rate)
    // { ((::llvm_dsp*)this)->init(sample_rate); }
       
    // void llvm_dsp::instanceInit(int sample_rate)
    // { ((::llvm_dsp*)this)->instanceInit(sample_rate); }
    
    // void llvm_dsp::instanceConstants(int sample_rate)
    // { ((::llvm_dsp*)this)->instanceConstants(sample_rate); }
    
    // void llvm_dsp::instanceResetUserInterface()
    // { ((::llvm_dsp*)this)->instanceResetUserInterface(); }
        
    // void llvm_dsp::instanceClear()
    // { ((::llvm_dsp*)this)->instanceClear(); }
        
    // llvm_dsp* llvm_dsp::clone()
    // { return (llvm_dsp*)((::llvm_dsp*)this)->clone(); }
        
    // void llvm_dsp::metadata(Meta* m)
    // { ((::llvm_dsp*)this)->metadata((::Meta*) m); }
        
    // void llvm_dsp::compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
    // { ((::llvm_dsp*)this)->compute(count, inputs, outputs); }

    // llvm_dsp_factory
    
    // llvm_dsp_factory::~llvm_dsp_factory()
    // { ((::llvm_dsp_factory*)this)->~llvm_dsp_factory(); }

    std::string llvm_dsp_factory::getName()
    { return ((::llvm_dsp_factory*)this)->getName(); }
    
    // undefined reference
    // std::string llvm_dsp_factory::getTarget()
    // { return ((::llvm_dsp_factory*)this)->getTarget(); }
        
    std::string llvm_dsp_factory::getSHAKey()
    { return ((::llvm_dsp_factory*)this)->getSHAKey(); }
        
    std::string llvm_dsp_factory::getDSPCode()
    { return ((::llvm_dsp_factory*)this)->getDSPCode(); }
        
    std::string llvm_dsp_factory::getCompileOptions()
    { return ((::llvm_dsp_factory*)this)->getCompileOptions(); }
        
    std::vector<std::string> llvm_dsp_factory::getLibraryList()
    { return ((::llvm_dsp_factory*)this)->getLibraryList(); }
        
    std::vector<std::string> llvm_dsp_factory::getIncludePathnames()
    { return ((::llvm_dsp_factory*)this)->getIncludePathnames(); }
    

#if !FAUST_NO_WARNING_MESSAGES // if the compilation fails here, update Faust to > 2.50.4 or disable the flag in the projucer
    std::vector<std::string> llvm_dsp_factory::getWarningMessages()
    { return ((::llvm_dsp_factory*)this)->getWarningMessages(); }
#endif
    
    // llvm_dsp* llvm_dsp_factory::createDSPInstance()
    // { return (llvm_dsp*)((::llvm_dsp_factory*)this)->createDSPInstance(); }
        
    void llvm_dsp_factory::setMemoryManager(::faust::dsp_memory_manager* manager)
    { ((::llvm_dsp_factory*)this)->setMemoryManager((::dsp_memory_manager*) manager); }
        
    ::faust::dsp_memory_manager* llvm_dsp_factory::getMemoryManager()
    { return (::faust::dsp_memory_manager*) ((::llvm_dsp_factory*)this)->getMemoryManager(); }


    LIBFAUST_API llvm_dsp_factory* getDSPFactoryFromSHAKey(const std::string& sha_key)
    { return (llvm_dsp_factory*)::getDSPFactoryFromSHAKey(sha_key); }

    LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromFile(const std::string& filename, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    { return (llvm_dsp_factory*)::createDSPFactoryFromFile(filename, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromString(name_app, dsp_content, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromSignals(const std::string& name_app, tvec signals_vec, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromSignals(name_app, (::tvec) signals_vec, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API llvm_dsp_factory* createDSPFactoryFromBoxes(const std::string& name_app, Box box, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (llvm_dsp_factory*)::createDSPFactoryFromBoxes(name_app, (::Box) box, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API bool deleteDSPFactory(llvm_dsp_factory* factory)
    // { return ::deleteDSPFactory((::llvm_dsp_factory*) factory); }

    LIBFAUST_API void deleteAllDSPFactories()
    { ::deleteAllDSPFactories(); }

    LIBFAUST_API std::vector<std::string> getAllDSPFactories()
    { return ::getAllDSPFactories(); }

    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromBitcode(const std::string& bit_code, const std::string& target, std::string& error_msg, int opt_level)
    { return (llvm_dsp_factory*)::readDSPFactoryFromBitcode(bit_code, target, error_msg, opt_level); }
    
    LIBFAUST_API std::string writeDSPFactoryToBitcode(llvm_dsp_factory* factory)
    { return ::writeDSPFactoryToBitcode((::llvm_dsp_factory*) factory); }
    
    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromBitcodeFile(const std::string& bit_code_path, const std::string& target, std::string& error_msg, int opt_level)
    { return (llvm_dsp_factory*)::readDSPFactoryFromBitcodeFile(bit_code_path, target, error_msg, opt_level); }

    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromIR(const std::string& ir_code, const std::string& target, std::string& error_msg, int opt_level)
    { return (llvm_dsp_factory*)::readDSPFactoryFromIR(ir_code, target, error_msg, opt_level); }

    LIBFAUST_API std::string writeDSPFactoryToIR(llvm_dsp_factory* factory)
    { return ::writeDSPFactoryToIR((::llvm_dsp_factory*) factory); }

    LIBFAUST_API bool writeDSPFactoryToIRFile(llvm_dsp_factory* factory, const std::string& ir_code_path)
    { return ::writeDSPFactoryToIRFile((::llvm_dsp_factory*) factory, ir_code_path); }

    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromIRFile(const std::string& ir_code_path, const std::string& target, std::string& error_msg, int opt_level)
    { return (llvm_dsp_factory*)::readDSPFactoryFromIRFile(ir_code_path, target, error_msg, opt_level); }

    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromMachine(const std::string& machine_code, const std::string& target, std::string& error_msg)
    { return (llvm_dsp_factory*)::readDSPFactoryFromMachine(machine_code, target, error_msg); }

    LIBFAUST_API llvm_dsp_factory* readDSPFactoryFromMachineFile(const std::string& machine_code_path, const std::string& target, std::string& error_msg)
    { return (llvm_dsp_factory*)::readDSPFactoryFromMachineFile(machine_code_path, target, error_msg); }

    LIBFAUST_API std::string writeDSPFactoryToMachine(llvm_dsp_factory* factory, const std::string& target)
    { return ::writeDSPFactoryToMachine((::llvm_dsp_factory*) factory, target); }

    LIBFAUST_API bool writeDSPFactoryToMachineFile(llvm_dsp_factory* factory, const std::string& machine_code_path, const std::string& target)
    { return ::writeDSPFactoryToMachineFile((::llvm_dsp_factory*) factory, machine_code_path, target); }

    LIBFAUST_API bool writeDSPFactoryToObjectcodeFile(llvm_dsp_factory* factory, const std::string& object_code_path, const std::string& target)
    { return ::writeDSPFactoryToObjectcodeFile((::llvm_dsp_factory*) factory, object_code_path, target); }
}
