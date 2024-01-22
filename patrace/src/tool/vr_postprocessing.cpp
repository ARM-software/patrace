#include <map>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include <unordered_map>
#include <vector>
#include <set>
#include <list>

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/out_file.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"

struct MakeCurrentInfo {
    int curDisp;
    int curDraw;
    int curContext;
    bool isTimeWarp;
    bool scissor_test = false;
};

static void printHelp()
{
    std::cout <<
        "Usage : vr_pp [OPTIONS] <source trace> <target trace>\n"
        "Options:\n"
        "  -h               print help\n"
        "  -v               print version\n"
        "  -b               backbuffer mode (turn on when processing a non-frontbuffer trace)\n"
        "  -smallBlit       Perform a small (16x16) blit on the screen instead of a full sized one\n"
        "  -daydream        Input trace is a Daydream VR trace\n"
        "  -gearvr          Input trace is a Gear VR trace\n"
        "  -skip_until_call Skip processing until this call\n"
        "\n"
        "WARNING: TOOL ASSUMES THIS IS A SYNCHRONOUS-TIMEWARP TRACE FOR GEAR VR\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

// Add this to get a (hopefully) free FBO handle we can use
const uint32_t blit_fbo_handle_offset = 10000;

static bool     is_backbuffer_trace = false;
static bool     is_daydream = false;
static bool     is_gearvr = false;
static bool     small_blit = false;
static uint32_t skip_until_call = 0;
static uint32_t blit_h = 512;
static uint32_t blit_w = 512;

std::string source_trace_filename;
std::string target_trace_filename;
std::string tmp_trace_filename;

std::map<uint32_t, MakeCurrentInfo> currentInfoMap;
uint32_t injectCount = 0;

// State tracking
uint32_t current_framebuffer = 0;
uint32_t current_2d_texture_handle = 0;
std::unordered_map<uint32_t,std::unordered_map<uint32_t, std::set<uint32_t> > > texture_handle_to_framebuffer;
std::unordered_map<uint32_t,std::unordered_map<uint32_t, uint32_t> > framebuffer_handle_to_texture;
std::unordered_map<uint32_t,std::set< std::vector<uint32_t> > > framebuffer_viewports;
std::unordered_map<uint32_t,std::pair<uint32_t,uint32_t>> texture_size_map;
std::set<uint32_t>    frame_fbos;
bool last_sync_call_is_fence = false;
bool last_sync_call_is_create_sync = false;

// Timewarp state tracking
std::set<uint32_t>    timewarp_input_2d_texture;
std::set<uint32_t>    timewarp_contexts;
std::set<uint32_t>    all_eyebuffer_textures;
uint32_t timewarp_active = 0;
uint32_t timewarp_drawcalls = 0;
bool is_right_eye = false;

// Used to determine surface size
uint32_t max_blits = 0;

// Load input and output traces
common::TraceFileTM inputFile;
common::OutFile tmpFile;


static void writeout(common::OutFile &outputFile, common::CallTM *call, bool injected = false)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];

    char *dest = call->Serialize(buffer, -1, injected);
    outputFile.Write(buffer, dest-buffer);
}

int parse_args(int argc, char **argv) {
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (!strcmp(arg, "-h"))
        {
            printHelp();
            return 0;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 0;
        }
        else if (!strcmp(arg, "-b"))
        {
            is_backbuffer_trace = true;
        }
        else if (!strcmp(arg, "-smallBlit"))
        {
            small_blit = true;
            blit_w = 16;
            blit_h = 16;
        }
        else if (!strcmp(arg, "-daydream"))
        {
            is_daydream = true;
        }
        else if (!strcmp(arg, "-gearvr"))
        {
            is_gearvr = true;
        }
        else if (!strcmp(arg, "-skip_until_call"))
        {
            skip_until_call = (uint32_t)atoi(argv[++argIndex]);
            PAT_DEBUG_LOG("Skip calls until call number %d\n", skip_until_call);
        }
        else
        {
            printf("Error: Unknown option %s\n", arg);
            printHelp();
            return 0;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 0;
    }

    if (is_daydream && is_gearvr) {
        printf("Trace can't be both Daydream and Gear VR. Select one.\n");
        return 0;
    }
    else if (!is_daydream && !is_gearvr) {
        is_daydream = true;
        printf("Trace type not specified. Defaulting to Daydream.\n");
    }

    if (is_daydream && is_backbuffer_trace) {
        printf("Trace can't be both Daydream and backbuffer trace. Select one.\n");
        return 0;
    }

    source_trace_filename = argv[argIndex++];
    target_trace_filename = argv[argIndex++];
    tmp_trace_filename = target_trace_filename + ".tmp";

    return 1;
}

int open_files(common::TraceFileTM *in_file, std::string in_file_name, common::OutFile *out_file, std::string out_file_name) {
    if (!in_file->Open(in_file_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for reading: %s\n", in_file_name.c_str());
        return 0;
    }

    if (!out_file->Open(out_file_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for reading: %s\n", out_file_name.c_str());
        return 0;
    }

    return 1;
}

void glBindFrameBuffer_common(common::CallTM *call) {
    uint32_t new_fbo = call->mArgs[1]->GetAsInt();

    current_framebuffer = new_fbo;
    frame_fbos.insert(current_framebuffer);
    DBG_LOG("Bind FBO %d\n", current_framebuffer);

    if(is_gearvr && current_framebuffer == 0) {
        currentInfoMap[call->mTid].isTimeWarp = true;
    }
    else if(is_gearvr && current_framebuffer != 0) {
        currentInfoMap[call->mTid].isTimeWarp = false;
    }
}

uint32_t create_blit_fbo(common::CallTM *call, bool is_multiview, uint32_t fbo_texture) {
    uint32_t fbo_to_blit_from = current_framebuffer + blit_fbo_handle_offset;

    // Generate it
    std::vector<GLuint> fbos;
    fbos.push_back(fbo_to_blit_from);
    common::ValueTM *fbosArray = common::CreateUInt32ArrayValue(fbos);

    common::CallTM genFrameBuffers("glGenFramebuffers");
    genFrameBuffers.mArgs.push_back(new common::ValueTM(1));
    genFrameBuffers.mArgs.push_back(fbosArray);
    genFrameBuffers.mRet = common::ValueTM();
    genFrameBuffers.mTid = call->mTid;
    writeout(tmpFile, &genFrameBuffers, true);

    // Bind it
    common::CallTM callBindFramebuffer("glBindFramebuffer");
    callBindFramebuffer.mArgs.push_back(new common::ValueTM(GL_FRAMEBUFFER));
    callBindFramebuffer.mArgs.push_back(new common::ValueTM(fbo_to_blit_from));
    callBindFramebuffer.mRet = common::ValueTM();
    callBindFramebuffer.mTid = call->mTid;
    writeout(tmpFile, &callBindFramebuffer, true);

    // Attach texture to it
    const char* call_name =  is_multiview ? "glFramebufferTextureLayer" : "glFramebufferTexture2D";
    common::CallTM callFBOTexture2D(call_name);
    callFBOTexture2D.mArgs.push_back(new common::ValueTM(GL_FRAMEBUFFER));
    callFBOTexture2D.mArgs.push_back(new common::ValueTM(GL_COLOR_ATTACHMENT0));
    if(!is_multiview) {
        callFBOTexture2D.mArgs.push_back(new common::ValueTM(GL_TEXTURE_2D));
    }
    callFBOTexture2D.mArgs.push_back(new common::ValueTM(fbo_texture));
    callFBOTexture2D.mArgs.push_back(new common::ValueTM(0));
    if(is_multiview) {
        callFBOTexture2D.mArgs.push_back(new common::ValueTM(0));
    }
    callFBOTexture2D.mRet = common::ValueTM();
    callFBOTexture2D.mTid = call->mTid;
    writeout(tmpFile, &callFBOTexture2D, true);

    // Bind old FBO
    common::CallTM callBindFramebuffer1("glBindFramebuffer");
    callBindFramebuffer1.mArgs.push_back(new common::ValueTM(GL_FRAMEBUFFER));
    callBindFramebuffer1.mArgs.push_back(new common::ValueTM(current_framebuffer));
    callBindFramebuffer1.mRet = common::ValueTM();
    callBindFramebuffer1.mTid = call->mTid;
    writeout(tmpFile, &callBindFramebuffer1, true);

    return fbo_to_blit_from;
}

void glBindFramebufferTexture2D_common(common::CallTM *call) {
    uint32_t tex_attachment = call->mArgs[1]->GetAsInt();
    uint32_t tex_target     = call->mArgs[2]->GetAsInt();
    uint32_t fbo_texture    = call->mArgs[3]->GetAsInt();
    uint32_t samples        = (call->mCallName == "glFramebufferTexture2DMultisampleEXT") ? call->mArgs[5]->GetAsInt() : 1;

    if (tex_target == GL_TEXTURE_2D && tex_attachment == GL_COLOR_ATTACHMENT0)
    {
        uint32_t fbo_to_blit_from = current_framebuffer;

        if (!(is_daydream && current_framebuffer == 0)) {

            // Is MSAA enabled, create FBO we can blit from
            if (samples > 1)
            {
                fbo_to_blit_from = create_blit_fbo(call, false, fbo_texture);
            }

            // Store mapping from texture-handle to FBO we can blit from
            texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext][fbo_texture].insert(fbo_to_blit_from);
            framebuffer_handle_to_texture[currentInfoMap[call->mTid].curContext][fbo_to_blit_from] = fbo_texture;

            DBG_LOG("Texture %d bound to FBO %d\n", fbo_texture, fbo_to_blit_from);
        }
    }
}

void glFramebufferTextureMultisampleMultiviewOVR_common(common::CallTM *call) {
    uint32_t tex_attachment = call->mArgs[1]->GetAsInt();
    uint32_t tex_target     = GL_TEXTURE_2D;
    uint32_t fbo_texture    = call->mArgs[2]->GetAsInt();
    uint32_t samples        = call->mArgs[4]->GetAsInt();

    if (tex_target == GL_TEXTURE_2D && tex_attachment == GL_COLOR_ATTACHMENT0)
    {
        uint32_t fbo_to_blit_from = current_framebuffer;

        // Is MSAA enabled, create FBO we can blit from
        if (samples > 1)
        {
            fbo_to_blit_from = create_blit_fbo(call, true, fbo_texture);
        }

        // Store mapping from texture-handle to FBO we can blit from
        texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext][fbo_texture].insert(fbo_to_blit_from);
        // TODO: The line below is completely untested due to the lack of a test case!
        framebuffer_handle_to_texture[currentInfoMap[call->mTid].curContext][fbo_to_blit_from] = fbo_texture;

        DBG_LOG("Texture %d bound to FBO %d\n", fbo_texture, fbo_to_blit_from);
    }
}

void eglCreateContext_common(common::CallTM *call) {
    //mArgs[3] is EGLint const *attrib_list
    for (unsigned int i = 0; i < call->mArgs[3]->mArrayLen; ++i )
    {
        if (call->mArgs[3]->mArray[i].mUint == EGL_CONTEXT_PRIORITY_LEVEL_IMG)
        {
            timewarp_contexts.insert(call->mRet.GetAsUInt());
            DBG_LOG("Created timewarp context =  %d\n", call->mRet.GetAsUInt());
            break;
        }
    }
}

void glBindTexture_common(common::CallTM *call) {
    uint32_t type   = call->mArgs[0]->GetAsInt();
    uint32_t handle = call->mArgs[1]->GetAsInt();

    if (type == GL_TEXTURE_2D)
    {
        current_2d_texture_handle = handle;
    }
}

void glTexStorage2D_common(common::CallTM *call) {
    // Ex: glTexStorage2D(target=GL_TEXTURE_2D, levels=1, internalformat=GL_RGBA8, width=1344, height=756)
    uint32_t target = call->mArgs[0]->GetAsInt();
    uint32_t width  = call->mArgs[3]->GetAsInt();
    uint32_t height = call->mArgs[4]->GetAsInt();

    if (target == GL_TEXTURE_2D) {
        texture_size_map[current_2d_texture_handle] = std::make_pair(width, height);
    }
}

void glDisable_common(common::CallTM *call) {
    GLenum cap = call->mArgs[0]->GetAsInt();
    if (cap == GL_SCISSOR_TEST)
    {
        currentInfoMap[call->mTid].scissor_test = false;
    }
}

void glEnable_common(common::CallTM *call) {
    GLenum cap = call->mArgs[0]->GetAsInt();
    if (cap == GL_SCISSOR_TEST)
    {
        currentInfoMap[call->mTid].scissor_test = true;
    }
}

void glMakeCurrent_common(common::CallTM *call) {
    currentInfoMap[call->mTid].curDisp = call->mArgs[0]->GetAsInt();
    currentInfoMap[call->mTid].curDraw = call->mArgs[1]->GetAsInt();
    currentInfoMap[call->mTid].curContext = call->mArgs[3]->GetAsInt();

    if(timewarp_contexts.count(currentInfoMap[call->mTid].curContext) == 0) {
        currentInfoMap[call->mTid].isTimeWarp = false;
    } else {
        currentInfoMap[call->mTid].isTimeWarp = true;
        DBG_LOG("Found timewarp thread %d\n", call->mTid);
    }
}

void eglCreatePbufferSurface_common(common::CallTM *call, common::OutFile &outFile,uint32_t overrideResWidth, uint32_t overrideResHeight)
{
    int dpy = call->mArgs[0]->GetAsInt();
    int config = call->mArgs[1]->GetAsInt();
    int ret = call->mRet.GetAsInt();
    int attrib_list[1] = {1};

    common::CallTM eglCreateWindowSurface2("eglCreateWindowSurface2");
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(dpy));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(config));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(-1));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM((const char *)attrib_list, 4));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(0));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(0));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(overrideResWidth));
    eglCreateWindowSurface2.mArgs.push_back(new common::ValueTM(overrideResHeight));
    eglCreateWindowSurface2.mRet = common::ValueTM(ret);
    eglCreateWindowSurface2.mTid = call->mTid;

    writeout(outFile, &eglCreateWindowSurface2, true);
}

void glBindTexture_gearvr(common::CallTM *call) {
    uint32_t type   = call->mArgs[0]->GetAsInt();
    uint32_t handle = call->mArgs[1]->GetAsInt();

    if (handle != 0)
    {
        if (type == GL_TEXTURE_2D || type == GL_TEXTURE_2D_ARRAY || type == GL_TEXTURE_CUBE_MAP)
        {
            DBG_LOG("Saw glBindTexture(%d, %d) during timewarp. Assuming this is a layer we should blit.\n", type, handle);
            timewarp_input_2d_texture.insert(handle);
        }
        else if (type == 0x8D65) // GL_TEXTURE_EXTERNAL_OES
        {
            DBG_LOG("Saw glBindTexture(%d, %d) during timewarp. This is an GL_TEXTURE_EXTERNAL_OES texture, so ignoring it.\n", type, handle);
        }
        else
        {
            DBG_LOG("Unknown texture-type bound during timewarp: %d (handle %d)\n", type, handle);
        }
    }
}

void glViewport_daydream(common::CallTM *call) {
    uint32_t x = call->mArgs[0]->GetAsInt();
    uint32_t y = call->mArgs[1]->GetAsInt();
    uint32_t width = call->mArgs[2]->GetAsInt();
    uint32_t height = call->mArgs[3]->GetAsInt();

     if (currentInfoMap[call->mTid].isTimeWarp) {
        // Right eye starts from the middle of the display
        is_right_eye = (x != 0);
    } else {
        std::vector<uint32_t> viewport {x, y, width, height};
        framebuffer_viewports[current_framebuffer].insert(viewport);
        framebuffer_viewports[current_framebuffer+blit_fbo_handle_offset].insert(viewport);

        DBG_LOG("Found candidate for viewport %d %d %d %d\n",x, y, width, height);
    }
}

void end_of_application_frame_daydream(common::CallTM *call, bool &timewarp_done, bool &skip_call) {
    // Detect end of frame by looking for a glFlush that follows a glFenceSync call
    if (is_daydream && !currentInfoMap[call->mTid].isTimeWarp && call->mCallName  == "glFlush" && last_sync_call_is_create_sync) {
        timewarp_done = true;
        skip_call = true;

        for(auto fbo : frame_fbos) {
            if (fbo != 0) {
                DBG_LOG("Detected FBO %d for frame\n", fbo);
                uint32_t eyebuffer_texture = framebuffer_handle_to_texture[currentInfoMap[call->mTid].curContext][fbo];
                if (all_eyebuffer_textures.find(eyebuffer_texture) != all_eyebuffer_textures.end()) {
                    if (eyebuffer_texture >= 0) {
                        DBG_LOG("Detected texture %d for FBO %d\n", eyebuffer_texture, fbo+blit_fbo_handle_offset);
                        timewarp_input_2d_texture.insert(eyebuffer_texture);
                    }
                }

                // TODO: Hacky way to handle multisampled FBOs. Just try both, use the one that succeeds.
                eyebuffer_texture = framebuffer_handle_to_texture[currentInfoMap[call->mTid].curContext][fbo+blit_fbo_handle_offset];
                if (all_eyebuffer_textures.find(eyebuffer_texture) != all_eyebuffer_textures.end()) {
                    if (eyebuffer_texture >= 0) {
                        DBG_LOG("Detected texture %d for FBO %d\n", eyebuffer_texture, fbo+blit_fbo_handle_offset);
                        timewarp_input_2d_texture.insert(eyebuffer_texture);
                    }
                }
            }
        }
        writeout(tmpFile, call);
    }
}


// TODO: refactor this monster function
void end_of_frame_common(common::CallTM *call) {
    DBG_LOG("Detected end of timewarp. Saw %u drawcalls, %lu bound 2D textures.\n", (unsigned)timewarp_drawcalls, (unsigned long)timewarp_input_2d_texture.size());
    timewarp_active = false;

    if ((is_gearvr && timewarp_drawcalls) || (is_daydream && call->mCallNo > skip_until_call))
    {
        common::CallTM callBindFramebuffer0("glBindFramebuffer");
        callBindFramebuffer0.mArgs.push_back(new common::ValueTM(GL_DRAW_FRAMEBUFFER));
        callBindFramebuffer0.mArgs.push_back(new common::ValueTM(0));
        callBindFramebuffer0.mRet = common::ValueTM();
        callBindFramebuffer0.mTid = call->mTid;
        writeout(tmpFile, &callBindFramebuffer0, true);

        // Disable scissor-test since this affects glBlitFramebuffer
        common::CallTM disableScissor("glDisable");
        disableScissor.mArgs.push_back(new common::ValueTM(GL_SCISSOR_TEST));
        disableScissor.mRet = common::ValueTM();
        disableScissor.mTid = call->mTid;
        writeout(tmpFile, &disableScissor, true);

        // Clear
        common::CallTM clear("glClear");
        clear.mArgs.push_back(new common::ValueTM(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
        clear.mRet = common::ValueTM();
        clear.mTid = call->mTid;
        writeout(tmpFile, &clear, true);

        // Inject glBlitFramebuffers for each layer
        unsigned i = 0;
        for (auto texture : timewarp_input_2d_texture)
        {
            uint32_t texture_id = texture;

            if(texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext].find(texture_id) == texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext].end()) {
                DBG_LOG("Couldn't find eye-buffer FBO for texture %d -- maybe it isn't an eye-buffer?\n", texture_id);
                continue;
            }

            for(auto fbo : texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext][texture_id]) {

                //uint32_t fbo = texture_handle_to_framebuffer[currentInfoMap[call->mTid].curContext][texture_id];
                DBG_LOG("Found eye-buffer FBO %d for texture %d\n", fbo, texture);

                common::CallTM bindReadFBO("glBindFramebuffer");
                bindReadFBO.mArgs.push_back(new common::ValueTM(GL_READ_FRAMEBUFFER));
                bindReadFBO.mArgs.push_back(new common::ValueTM(fbo));
                bindReadFBO.mRet = common::ValueTM();
                bindReadFBO.mTid = call->mTid;

                uint32_t tex_w = 1024;
                uint32_t tex_h = 1024;

                if (texture_size_map.find(texture) != texture_size_map.end())
                {
                    tex_w = texture_size_map[texture].first;
                    tex_h = texture_size_map[texture].second;
                } else
                {
                    if (is_gearvr) {
                        DBG_LOG("WARNING: Tried to look up size of texture but couldn't find it.\n");
                    }
                }

                if(is_gearvr) {
                    writeout(tmpFile, &bindReadFBO, true);
                    common::CallTM callBlitFramebuffer("glBlitFramebuffer");
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(0));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(0));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(tex_w));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(tex_h));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM((i+0) * blit_w));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(0));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM((i+0) * blit_w + blit_w));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(blit_h));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(GL_COLOR_BUFFER_BIT));
                    callBlitFramebuffer.mArgs.push_back(new common::ValueTM(GL_NEAREST));
                    callBlitFramebuffer.mRet = common::ValueTM();
                    callBlitFramebuffer.mTid = call->mTid;
                    writeout(tmpFile, &callBlitFramebuffer, true);

                    DBG_LOG("Blit FBO %d with size (%ux%u)\n", fbo, tex_w, tex_h);
                    i++;
                }
                else if (is_daydream) {
                    // For Daydream we blit the viewports the eyebuffer renderpasses used to draw to the FBO individually instead
                    // of blitting the whole FBO so we are sure that what we blit is the actual contents of the eye buffers

                    uint32_t blit_x = 0;
                    for (auto viewport : framebuffer_viewports[fbo]) {
                        writeout(tmpFile, &bindReadFBO, true);
                        uint32_t viewport_x = viewport[0];
                        uint32_t viewport_y = viewport[1];
                        uint32_t viewport_w = viewport[2];
                        uint32_t viewport_h = viewport[3];

                        // Make trace for on 1920x1080 screen assuming we have max 4 eye buffers
                        if (!small_blit) {
                            for(int j = 1; j < 10; j++) {
                                blit_w = viewport_w/j;
                                blit_h = viewport_h/j;

                                if (blit_w*4 <= 1920 && blit_h <= 1080) {
                                    break;
                                }
                            }
                        }

                        DBG_LOG("Changing blit resolution to %dx%d.\n", blit_w, blit_h);

                        common::CallTM callBlitFramebuffer("glBlitFramebuffer");
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(viewport_x));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(viewport_y));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(viewport_x+viewport_w));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(viewport_y+viewport_h));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(blit_x));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(0));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(blit_x + blit_w));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(blit_h));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(GL_COLOR_BUFFER_BIT));
                        callBlitFramebuffer.mArgs.push_back(new common::ValueTM(GL_NEAREST));
                        callBlitFramebuffer.mRet = common::ValueTM();
                        callBlitFramebuffer.mTid = call->mTid;
                        writeout(tmpFile, &callBlitFramebuffer, true);

                        DBG_LOG("Blit FBO %d from (%u %u %u %u) to (%u %u %u %u)\n", fbo, viewport_x, viewport_y, viewport_x+viewport_w, viewport_y+viewport_h, blit_x, 0, blit_x + blit_w, blit_h);
                        blit_x += blit_w;
                        i++;
                    }
                }
            }
        }

        max_blits = (i > max_blits) ? i : max_blits;

        // Re-enable scissor-test if it was enabled before
        if (currentInfoMap[call->mTid].scissor_test)
        {
            common::CallTM enableScissor("glEnable");
            enableScissor.mArgs.push_back(new common::ValueTM(GL_SCISSOR_TEST));
            enableScissor.mRet = common::ValueTM();
            enableScissor.mTid = call->mTid;

            writeout(tmpFile, &enableScissor, true);
        }

        // Inject an eglSwapBuffers if trace doesn't already have one after timewarp
        if ((is_gearvr && !is_backbuffer_trace) || is_daydream)
        {
            common::CallTM callSwapBuffers("eglSwapBuffers");
            callSwapBuffers.mArgs.push_back(new common::ValueTM(currentInfoMap[call->mTid].curDisp));
            callSwapBuffers.mArgs.push_back(new common::ValueTM(currentInfoMap[call->mTid].curDraw));
            callSwapBuffers.mRet = common::ValueTM((int)EGL_TRUE);
            timewarp_input_2d_texture.clear();

            callSwapBuffers.mTid = call->mTid;
            if (is_daydream) {
                frame_fbos.clear();
                framebuffer_viewports.clear();
            }

            if (callSwapBuffers.mTid >= 0) {
                writeout(tmpFile, &callSwapBuffers, true);
                DBG_LOG("Injected eglSwapBuffers in calls[%lu]\n", (long unsigned)call->mCallNo);
            }
        }

        DBG_LOG("----------------------------------\n");
        injectCount++;
    }
}

void filter_calls_common(common::CallTM *call, bool &skip_call) {
    // Filter out sync-object usage
    if (call->mCallName == "eglClientWaitSyncKHR" || call->mCallName == "eglDestroySyncKHR" || call->mCallName == "glFenceSync")
    {
        last_sync_call_is_fence = false;
        skip_call = true;
    }
    else if (call->mCallName == "glFenceSync") {
        last_sync_call_is_fence = true;
        skip_call = true;
    }
    else if (call->mCallName == "eglCreateSyncKHR") {
        last_sync_call_is_create_sync = true;
        skip_call = true;
    }

    if (currentInfoMap[call->mTid].isTimeWarp && !skip_call)
    {
        if (call->mCallName == "glFlush")
            skip_call = true;
        if (call->mCallName == "glInvalidateFramebuffer" || call->mCallName == "glInvalidateSubFramebuffer")
            skip_call = true;
        if ((call->mCallName == "glClear") || (call->mCallName == "glDrawElements"))
            skip_call = true;
    }
}

int write_trace_header(uint32_t &overrideResWidth, uint32_t &overrideResHeight) {
    // Write header
    Json::Value header = inputFile.mpInFileRA->getJSONHeader();
    Json::Value info;
    addConversionEntry(header, "vr_pp", source_trace_filename, info);

    // Update size
    Json::Value threadArray = header["threads"];
    unsigned numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return 0;
    }
    overrideResWidth = max_blits * blit_w;
    overrideResHeight = blit_h;
    for (unsigned i = 0; i < numThreads; i++)
    {
        unsigned tid = threadArray[i]["id"].asUInt();
        unsigned resW = threadArray[i]["winW"].asUInt();
        unsigned resH = threadArray[i]["winH"].asUInt();

        threadArray[i]["winW"] = overrideResWidth;
        threadArray[i]["winH"] = overrideResHeight;
        DBG_LOG("Trace header resolution override for thread[%d] from %d x %d -> %d x %d\n",
                tid, resW, resH, overrideResWidth, overrideResHeight);
    }
    header["threads"] = threadArray;
    header["forceSingleWindow"] = true;

    Json::FastWriter writer;
    const std::string json_header = writer.write(header);
    tmpFile.mHeader.jsonLength = json_header.size();
    tmpFile.WriteHeader(json_header.c_str(), json_header.size());
    return 1;
}

int update_surface_resolutions(uint32_t overrideResWidth, uint32_t overrideResHeight) {
    // Update resolution of surfaces
    common::OutFile outFile;
    if (!outFile.Open(target_trace_filename.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for writing: %s\n", target_trace_filename.c_str());
        return 0;
    }

    common::TraceFileTM inputTmpFile;
    if (!inputTmpFile.Open(tmp_trace_filename.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open for reading: %s\n", tmp_trace_filename.c_str());
        return 0;
    }

    // Iterate over all calls
    common::CallTM *call = NULL;
    while ((call = inputTmpFile.NextCall()))
    {
        // ret=-892076320 eglCreateWindowSurface2(dpy=-185888000, config=-767040784, win=-294846456, attrib_list={12344}, x=0, y=0, width=2560, height=1440)
        bool skip = false;
        if (call->mCallName == "eglCreateWindowSurface2")
        {
            call->mArgs[6]->mUint = overrideResWidth;
            call->mArgs[7]->mUint = overrideResHeight;
        }
        if(is_gearvr == true && call->mCallName == "eglCreatePbufferSurface")
        {
            eglCreatePbufferSurface_common(call, outFile, overrideResWidth, overrideResHeight);
            skip = true;
        }

        if(skip == false)
        {
            writeout(outFile, call);
        }
    }

    Json::Value header = inputTmpFile.mpInFileRA->getJSONHeader();

    Json::FastWriter writer;
    const std::string json_header = writer.write(header);

    outFile.mHeader.jsonLength = json_header.size();
    outFile.WriteHeader(json_header.c_str(), json_header.size());

    inputTmpFile.Close();
    outFile.Close();

    return 1;
}

int main(int argc, char **argv)
{
    if(!parse_args(argc, argv)) {
        return 1;
    }

    common::gApiInfo.RegisterEntries(common::parse_callbacks);

    open_files(&inputFile, source_trace_filename, &tmpFile, tmp_trace_filename);

    // Iterate over all calls
    common::CallTM *call = NULL;
    //common::CallTM *trailing_call = NULL;
    while ((call = inputFile.NextCall()))
    {
        bool skip_call = false;

        // Handling of calls common to Gear VR and Daydream
        if (call->mCallName == "glBindFramebuffer" && (call->mArgs[0]->GetAsInt() == GL_FRAMEBUFFER || call->mArgs[0]->GetAsInt() == GL_DRAW_FRAMEBUFFER))
        {
            glBindFrameBuffer_common(call);
        }
        // Update textureID-to-framebufferID mapping.
        // For framebuffers we might want to blit from later, make glBlitFramebuffer-compatible FBOs for this where necessary.
        else if ((call->mCallName == "glFramebufferTexture2D") || (call->mCallName == "glFramebufferTexture2DMultisampleEXT"))
        {
            glBindFramebufferTexture2D_common(call);
        }
        else if ((call->mCallName == "glFramebufferTextureMultisampleMultiviewOVR"))
        {
            glFramebufferTextureMultisampleMultiviewOVR_common(call);
        }
        else if (call->mCallName == "eglCreateContext")
        {
            eglCreateContext_common(call);
        }
        else if (call->mCallName == "glBindTexture")
        {
            glBindTexture_common(call);
        }
        else if (call->mCallName == "glTexStorage2D")
        {
            glTexStorage2D_common(call);
        }
        if (call->mCallName == "glDisable")
        {
            glDisable_common(call);
        }
        else if (call->mCallName == "glEnable")
        {
            glEnable_common(call);
        }
        else if (call->mCallName == "eglMakeCurrent")
        {
            glMakeCurrent_common(call);
        }

        if (is_daydream && !currentInfoMap[call->mTid].isTimeWarp &&call->mCallName == "glEGLImageTargetTexture2DOES")
        {
            DBG_LOG("Found texture %d bound to an external image on thread %d\n", current_2d_texture_handle, call->mTid);
            all_eyebuffer_textures.insert(current_2d_texture_handle);
        }

        bool timewarp_done = false;

        // Handle platform specific calls
        if (is_gearvr) {
            // If timewarp is active, track some state to detect what we need to blit
            if(currentInfoMap[call->mTid].isTimeWarp && timewarp_active) {
                if (call->mCallName == "glBindTexture") {
                    glBindTexture_gearvr(call);
                }
                else if (call->mCallName == "glDrawElements") {
                    timewarp_drawcalls++;
                }
                else if (call->mCallName  == "eglSwapBuffers")
                {
                    // If we hit a swap (happens in backbuffer traces), timewarp must be done.
                    timewarp_done = true;
                }
            }
            else if (!currentInfoMap[call->mTid].isTimeWarp && timewarp_active) {
                // Moving to non-timewarp thread on Gear VR with synchronous mode on. Timewarp must have ended.
                timewarp_done = true;
            }
            else if (currentInfoMap[call->mTid].isTimeWarp && !timewarp_active)
            {
                // Clear state-tracking if this is the start of timewarp
                timewarp_active = true;
                timewarp_drawcalls = 0;
                DBG_LOG("Detected start of timewarp.\n");
            }
        }
        else if (is_daydream) {
            // For Daydream we want to track the viewports used when drawing eyebuffers for blitting
            if (call->mCallName == "glViewport") {
                glViewport_daydream(call);
            }

            if (currentInfoMap[call->mTid].isTimeWarp && call->mCallName == "glFlush" && last_sync_call_is_fence && timewarp_active) {
                if (is_right_eye) {
                    DBG_LOG("Detected end of timewarp for right eye.\n");
                } else {
                    DBG_LOG("Detected end of timewarp for left eye.\n");
                }

                skip_call = true;
            }

            // Detect end of frame by looking for a glFlush that follows a glFenceSync call
            if (!currentInfoMap[call->mTid].isTimeWarp && call->mCallName  == "glFlush" && last_sync_call_is_create_sync) {
                end_of_application_frame_daydream(call, timewarp_done, skip_call);
            }

            // Filter out timewarp-thread
            if (currentInfoMap[call->mTid].isTimeWarp)
            {
                skip_call = true;
            }
        }

        // When timewarp is over, blit layers that were rendered.
        if (timewarp_done)
        {
            end_of_frame_common(call);
        }

        filter_calls_common(call, skip_call);

        if (!skip_call)
        {
            writeout(tmpFile, call);
        }
    }

    uint32_t overrideResWidth = 0;
    uint32_t overrideResHeight = 0;
    if (!write_trace_header(overrideResWidth, overrideResHeight)) {
        return 1;
    }

    DBG_LOG("Post processing finished, %d swapbuffers are injected\n", injectCount);
    inputFile.Close();
    tmpFile.Close();

    if (!update_surface_resolutions(overrideResWidth, overrideResHeight)) {
        return 1;
    }

    return 0;
}
