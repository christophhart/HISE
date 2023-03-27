namespace faust {


    // interpreter_dsp
    // int interpreter_dsp::getNumInputs()
    // { return ((::interpreter_dsp*)this)->getNumInputs(); }
       
    // int interpreter_dsp::getNumOutputs()
    // { return ((::interpreter_dsp*)this)->getNumOutputs(); }
        
    // void interpreter_dsp::buildUserInterface(UI* ui_interface)
    // { ((::interpreter_dsp*)this)->buildUserInterface((::UI*) ui_interface); }
       
    // int interpreter_dsp::getSampleRate()
    // { return ((::interpreter_dsp*)this)->getSampleRate(); }
        
    // void interpreter_dsp::init(int sample_rate)
    // { ((::interpreter_dsp*)this)->init(sample_rate); }
       
    // void interpreter_dsp::instanceInit(int sample_rate)
    // { ((::interpreter_dsp*)this)->instanceInit(sample_rate); }
    
    // void interpreter_dsp::instanceConstants(int sample_rate)
    // { ((::interpreter_dsp*)this)->instanceConstants(sample_rate); }
    
    // void interpreter_dsp::instanceResetUserInterface()
    // { ((::interpreter_dsp*)this)->instanceResetUserInterface(); }
        
    // void interpreter_dsp::instanceClear()
    // { ((::interpreter_dsp*)this)->instanceClear(); }
        
    // interpreter_dsp* interpreter_dsp::clone()
    // { return (interpreter_dsp*)((::interpreter_dsp*)this)->clone(); }
        
    // void interpreter_dsp::metadata(Meta* m)
    // { ((::interpreter_dsp*)this)->metadata((::Meta*) m); }
        
    // void interpreter_dsp::compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
    // { ((::interpreter_dsp*)this)->compute(count, inputs, outputs); }

    // interpreter_dsp_factory
    
    // interpreter_dsp_factory::~interpreter_dsp_factory()
    // { ((::interpreter_dsp_factory*)this)->~interpreter_dsp_factory(); }

    std::string interpreter_dsp_factory::getName()
    { return ((::interpreter_dsp_factory*)this)->getName(); }
    
    // undefined reference
    // std::string interpreter_dsp_factory::getTarget()
    // { return ((::interpreter_dsp_factory*)this)->getTarget(); }
        
    std::string interpreter_dsp_factory::getSHAKey()
    { return ((::interpreter_dsp_factory*)this)->getSHAKey(); }
        
    std::string interpreter_dsp_factory::getDSPCode()
    { return ((::interpreter_dsp_factory*)this)->getDSPCode(); }
        
    std::string interpreter_dsp_factory::getCompileOptions()
    { return ((::interpreter_dsp_factory*)this)->getCompileOptions(); }
        
    std::vector<std::string> interpreter_dsp_factory::getLibraryList()
    { return ((::interpreter_dsp_factory*)this)->getLibraryList(); }
        
    std::vector<std::string> interpreter_dsp_factory::getIncludePathnames()
    { return ((::interpreter_dsp_factory*)this)->getIncludePathnames(); }
    
#if !FAUST_NO_WARNING_MESSAGES
    std::vector<std::string> interpreter_dsp_factory::getWarningMessages()
    { return ((::interpreter_dsp_factory*)this)->getWarningMessages(); }
#endif
    
    // interpreter_dsp* interpreter_dsp_factory::createDSPInstance()
    // { return (interpreter_dsp*)((::interpreter_dsp_factory*)this)->createDSPInstance(); }
        
    void interpreter_dsp_factory::setMemoryManager(dsp_memory_manager* manager)
    { ((::interpreter_dsp_factory*)this)->setMemoryManager((::dsp_memory_manager*) manager); }
        
    dsp_memory_manager* interpreter_dsp_factory::getMemoryManager()
    { return (dsp_memory_manager*) ((::interpreter_dsp_factory*)this)->getMemoryManager(); }


    LIBFAUST_API interpreter_dsp_factory* getInterpreterDSPFactoryFromSHAKey(const std::string& sha_key)
    { return (interpreter_dsp_factory*)::getInterpreterDSPFactoryFromSHAKey(sha_key); }

    LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromFile(const std::string& filename, int argc, const char* argv[], std::string& error_msg)
    { return (interpreter_dsp_factory*)::createInterpreterDSPFactoryFromFile(filename, argc, argv, error_msg); }

    // LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], std::string& error_msg)
    // { return (interpreter_dsp_factory*)::createInterpreterDSPFactoryFromString(name_app, dsp_content, argc, argv, error_msg); }

    // LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromSignals(const std::string& name_app, tvec signals_vec, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (interpreter_dsp_factory*)::createInterpreterDSPFactoryFromSignals(name_app, (::tvec) signals_vec, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API interpreter_dsp_factory* createInterpreterDSPFactoryFromBoxes(const std::string& name_app, Box box, int argc, const char* argv[], const std::string& target, std::string& error_msg, int opt_level)
    // { return (interpreter_dsp_factory*)::createInterpreterDSPFactoryFromBoxes(name_app, (::Box) box, argc, argv, target, error_msg, opt_level); }

    // LIBFAUST_API bool deleteInterpreterDSPFactory(interpreter_dsp_factory* factory)
    // { return ::deleteInterpreterDSPFactory((::interpreter_dsp_factory*) factory); }

    LIBFAUST_API void deleteAllInterpreterDSPFactories()
    { ::deleteAllInterpreterDSPFactories(); }

    LIBFAUST_API std::vector<std::string> getAllInterpreterDSPFactories()
    { return ::getAllInterpreterDSPFactories(); }

    LIBFAUST_API interpreter_dsp_factory* readInterpreterDSPFactoryFromBitcode(const std::string& bit_code, std::string& error_msg)
    { return (interpreter_dsp_factory*)::readInterpreterDSPFactoryFromBitcode(bit_code, error_msg); }
    
    LIBFAUST_API std::string writeInterpreterDSPFactoryToBitcode(interpreter_dsp_factory* factory)
    { return ::writeInterpreterDSPFactoryToBitcode((::interpreter_dsp_factory*) factory); }
    
    LIBFAUST_API interpreter_dsp_factory* readInterpreterDSPFactoryFromBitcodeFile(const std::string& bit_code_path, std::string& error_msg)
    { return (interpreter_dsp_factory*)::readInterpreterDSPFactoryFromBitcodeFile(bit_code_path, error_msg); }

}
