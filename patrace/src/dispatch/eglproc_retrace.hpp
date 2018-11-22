#ifndef _EGLPROC_RETRACE_HPP_
#define _EGLPROC_RETRACE_HPP_

#include <string>
extern void SetCommandLineEGLPath(const std::string& libEGL_path);
extern void SetCommandLineGLES1Path(const std::string& libGLESv1_path);
extern void SetCommandLineGLES2Path(const std::string& libGLESv2_path);

#endif
