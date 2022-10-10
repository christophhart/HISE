#include <vector>
#include <string>

namespace faust {
	// std::string generateSHA1(const std::string& data) {
	// 	return ::generateSHA1(data);
	// }

	// std::string expandDSPFromFile(const std::string& filename, int argc, const char* argv[], std::string& sha_key, std::string& error_msg) {
	// 	return ::expandDSPFromFile(filename, argc, argv, sha_key, error_msg);
	// }

	// std::string expandDSPFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], std::string& sha_key, std::string& error_msg) {
	// 	return ::expandDSPFromString(name_app, dsp_content, argc, argv, sha_key, error_msg);
	// }

	bool generateAuxFilesFromFile(const std::string& filename, int argc, const char* argv[], std::string& error_msg) {
	    std::vector<char> buffer;
	    // allocate 4096 bytes as per spec in <faust/dsp/libfaust-c.h>
	    buffer.reserve(4096);
	    // make windows happy: provide the item we are virtually referencing later
	    buffer.push_back(0);
		auto res = ::generateCAuxFilesFromFile(filename.c_str(), argc, argv, &(buffer[0]));
		return res;
	}

	bool generateAuxFilesFromString(const std::string& name_app, const std::string& dsp_content, int argc, const char* argv[], std::string& error_msg) {
	    std::vector<char> buffer;
	    // allocate 4096 bytes as per spec in <faust/dsp/libfaust-c.h>
	    buffer.reserve(4096);
	    // make windows happy: provide the item we are virtually referencing later
	    buffer.push_back(0);
		auto res = ::generateCAuxFilesFromString(name_app.c_str(), dsp_content.c_str(), argc, argv, &(buffer[0]));
	    error_msg = (const char*)&(buffer[0]);
	    return res;
	}
}
