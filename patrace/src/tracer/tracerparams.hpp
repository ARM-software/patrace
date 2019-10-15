#ifndef _TRACECFG_H_
#define _TRACECFG_H_

#include <fstream>
#include <string>
#include <vector>

class TracerParams
{
public:
    bool EnableErrorCheck = false;                  // Enable error checking that is stored in trace file after each GLES call
    bool EnableActiveAttribCheck = true;            // Only query actually used active attributes. Fixes performance issues on Qcom
    bool InteractiveIntercept = false;              // Debugging tool
    bool FilterSupportedExtension = false;          // Respond with given list of extensions instead of what the driver says
    std::vector<std::string> SupportedExtensions;   // Our own list of extensions for the above option
    std::string SupportedExtensionsString;
    bool FlushTraceFileEveryFrame = false;          // Save trace file for each completed frame. Slower but safer.
    bool StateDumpAfterSnapshot = false;            // Debugging
    bool StateDumpAfterDrawCall = false;            // Debugging
    int UniformBufferOffsetAlignment = 256;         // Enforce an alignment that works crossplatform
    int ShaderStorageBufferOffsetAlignment = 256;   // As above
    int MaximumAnisotropicFiltering = 0;            // Anisotropic support. Must also add GL_EXT_texture_filter_anisotropic to SupportedExtensions
    bool ErrorOutOnBinaryShaders = true;            // Return an error if a program attempts to upload a binary shader
    bool DisableErrorReporting = false;             // Disable GLES error reporting callbacks

public:
    TracerParams();
    ~TracerParams();

private:
    void LoadParams();

    std::ifstream m_file;
};

extern TracerParams tracerParams;

#endif
