#if !defined(GLESLAYER_HELPER_H)
#define GLESLAYER_HELPER_H

#include "EGL/egl.h"
#include <string>
#include <unordered_map>

#define PATRACE_LAYER_NAME "patraceLayer"

typedef __eglMustCastToProperFunctionPointerType EGLFuncPointer;
typedef void* (*PFNEGLGETNEXTLAYERPROCADDRESSPROC) (void *, const char*);

typedef struct layer_data{
    std::unordered_map<std::string, EGLFuncPointer> nextLayerFuncMap;
    std::unordered_map<std::string, EGLFuncPointer> interceptFuncMap;
} LAYER_DATA;

extern std::unordered_map<std::string, LAYER_DATA *> gLayerCollector;

extern "C" void layer_init_intercept_map();

extern "C" void layer_init_next_proc_address(const char *layerName, void *layer_id, PFNEGLGETNEXTLAYERPROCADDRESSPROC get_next_layer_proc_address);

extern "C" bool layer_start_trace();

extern "C" void* dispatch_intercept_func(const char *layerName, const char* procName);

extern void create_traceout_handler();

#endif /* GLESLAYER_HELPER_H*/
