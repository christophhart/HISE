namespace faust {
	std::string generateSHA1(const std::string& data) {
		return ::generateSHA1(data);
	}

	std::string expandDSPFromFile(const std::string& filename, int argc, const char* argv[], std::string& sha_key, std::string& error_msg) {
		return ::expandDSPFromFile(filename, argc, argv, sha_key, error_msg);
	}

	std::string expandDSPFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], std::string& sha_key, std::string& error_msg) {
		return ::expandDSPFromString(name_app, dsp_content, argc, argv, sha_key, error_msg);
	}

	bool generateAuxFilesFromFile(const std::string& filename, int argc, const char* argv[], std::string& error_msg) {
		return ::generateAuxFilesFromFile(filename, argc, argv, error_msg);
	}

	bool generateAuxFilesFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], std::string& error_msg) {
		return ::generateAuxFilesFromString(name_app, dsp_content, argc, argv, error_msg);
	}
}
