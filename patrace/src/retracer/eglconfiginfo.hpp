#if !defined(EGLCONFIGINFO_HPP)
#define EGLCONFIGINFO_HPP

#include <string>
#include <ostream>

// forward declaration
namespace Json
{
    class Value;
}
typedef void *EGLDisplay;
typedef void *EGLConfig;

struct EglConfigInfo
{
public:
    EglConfigInfo();
    EglConfigInfo(int _red, int _green, int _blue, int _alpha, int _depth, int _stencil, int _msaa_samples, int _msaa_sample_buffers);
    EglConfigInfo(EGLDisplay& display, EGLConfig& config);

    EglConfigInfo& operator = (const Json::Value& json);

    EglConfigInfo& override(const EglConfigInfo& rhs, bool verbose = true);
    bool isStrictEgl(const EglConfigInfo& rhs);
    bool isStrictColor(const EglConfigInfo& rhs);

    /// This checks two things:
    /// 1. MSAA is _exactly_ as configured
    /// 2. Color, depth and stencil are _at least_ as large as configured
    /// NOTE: This part is NOT the strict mode. This is just a sanity check to check our selected configuration.
    /// Everything except antialiasing is redundant as long as PAFW_Choose_EGL_Config is used above, but we still want to keep it.
    bool isSane(const EglConfigInfo& rhs);

    int red;
    int green;
    int blue;
    int alpha;
    // Depth can be 0, 16 or 24 bits for FBO
    int depth;
    // Stencil can be 0 or 8 for FBO
    int stencil;
    // Samples are usually 0, 2, 4, or 8
    int msaa_samples;
    int msaa_sample_buffers;

private:
    bool compare(int lhs, int rhs, const std::string& name);
    int getOverriddenValue(int newValue, int defaultValue);
    void printOverridenValues(const EglConfigInfo& rhs);
    void printOverridenValue(int lhs, int rhs, const std::string& name);
};

namespace {

inline std::ostream& operator << (std::ostream& o, const EglConfigInfo& info)
{
    o << "EGL_RED_SIZE:       " << info.red << "\n";
    o << "EGL_GREEN_SIZE:     " << info.green << "\n";
    o << "EGL_BLUE_SIZE:      " << info.blue << "\n";
    o << "EGL_ALPHA_SIZE:     " << info.alpha << "\n";
    o << "EGL_DEPTH_SIZE:     " << info.depth << "\n";
    o << "EGL_STENCIL_SIZE:   " << info.stencil << "\n";
    o << "EGL_SAMPLES:        " << info.msaa_samples << "\n";
    o << "EGL_SAMPLE_BUFFERS: " << info.msaa_sample_buffers << "\n";
    return o;
}

}

#endif // EGLCONFIGINFO_HPP
