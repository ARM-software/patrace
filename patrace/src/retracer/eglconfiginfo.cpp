#include "eglconfiginfo.hpp"
#include "json/reader.h"
#include "common/os.hpp"
#include "dispatch/eglproc_auto.hpp"

EglConfigInfo::EglConfigInfo()
    : red(0)
    , green(0)
    , blue(0)
    , alpha(0)
    , depth(0)
    , stencil(0)
    , msaa_samples(0)
    , msaa_sample_buffers(0)
{
}

EglConfigInfo::EglConfigInfo(int _red, int _green, int _blue, int _alpha, int _depth, int _stencil, int _msaa_samples, int _msaa_sample_buffers)
    : red(_red)
    , green(_green)
    , blue(_blue)
    , alpha(_alpha)
    , depth(_depth)
    , stencil(_stencil)
    , msaa_samples(_msaa_samples)
    , msaa_sample_buffers(_msaa_sample_buffers)
{
}

EglConfigInfo::EglConfigInfo(EGLDisplay& display, EGLConfig& config)
    : red(0)
    , green(0)
    , blue(0)
    , alpha(0)
    , depth(0)
    , stencil(0)
    , msaa_samples(0)
    , msaa_sample_buffers(0)
{
    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alpha);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencil);
    eglGetConfigAttrib(display, config, EGL_SAMPLES, &msaa_samples);
    eglGetConfigAttrib(display, config, EGL_SAMPLE_BUFFERS, &msaa_sample_buffers);
}

EglConfigInfo& EglConfigInfo::operator = (const Json::Value& json)
{
    red = json["red"].asInt();
    green = json["green"].asInt();
    blue = json["blue"].asInt();
    alpha = json["alpha"].asInt();
    depth = json["depth"].asInt();
    stencil = json["stencil"].asInt();
    msaa_samples = json["msaaSamples"].asInt();
    return *this;
}

EglConfigInfo& EglConfigInfo::override(const EglConfigInfo& rhs, bool verbose)
{
    if (verbose)
    {
        printOverridenValues(rhs);
    }

    red = getOverriddenValue(rhs.red, red);
    green = getOverriddenValue(rhs.green, green);
    blue = getOverriddenValue(rhs.blue, blue);
    alpha = getOverriddenValue(rhs.alpha, alpha);
    depth = getOverriddenValue(rhs.depth, depth);
    stencil = getOverriddenValue(rhs.stencil, stencil);
    msaa_samples = getOverriddenValue(rhs.msaa_samples, msaa_samples);
    msaa_sample_buffers = getOverriddenValue(rhs.msaa_sample_buffers, msaa_sample_buffers);
    return *this;
}

bool EglConfigInfo::isStrictEgl(const EglConfigInfo& rhs)
{
    bool rhsUseSampleBuffers = rhs.msaa_sample_buffers == 1;
    bool lhsMsaaSpecified = msaa_samples > 0;

    if (msaa_samples == EGL_DONT_CARE ||
        lhsMsaaSpecified != rhsUseSampleBuffers)
    {
        DBG_LOG("MSAA samples error\n");
        return false;
    }

    return (isStrictColor(rhs) &&
            compare(depth, rhs.depth, "depth") &&
            compare(stencil, rhs.stencil, "stencil") &&
            compare(msaa_samples, rhs.msaa_samples, "MSAA samples"));
}
bool EglConfigInfo::isStrictColor(const EglConfigInfo& rhs)
{
    return (compare(red, rhs.red, "red") &&
            compare(green, rhs.green, "green") &&
            compare(blue, rhs.blue, "blue") &&
            compare(alpha, rhs.alpha, "alpha"));
}

bool EglConfigInfo::isSane(const EglConfigInfo& rhs)
{
    bool rhsUseSampleBuffers = rhs.msaa_sample_buffers == 1;
    bool lhsMsaaSpecified = msaa_samples > 0;
    bool lhsMsaaCare = msaa_samples != EGL_DONT_CARE;

    return (lhsMsaaCare &&
            lhsMsaaSpecified == rhsUseSampleBuffers &&
            msaa_samples == rhs.msaa_samples &&
            red <= rhs.red &&
            green <= rhs.green &&
            blue <= rhs.blue &&
            alpha <= rhs.alpha &&
            depth <= rhs.depth &&
            stencil <= rhs.stencil);
}

int EglConfigInfo::getOverriddenValue(int newValue, int defaultValue)
{
    return (newValue == -1) ? defaultValue : newValue;
}

void EglConfigInfo::printOverridenValues(const EglConfigInfo& rhs)
{
    printOverridenValue(red, rhs.red, "red");
    printOverridenValue(green, rhs.green, "green");
    printOverridenValue(blue, rhs.blue, "blue");
    printOverridenValue(alpha, rhs.alpha, "alpha");
    printOverridenValue(depth, rhs.depth, "depth");
    printOverridenValue(stencil, rhs.stencil, "stencil");
    printOverridenValue(msaa_samples, rhs.msaa_samples, "msaa_samples");
    printOverridenValue(msaa_sample_buffers, rhs.msaa_sample_buffers, "msaa_sample_buffers");
}

void EglConfigInfo::printOverridenValue(int lhs, int rhs, const std::string& name)
{
    if (getOverriddenValue(rhs, lhs) != lhs)
    {
        DBG_LOG("override %s %d -> %d\n", name.c_str(), lhs, rhs);
    }
}

bool EglConfigInfo::compare(int lhs, int rhs, const std::string& name)
{
    if (lhs != rhs)
    {
        DBG_LOG("Size error: %s differs %d != %d\n", name.c_str(), lhs, rhs);
        return false;
    }

    return true;
}

