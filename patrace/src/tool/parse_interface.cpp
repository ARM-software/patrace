#include <cassert>
#include <algorithm>
#include "common/memory.hpp"

#include "parse_interface.h"
#include "specs/pa_func_to_version.h"
#include "glsl_parser.h"
#include "glsl_utils.h"
#include "gl_utility.hpp"
#include "dispatch/eglproc_auto.hpp"

#pragma GCC diagnostic ignored "-Wunused-variable"

static const bool print_dupes = false;
Position current_pos = { 0, 0 };

void ParseInterfaceBase::setEglConfig(StateTracker::EglConfig& config, int attribute, int value)
{
    if (attribute == EGL_SAMPLES) config.samples = value;
    else if (attribute == EGL_BLUE_SIZE) config.blue = value;
    else if (attribute == EGL_RED_SIZE) config.red = value;
    else if (attribute == EGL_GREEN_SIZE) config.green = value;
    else if (attribute == EGL_DEPTH_SIZE) config.depth = value;
    else if (attribute == EGL_STENCIL_SIZE) config.stencil = value;
    else if (attribute == EGL_ALPHA_SIZE) config.alpha = value;
}

// Does not set number of primitives. This needs to be handled separately, since we cannot query patch size.
DrawParams ParseInterfaceBase::getDrawCallCount(common::CallTM *call)
{
    DrawParams ret;

    ret.index_buffer = nullptr; // clientsidebuffer contents currently not available without running replayer
    int ptr_idx = -1;
    if (call->mCallName == "glDrawElements" || call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glDrawElementsBaseVertex"
        || call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        ret.count = ret.vertices = call->mArgs[1]->GetAsUInt();
        ret.value_type = call->mArgs[2]->GetAsUInt();
        ptr_idx = 3;
    }
    else if (call->mCallName == "glDrawArrays" || call->mCallName == "glDrawArraysInstanced")
    {
        ret.first_index = call->mArgs[1]->GetAsUInt();
        ret.count = ret.vertices = call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements" || call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        ret.count = ret.vertices = call->mArgs[3]->GetAsUInt();
        ret.value_type = call->mArgs[4]->GetAsUInt();
        ptr_idx = 5;
    }
    else if (call->mCallName == "glDrawArraysIndirect")
    {
        ptr_idx = 1;
    }
    else if (call->mCallName == "glDrawElementsIndirect")
    {
        ptr_idx = 2;
    }

    if ((call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glDrawElementsInstancedBaseVertex"))
    {
        ret.instances = call->mArgs[4]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArraysInstanced")
    {
        ret.instances = call->mArgs[3]->GetAsUInt();
    }
    assert(ret.instances >= 0);

    if (ptr_idx != -1 && call->mArgs[ptr_idx]->mOpaqueType == common::ClientSideBufferObjectReferenceType)
    {
        ret.client_side_buffer_name = call->mArgs[ptr_idx]->mOpaqueIns->mClientSideBufferName;
        ret.client_side_buffer_offset = call->mArgs[ptr_idx]->mOpaqueIns->mClientSideBufferOffset;
    }

    ret.mode = call->mArgs[0]->GetAsUInt();
    ret.num_vertices_out = get_num_output_vertices(ret.mode, ret.vertices);
    ret.primitives = calculate_primitives(ret.mode, ret.vertices, contexts[context_index].patchSize);

    // Indirect draw calls not handled
    return ret;
}

/// Identify a previous clear that is a identical or a superset of the current clear. We could also try to
/// stitch together previous clears to match against the current here, but that seems much work for little
/// benefit at the moment. We could also track overlapping clears here, but those are probably not that common.
bool ParseInterfaceBase::find_duplicate_clears(const StateTracker::FillState& f, const StateTracker::Attachment& at, GLenum type, StateTracker::Framebuffer& fbo, const std::string& call)
{
    const std::vector<StateTracker::FillState>& prev = at.clears;
    assert(f.scissor.width != -1);
    fbo.total_clears[frames]++;
    for (const StateTracker::FillState& i : prev)
    {
        bool same = true; // assume it is same
        assert(i.scissor.width != -1);
        // Area test - if another clear is exactly the same size, it is a duplicate
        same &= (i.scissor.x == f.scissor.x && i.scissor.y == f.scissor.y && i.scissor.width == f.scissor.width && i.scissor.height == f.scissor.height);
        const char *typestr = "error";
        switch (type)
        {
        case GL_COLOR:
            typestr = "color";
            same &= (memcmp(i.clearcolor, f.clearcolor, sizeof(i.clearcolor)) == 0);
            for (int j = 0; j < 4; j++) same &= (i.colormask[j] && f.colormask[j]) || f.colormask[j] == false;
            break;
        case GL_DEPTH:
            typestr = "depth";
            same &= i.depthfunc == f.depthfunc;
            same &= i.cleardepth == f.cleardepth;
            break;
        case GL_STENCIL:
            typestr = "stencil";
            same &= (i.stencilwritemask.at(GL_FRONT) == f.stencilwritemask.at(GL_FRONT) || f.stencilwritemask.at(GL_FRONT) == false);
            same &= (i.stencilwritemask.at(GL_BACK) == f.stencilwritemask.at(GL_BACK) || f.stencilwritemask.at(GL_BACK) == false);
            same &= i.clearstencil == f.clearstencil;
            break;
        default: assert(false); break;
        }
        if (same)
        {
            if (print_dupes) DBG_LOG("DUPE CLEAR at %u duped from %u of type %s from %s of size %d,%d\n", f.call_stored, i.call_stored, typestr, call.c_str(), f.scissor.width, f.scissor.height);
            fbo.duplicate_clears[frames]++;
            return true;
        }
    }
    return false;
}

bool ParseInterface::open(const std::string& input, const std::string& output)
{
    filename = input;
    common::gApiInfo.RegisterEntries(common::parse_callbacks);
    if (!inputFile.Open(input.c_str()))
    {
        DBG_LOG("Failed to open for reading: %s\n", input.c_str());
        return false;
    }
    if (!output.empty() && !outputFile.Open(output.c_str()))
    {
        DBG_LOG("Failed to open for writing: %s\n", output.c_str());
        return false;
    }
    header = inputFile.getJSONHeader();
    threadArray = header["threads"];
    defaultTid = header["defaultTid"].asUInt();
    highest_gles_version = header["glesVersion"].asInt() * 10;
    DBG_LOG("Initial GLES version set to %d based on JSON header\n", highest_gles_version);
    numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return false;
    }
    const Json::Value eglconfig = inputFile.getJSONThreadById(defaultTid)["EGLConfig"];
    jsonConfig.red = eglconfig["red"].asInt();
    jsonConfig.green = eglconfig["green"].asInt();
    jsonConfig.blue = eglconfig["blue"].asInt();
    jsonConfig.alpha = eglconfig["alpha"].asInt();
    jsonConfig.depth = eglconfig["depth"].asInt();
    jsonConfig.stencil = eglconfig["stencil"].asInt();
    jsonConfig.samples = eglconfig.get("msaaSamples", -1).asInt();
    if (header.isMember("multiThread")) only_default = !header.get("multiThread", false).asBool();
    if (mForceMultithread) only_default = false;
    return true;
}

void ParseInterface::writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

static void unbind_renderbuffers_if(StateTracker::Context& context, const int fb_index, bool renderBuffer, GLuint id)
{
    if (fb_index == UNBOUND) return;
    for (auto& pair : context.framebuffers[fb_index].attachments)
    {
        if (pair.second.id == id
            && ((pair.second.type != GL_RENDERBUFFER && !renderBuffer)
                 || (pair.second.type == GL_RENDERBUFFER && renderBuffer)))
        {
            pair.second.id = 0;
        }
    }
}

common::CallTM* ParseInterface::next_call()
{
    void *fptr = nullptr;
    char *src = nullptr;
    common::BCall_vlen call;
    if (!inputFile.GetNextCall(fptr, call, src))
    {
        return nullptr;
    }
    delete mCall;
    mCall = new common::CallTM(inputFile, mCallNo, call);
    if (current_context.count(mCall->mTid) > 0)
    {
        context_index = current_context[mCall->mTid];
    }
    else context_index = UNBOUND;
    if (!only_default || mCall->mTid == defaultTid)
    {
        interpret_call(mCall);
    }
    mCallNo++;
    current_pos.call = mCallNo;
    return mCall;
}

static void adjust_sampler_state(common::CallTM* call, GLenum pname, StateTracker::SamplerState& state, common::ValueTM* arg)
{
    if (arg->IsArray() && arg->mArrayLen == 1)
    {
        arg = &arg->mArray[0];
    }

    if (!arg->IsArray())
    {
        switch (pname)
        {
        case GL_TEXTURE_CROP_RECT_OES:
            // ignored (GLES1 extension)
            break;
        case GL_TEXTURE_SRGB_DECODE_EXT:
            state.srgb_decode = arg->GetAsUInt();
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            state.anisotropy = arg->GetAsFloat();
            break;
        case GL_DEPTH_STENCIL_TEXTURE_MODE:
            state.depth_stencil_mode = arg->GetAsUInt();
            break;
        case GL_TEXTURE_BASE_LEVEL:
            state.base_level = arg->GetAsInt();
            break;
        case GL_TEXTURE_MAX_LEVEL:
            state.max_level = arg->GetAsInt();
            break;
        case GL_TEXTURE_WRAP_S:
            state.texture_wrap_s = arg->GetAsUInt();
            break;
        case GL_TEXTURE_WRAP_T:
            state.texture_wrap_t = arg->GetAsUInt();
            break;
        case GL_TEXTURE_WRAP_R:
            state.texture_wrap_r = arg->GetAsUInt();
            break;
        case GL_TEXTURE_MIN_FILTER:
            state.min_filter = arg->GetAsUInt();
            break;
        case GL_TEXTURE_MAG_FILTER:
            state.mag_filter = arg->GetAsUInt();
            break;
        case GL_TEXTURE_MIN_LOD:
            state.min_lod = arg->GetAsInt();
            break;
        case GL_TEXTURE_MAX_LOD:
            state.max_lod = arg->GetAsInt();
            break;
        case GL_TEXTURE_COMPARE_MODE:
            state.texture_compare_mode = arg->GetAsUInt();
            break;
        case GL_TEXTURE_COMPARE_FUNC:
            state.texture_compare_func = arg->GetAsUInt();
            break;
        case GL_TEXTURE_SWIZZLE_R:
            state.swizzle[0] = arg->GetAsUInt();
            break;
        case GL_TEXTURE_SWIZZLE_G:
            state.swizzle[1] = arg->GetAsUInt();
            break;
        case GL_TEXTURE_SWIZZLE_B:
            state.swizzle[2] = arg->GetAsUInt();
            break;
        case GL_TEXTURE_SWIZZLE_A:
            state.swizzle[3] = arg->GetAsUInt();
            break;
        default:
            DBG_LOG("Unsupported texture parameter for %s: 0x%04x (%s)\n", call->mCallName.c_str(), pname, texEnum(pname).c_str());
            break;
        }
    }
    else // non-length-of-one array values
    {
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            assert(arg->mArrayLen == 4);
            state.border[0] = arg->mArray[0].GetAsFloat();
            state.border[1] = arg->mArray[1].GetAsFloat();
            state.border[2] = arg->mArray[2].GetAsFloat();
            state.border[3] = arg->mArray[3].GetAsFloat();
        }
        else DBG_LOG("Unsupported texture array parameter for %s: 0x%04x (%s)\n", call->mCallName.c_str(), pname, texEnum(pname).c_str());
    }
}

void ParseInterfaceBase::new_renderpass(common::CallTM *call, StateTracker::Context& ctx, bool newframe)
{
    // First update existing renderpass info
    const int fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
    StateTracker::RenderPass &rp = ctx.render_passes.back();
    update_renderpass(call, ctx, rp, fb_index);
    // Create new (empty) renderpass
    ctx.render_passes.back().last_call = call->mCallNo;
    if (mDumpRenderpassJson) completed_renderpass(ctx.render_passes.back());
    int new_index = newframe ? 0 : (ctx.render_passes.back().index + 1);
    ctx.render_passes.emplace_back(new_index, call->mCallNo, frames, ctx.render_passes.size());
}

// Called for each draw call and when creating new renderpass to update renderpass info
void ParseInterfaceBase::update_renderpass(common::CallTM *call, StateTracker::Context& ctx, StateTracker::RenderPass &rp, const int fb_index)
{
    rp.drawframebuffer_index = fb_index;
    rp.attachments.resize(contexts[context_index].framebuffers[fb_index].attachments.size());
    int i = 0;
    for (auto& rb : contexts[context_index].framebuffers[fb_index].attachments)
    {
        if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN && rb.second.invalidated)
        {
            rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_DONT_CARE;
        }
        else if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN && rb.second.clears.size() > 0)
        {
            rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_CLEAR;
        }
        else if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN)
        {
            rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_LOAD;
        }
        rp.attachments[i].id = rb.second.id;
        rp.attachments[i].slot = rb.first;
        rb.second.clears.clear();
        rb.second.invalidated = false;
        if (rb.second.id > 0 && rb.second.type == GL_RENDERBUFFER && contexts[context_index].renderbuffers.contains(rb.second.id))
        {
            const int index = contexts[context_index].renderbuffers.remap(rb.second.id);
            rp.used_renderbuffers.insert(index);
            rp.render_targets[rb.first].first = GL_RENDERBUFFER;
            rp.render_targets[rb.first].second = index;
            rp.attachments[i].index = index;
            assert(rp.attachments[i].type == GL_RENDERBUFFER || rp.attachments[i].type == GL_NONE);
            rp.attachments[i].type = GL_RENDERBUFFER;
            StateTracker::Renderbuffer& r = contexts[context_index].renderbuffers.at(index);
            rp.attachments[i].format = r.internalformat;
            rp.width = r.width;
            rp.height = r.height;
            rp.depth = 1;
        }
        else if (rb.second.id > 0 && contexts[context_index].textures.contains(rb.second.id))
        {
            const int index = contexts[context_index].textures.remap(rb.second.id);
            rp.used_texture_targets.insert(index);
            rp.render_targets[rb.first].first = GL_TEXTURE;
            rp.render_targets[rb.first].second = index;
            rp.attachments[i].index = index;
            StateTracker::Texture& tx = contexts[context_index].textures.at(index);
            rp.attachments[i].format = tx.internal_format;
            rp.attachments[i].type = tx.binding_point;
            rp.width = tx.width;
            rp.height = tx.height;
            rp.depth = tx.depth;
        }
        else if (surface_index != UNBOUND) // backbuffer
        {
            rp.width = surfaces.at(surface_index).width;
            rp.height = surfaces.at(surface_index).height;
            rp.depth = 1;
            rp.attachments[i].slot = GL_BACK;
            rp.attachments[i].type = rb.second.type;
            assert(rb.second.type != GL_NONE);
            // Convert from egl config to framebuffer format
            if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 8 && jsonConfig.depth == 24) rp.attachments[i].slot = GL_NONE; // remove
            else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 8) rp.attachments[i].format = GL_UNSIGNED_BYTE;
            else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 16) rp.attachments[i].format = GL_UNSIGNED_SHORT;
            else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 32) rp.attachments[i].format = GL_UNSIGNED_INT;
            else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 0) rp.attachments[i].slot = GL_NONE; // remove
            else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 8) rp.attachments[i].format = GL_UNSIGNED_BYTE;
            else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 24) { rp.attachments[i].format = GL_DEPTH24_STENCIL8; rp.attachments[i].type = GL_DEPTH_STENCIL; }
            else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 16) rp.attachments[i].format = GL_UNSIGNED_SHORT;
            else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 32) rp.attachments[i].format = GL_UNSIGNED_INT;
            else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 0) rp.attachments[i].slot = GL_NONE; // remove
            else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 5) rp.attachments[i].format = GL_RGB565;
            else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 8 && jsonConfig.alpha == 0) rp.attachments[i].format = GL_RGB8;
            else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 8 && jsonConfig.alpha == 8) rp.attachments[i].format = GL_RGBA8;
            else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 10 && jsonConfig.alpha == 2) rp.attachments[i].format = GL_RGB10_A2;
            else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 11) rp.attachments[i].format = GL_R11F_G11F_B10F;
        }
        i++;
    }
}

// This is a hack, but there is no means within the GLES XML to reliably map enums to extensions.
void ParseInterfaceBase::check_enum(const std::string& callname, GLenum value)
{
    if (value == GL_BLEND_ADVANCED_COHERENT_KHR) used_extensions.insert("GL_KHR_blend_equation_advanced_coherent");
    else if (callname == "glBlendEquation" || callname == "glBlendEquationi")
    {
        switch (value)
        {
        case GL_MULTIPLY_KHR:
        case GL_SCREEN_KHR:
        case GL_OVERLAY_KHR:
        case GL_DARKEN_KHR:
        case GL_LIGHTEN_KHR:
        case GL_COLORDODGE_KHR:
        case GL_COLORBURN_KHR:
        case GL_HARDLIGHT_KHR:
        case GL_SOFTLIGHT_KHR:
        case GL_DIFFERENCE_KHR:
        case GL_EXCLUSION_KHR:
        case GL_HSL_HUE_KHR:
        case GL_HSL_SATURATION_KHR:
        case GL_HSL_COLOR_KHR:
        case GL_HSL_LUMINOSITY_KHR:
            used_extensions.insert("GL_KHR_blend_equation_advanced");
        default: break;
        }
    }
}

void ParseInterfaceBase::interpret_call(common::CallTM *call)
{
    // Check versions and extensions used
    if (map_func_to_version.count(call->mCallName))
    {
        int funcver = map_func_to_version.at(call->mCallName);
        if (funcver > highest_gles_version && highest_gles_version > 10)
        {
            DBG_LOG("The use of %s increases GLES version from %d to %d\n", call->mCallName.c_str(), (int)highest_gles_version, (int)funcver);
            highest_gles_version = funcver;
        }
    }
    if (map_func_to_extension.count(call->mCallName))
    {
        const auto its = map_func_to_extension.equal_range(call->mCallName);
        for (auto it = its.first; it != its.second; ++it)
        {
            used_extensions.insert(it->second);
        }
    }

    callstats[call->mCallName].count++;

    if (call->mCallName == "eglMakeCurrent") // find contexts that are used
    {
        int surface = call->mArgs[1]->GetAsInt();
        int readsurface = call->mArgs[2]->GetAsInt();
        int context = call->mArgs[3]->GetAsInt();

        if (surface != readsurface)
        {
            DBG_LOG("WARNING: Read surface != write surface not supported at call %u!\n", call->mCallNo);
            abort();
        }

        if (context == (int64_t)EGL_NO_CONTEXT)
        {
            current_context[call->mTid] = UNBOUND;
            context_index = UNBOUND;
        }
        else if (context_remapping.count(context) > 0)
        {
            context_index = context_remapping.at(context);
            current_context[call->mTid] = context_index;
            contexts[context_index].swaps.insert(frames);
        }

        if (surface == (int64_t)EGL_NO_SURFACE)
        {
            current_surface[call->mTid] = UNBOUND;
            surface_index = UNBOUND;
        }
        else if (surface_remapping.count(surface) > 0)
        {
            surface_index = surface_remapping.at(surface);
            current_surface[call->mTid] = surface_index;
            surfaces.at(surface_index).swaps.insert(frames);
        }

        // "The first time a OpenGL or OpenGL ES context is made current the viewport
        // and scissor dimensions are set to the size of the draw surface" (EGL standard 3.7.3)
        if (surface_index != UNBOUND && context_index != UNBOUND && contexts[context_index].fillstate.scissor.width == -1)
        {
            contexts[context_index].fillstate.scissor.width = surfaces.at(surface_index).width;
            contexts[context_index].fillstate.scissor.height = surfaces.at(surface_index).height;
            contexts[context_index].viewport.width = surfaces.at(surface_index).width;
            contexts[context_index].viewport.height = surfaces.at(surface_index).height;
        }
    }
    else if (call->mCallName == "eglCreateContext")
    {
        int mret = call->mRet.GetAsInt();
        int display = call->mArgs[0]->GetAsInt();
        int share = call->mArgs[2]->GetAsInt();
        context_remapping[mret] = contexts.size(); // generate id<->idx table
        if (share && context_remapping.count(share) > 0)
        {
            int share_idx = UNBOUND;
            int share_id = share;
            // Find root context, since sharing can be transitive
            do {
                share_idx = context_remapping.at(share_id);
                share_id = contexts.at(share_idx).share_context;
            } while (share_id != 0);
            contexts.emplace_back(mret, display, contexts.size(), share, &contexts.at(share_idx));
        } else {
            contexts.emplace_back(mret, display, contexts.size());
        }
    }
    else if (call->mCallName == "eglGetConfigAttrib")
    {
        int display = call->mArgs[0]->GetAsInt();
        int config = call->mArgs[1]->GetAsInt();
        int attribute = call->mArgs[2]->GetAsInt();
        if (call->mArgs[3]->mPointer)
        {
            int value = call->mArgs[3]->mPointer->GetAsInt();
            assert(attribute != EGL_SAMPLES || value >= eglconfigs[config].samples); // we assume we never reduce
            setEglConfig(eglconfigs[config], attribute, value);
        }
    }
    else if (call->mCallName == "eglChooseConfig")
    {
        StateTracker::EglConfig filter;
        int display = call->mArgs[0]->GetAsInt();
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i += 2)
        {
            const GLint key = call->mArgs[1]->mArray[i].GetAsUInt();
            if (key == EGL_NONE)
            {
                break;
            }
            const GLint value = call->mArgs[1]->mArray[i + 1].GetAsInt();
            setEglConfig(filter, key, value);
        }
        // Deducting what attributes an EGL config must at least have had to get to this point.
        for (unsigned i = 0; i < call->mArgs[2]->mArrayLen; i++)
        {
            const GLint config = call->mArgs[2]->mArray[i].GetAsUInt();
            eglconfigs[config].merge(filter);
        }
    }
    else if (call->mCallName == "eglCreateWindowSurface" || call->mCallName =="eglCreateWindowSurface2"
             || call->mCallName == "eglCreatePbufferSurface" || call->mCallName == "eglCreatePixmapSurface")
    {
        int mret = call->mRet.GetAsInt();
        int display = call->mArgs[0]->GetAsInt();
        int config = call->mArgs[1]->GetAsInt();
        int attrloc = 3;
        int width = 0;
        int height = 0;
        SurfaceType type = SURFACE_NATIVE;
        if (call->mCallName == "eglCreatePixmapSurface")
        {
            type = SURFACE_PIXMAP;
        }
        if (call->mCallName == "eglCreatePbufferSurface")
        {
            attrloc = 2;
            type = SURFACE_PBUFFER;
        }
        else // use size info stored in pat trace header
        {
            width = threadArray[defaultTid]["winW"].asInt();
            height = threadArray[defaultTid]["winH"].asInt();
        }
        std::map<GLenum, GLint> attribs;
        for (unsigned i = 0; i < call->mArgs[attrloc]->mArrayLen; i += 2)
        {
            const GLint key = call->mArgs[attrloc]->mArray[i].GetAsUInt();
            if (key == EGL_NONE)
            {
                break;
            }
            const GLint value = call->mArgs[attrloc]->mArray[i + 1].GetAsInt();
            attribs[key] = value;
            if (key == EGL_WIDTH)
            {
                width = value;
            }
            else if (key == EGL_HEIGHT)
            {
                height = value;
            }
        }
        surface_remapping[mret] = surfaces.size(); // generate id<->idx table
        surfaces.emplace_back(mret, display, surfaces.size(), type, attribs, width, height, config);
    }
    else if (call->mCallName == "eglDestroySurface")
    {
        // "surface is destroyed when it becomes not current to any thread"
        int surface = call->mArgs[1]->GetAsInt();
        int target_surface_index = surface_remapping.at(surface);
        surfaces[target_surface_index].destroyed = current_pos;
    }
    else if (call->mCallName == "eglDestroyContext")
    {
        // "context is destroyed when it becomes not current to any thread"
        int context = call->mArgs[1]->GetAsInt();
        // Fix weird error from ThunderAssault / Asphalt8 / etc, where they calls eglDestroyContext before any context is created
        if (context_remapping.count(context) > 0)
        {
            const int target_context_index = context_remapping.at(context);
            contexts[target_context_index].destroyed = current_pos;
        }
    }
    else if (call->mCallName.compare(0, 14, "eglSwapBuffers") == 0)
    {
        const int surface = call->mArgs[1]->GetAsInt();
        // check all resources for dependencies here, if they have any
        if (context_index != UNBOUND)
        {
            const auto dep_fb = contexts[context_index].framebuffers.most_recent_frame_dependency();
            const auto dep_tx = contexts[context_index].textures.most_recent_frame_dependency();
            const auto dep_rb = contexts[context_index].renderbuffers.most_recent_frame_dependency();
            const auto dep_sp = contexts[context_index].samplers.most_recent_frame_dependency();
            const auto dep_qr = contexts[context_index].queries.most_recent_frame_dependency();
            const auto dep_tf = contexts[context_index].transform_feedbacks.most_recent_frame_dependency();
            const auto dep_bf = contexts[context_index].buffers.most_recent_frame_dependency();
            const auto dep_pr = contexts[context_index].programs.most_recent_frame_dependency();
            const auto dep_sh = contexts[context_index].shaders.most_recent_frame_dependency();
            const auto dep_va = contexts[context_index].vaos.most_recent_frame_dependency();
            const auto dep_pp = contexts[context_index].program_pipelines.most_recent_frame_dependency();
            const auto dep = std::max({ dep_fb, dep_tx, dep_rb, dep_sp, dep_qr, dep_tf, dep_bf, dep_pr, dep_sh, dep_va, dep_pp });
            dependencies.push_back({ dep, dep_fb, dep_tx, dep_rb, dep_sp, dep_qr, dep_tf, dep_bf, dep_pr, dep_sh, dep_va, dep_pp });
        }
        // record which surface and context that used this frame
        if (surface_remapping.count(surface) > 0)
        {
            const int target_surface_index = surface_remapping.at(surface);
            if (surface_index != UNBOUND) surfaces[target_surface_index].swaps.insert(frames);
        }
        if (context_index != UNBOUND) contexts[context_index].swaps.insert(frames);
        if (!only_default || call->mTid == defaultTid) frames++;
        current_pos.frame = frames;
        if (context_index != UNBOUND)
        {
            contexts[context_index].renderpasses_per_frame[frames] = 0;
            new_renderpass(call, contexts[context_index], true);
            for (auto& pair : contexts[context_index].framebuffers[0].attachments)
            {
                pair.second.clears.clear(); // after swap you should probably repeat clears
            }
        }
    }
    /// --- end EGL ---
    else if (context_index == UNBOUND)
    {
        // nothing, just prevent the GLES calls below from being processed without a GLES context
    }
    /// --- start GLES ---
    else if (call->mCallName == "glViewport")
    {
        if (contexts[context_index].viewport.x == call->mArgs[0]->GetAsInt()
            && contexts[context_index].viewport.y == call->mArgs[1]->GetAsInt()
            && contexts[context_index].viewport.width == call->mArgs[2]->GetAsInt()
            && contexts[context_index].viewport.height == call->mArgs[3]->GetAsInt())
        {
            callstats[call->mCallName].dupes++;
        }
        else contexts[context_index].state_change(frames);
        contexts[context_index].viewport.x = call->mArgs[0]->GetAsInt();
        contexts[context_index].viewport.y = call->mArgs[1]->GetAsInt();
        contexts[context_index].viewport.width = call->mArgs[2]->GetAsInt();
        contexts[context_index].viewport.height = call->mArgs[3]->GetAsInt();
    }
    else if (call->mCallName == "glScissor")
    {
        if (contexts[context_index].fillstate.scissor.x == call->mArgs[0]->GetAsInt()
            && contexts[context_index].fillstate.scissor.y == call->mArgs[1]->GetAsInt()
            && contexts[context_index].fillstate.scissor.width == call->mArgs[2]->GetAsInt()
            && contexts[context_index].fillstate.scissor.height == call->mArgs[3]->GetAsInt())
        {
            callstats[call->mCallName].dupes++;
        }
        else contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.scissor.x = call->mArgs[0]->GetAsInt();
        contexts[context_index].fillstate.scissor.y = call->mArgs[1]->GetAsInt();
        contexts[context_index].fillstate.scissor.width = call->mArgs[2]->GetAsInt();
        contexts[context_index].fillstate.scissor.height = call->mArgs[3]->GetAsInt();
    }
    else if (call->mCallName == "glGenVertexArrays" || call->mCallName == "glGenVertexArraysOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].vaos.add(id);
        }
    }
    else if (call->mCallName == "glDeleteVertexArrays" || call->mCallName == "glDeleteVertexArraysOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const GLuint id = call->mArgs[1]->mArray[i].GetAsUInt();
            // "Unused names in framebuffers are silently ignored, as is the value zero."
            if (id != 0 && contexts[context_index].vaos.contains(id))
            {
                contexts[context_index].vaos.remove(id);
            }
            // "If a vertex array object that is currently bound is deleted, the binding for that object
            // reverts to zero and the default vertex array becomes current."
            if (contexts[context_index].vao_index != 0
                && contexts[context_index].vaos[contexts[context_index].vao_index].id == id)
            {
                contexts[context_index].vao_index = 0;
            }
        }
    }
    else if (call->mCallName == "glBindVertexArray" || call->mCallName == "glBindVertexArrayOES")
    {
        contexts[context_index].state_change(frames);
        const GLuint vao_id = call->mArgs[0]->GetAsUInt();
        if (vao_id != 0 && !contexts[context_index].vaos.contains(vao_id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].vaos.add(vao_id);
        }
        if (vao_id != 0)
        {
            contexts[context_index].vao_index = contexts[context_index].vaos.remap(vao_id);
            StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
            // Touch all bound buffers
            for (auto& target : vao.boundBufferIds)
            {
                for (auto& buffer : target.second)
                {
                    if (!contexts[context_index].buffers.contains(buffer.second.buffer)) continue; // weird Netflix bug
                    const int buffer_index = contexts[context_index].buffers.remap(buffer.second.buffer);
                    StateTracker::Buffer& buf = contexts[context_index].buffers.at(buffer_index);
                    (void)buf; // compiler: do not complain
                }
            }
        }
        else
        {
            contexts[context_index].vao_index = 0;
        }
    }
    else if (call->mCallName == "glIsEnabled")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        bool retval = call->mRet.GetAsInt();
        assert(contexts[context_index].enabled.count(target) == 0 || retval == contexts[context_index].enabled.at(target)); // sanity check
    }
    else if (call->mCallName == "glEnable")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        if (contexts[context_index].enabled.count(target) == 0 || !contexts[context_index].enabled.at(target))
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        contexts[context_index].enabled[target] = true;
    }
    else if (call->mCallName == "glDisable")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        if (contexts[context_index].enabled.count(target) == 0 || contexts[context_index].enabled.at(target))
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        contexts[context_index].enabled[target] = false;
    }
    else if (call->mCallName == "glEnableVertexAttribArray")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLuint index = call->mArgs[0]->GetAsUInt();
        if (vao.array_enabled.count(index) == 0) vao.array_enabled.insert(index);
        else callstats[call->mCallName].dupes++;
    }
    else if (call->mCallName == "glDisableVertexAttribArray")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLuint index = call->mArgs[0]->GetAsUInt();
        // Check if we are done with some old clientside state
        if (vao.boundVertexAttribs.count(index) && vao.array_enabled.count(index))
        {
            const int cs_id = std::get<5>(vao.boundVertexAttribs.at(index));
            if (cs_id != UNBOUND)
            {
                client_side_last_use[call->mTid][cs_id] = call->mCallNo;
                client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
            }
        }
        // Update state
        if (vao.array_enabled.count(index)) vao.array_enabled.erase(index);
        else callstats[call->mCallName].dupes++;
    }
    else if (call->mCallName == "glGenFramebuffers" || call->mCallName == "glGenFramebuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].framebuffers.add(id);
        }
    }
    else if (call->mCallName == "glDeleteFramebuffers" || call->mCallName == "glDeleteFramebuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();

            // "Unused names in framebuffers are silently ignored, as is the value zero."
            if (id != 0 && contexts[context_index].framebuffers.contains(id))
            {
                const int fb_index = contexts[context_index].framebuffers.remap(id);
                for (auto& pair : contexts[context_index].framebuffers[fb_index].attachments) // touch all
                {
                    if (pair.second.type == GL_RENDERBUFFER)
                    {
                        StateTracker::Renderbuffer& rb = contexts[context_index].renderbuffers.at(pair.second.index);
                    }
                    else // texture
                    {
                        StateTracker::Texture& tx = contexts[context_index].textures.at(pair.second.index);
                    }
                }
                contexts[context_index].framebuffers.remove(id);
            }
            // "If a framebuffer that is currently bound to one or more of the targets DRAW_FRAMEBUFFER
            // or READ_FRAMEBUFFER is deleted, it is as though BindFramebuffer had been executed with the
            // corresponding target and framebuffer zero"
            if (contexts[context_index].drawframebuffer == id)
            {
                contexts[context_index].prev_drawframebuffer = contexts[context_index].drawframebuffer;
                contexts[context_index].drawframebuffer = 0;
            }
            if (contexts[context_index].readframebuffer == id)
            {
                contexts[context_index].prev_readframebuffer = contexts[context_index].readframebuffer;
                contexts[context_index].readframebuffer = 0;
            }
        }
    }
    else if (call->mCallName == "glBindFramebuffer" || call->mCallName == "glBindFramebufferOES")
    {
        contexts[context_index].state_change(frames);
        GLenum target = call->mArgs[0]->GetAsUInt();
        GLuint fb = call->mArgs[1]->GetAsUInt();
        const bool readtarget = (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER);
        const bool writetarget = (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER);
        if (writetarget && contexts[context_index].drawframebuffer != 0 && fb != contexts[context_index].drawframebuffer)
        {
            const int fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
            StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
            unsigned i = 0;
            for (auto& rb : contexts[context_index].framebuffers[fb_index].attachments)
            {
                if (rp.attachments.size() > i && rp.attachments.at(i).store_op == StateTracker::RenderPass::STORE_OP_UNKNOWN && rb.second.invalidated)
                {
                    rp.attachments[i].store_op = StateTracker::RenderPass::STORE_OP_DONT_CARE;
                }
            }
        }
        if (writetarget && contexts[context_index].drawframebuffer != fb && contexts[context_index].render_passes.back().active)
        {
            new_renderpass(call, contexts[context_index], false);
        }
        if (fb != 0 && !contexts[context_index].framebuffers.contains(fb))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].framebuffers.add(fb);
        }
        if (writetarget)
        {
            contexts[context_index].prev_drawframebuffer = contexts[context_index].drawframebuffer;
            contexts[context_index].drawframebuffer = fb;
        }
        if (readtarget)
        {
            contexts[context_index].prev_readframebuffer = contexts[context_index].readframebuffer;
            contexts[context_index].readframebuffer = fb;
        }
        if (fb != 0)
        {
            const int index = contexts[context_index].framebuffers.remap(fb);
            for (auto& pair : contexts[context_index].framebuffers[index].attachments)
            {
                pair.second.clears.clear(); // we consider an FBO bind to be a valid point to repeat clears
            }
        }
    }
    else if (call->mCallName == "glFramebufferTexture2D" || call->mCallName == "glFramebufferTextureLayer"
             || call->mCallName == "glFramebufferTexture2DOES" || call->mCallName == "glFramebufferTextureLayerOES")
    {
        contexts[context_index].state_change(frames);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        int target_fb_index = 0;

        if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            target_fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
        }
        else if (target == GL_READ_FRAMEBUFFER)
        {
            target_fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].readframebuffer);
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Invalid target for %s!\n", (int)call->mCallNo, call->mCallName.c_str());
        }

        const GLenum attachment = call->mArgs[1]->GetAsUInt();
        GLenum textarget = 0;
        GLuint texture = 0;
        int texture_index = UNBOUND;

        if (call->mCallName == "glFramebufferTexture2D" || call->mCallName == "glFramebufferTexture2DOES")
        {
            textarget = call->mArgs[2]->GetAsUInt();
            texture = call->mArgs[3]->GetAsUInt();
            if (texture != 0)
            {
                texture_index = contexts[context_index].textures.remap(texture);
                StateTracker::Texture& tx = contexts[context_index].textures.at(texture_index);
                tx.used = true; // ideally not needed here since we check on draw, but this makes unused texture detection for GFXB5 work
                                // frame range check is skipped here in case the framebuffer attachment is possibly used later
            }
        }
        else
        {
            texture = call->mArgs[2]->GetAsUInt();
            if (texture != 0)
            {
                texture_index = contexts[context_index].textures.remap(texture);
                StateTracker::Texture& tx = contexts[context_index].textures.at(texture_index);
                tx.used = true; // as above
                textarget = tx.binding_point;
            }
        }

        if ((target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER) && contexts[context_index].render_passes.back().active)
        {
            new_renderpass(call, contexts[context_index], false);
        }

        if (texture != 0)
        {
            if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(GL_STENCIL_ATTACHMENT, StateTracker::Attachment(texture, textarget, texture_index));
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(GL_DEPTH_ATTACHMENT, StateTracker::Attachment(texture, textarget, texture_index));
            }
            else
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(attachment, StateTracker::Attachment(texture, textarget, texture_index));
            }
        }
        else // remove it
        {
            if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(GL_STENCIL_ATTACHMENT);
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(GL_DEPTH_ATTACHMENT);
            }
            else
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(attachment);
            }
        }
        contexts[context_index].framebuffers[target_fb_index].attachment_calls++;
    }
    else if (call->mCallName == "glGenerateMipmap" || call->mCallName == "glGenerateMipmapOES")
    {
        contexts[context_index].state_change(frames);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        if (tex_id != 0)
        {
            const int target_texture_index = contexts[context_index].textures.remap(tex_id);
            StateTracker::Texture& tx = contexts[context_index].textures.at(target_texture_index);
            tx.mipmaps[call->mCallNo] = { frames, false };
        }
    }
    else if (call->mCallName == "glGenRenderbuffers" || call->mCallName == "glGenRenderbuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].renderbuffers.add(id);
        }
    }
    else if (call->mCallName == "glDeleteRenderbuffers" || call->mCallName == "glDeleteRenderbuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].renderbuffers.contains(id))
            {
                contexts[context_index].renderbuffers.remove(id);
                // "If a renderbuffer object is attached to one or more attachment points in the currently
                // bound framebuffer, then it as if glFramebufferRenderbuffer had been called, with a renderbuffer
                // of zero for each attachment point to which this image was attached in the currently bound framebuffer."
                unbind_renderbuffers_if(contexts[context_index], contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer), true, id);
                unbind_renderbuffers_if(contexts[context_index], contexts[context_index].framebuffers.remap(contexts[context_index].readframebuffer), true, id);
            }
        }
    }
    else if (call->mCallName == "glFramebufferRenderbuffer" || call->mCallName == "glFramebufferRenderbufferOES")
    {
        contexts[context_index].state_change(frames);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        int target_fb_index = 0;
        if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            target_fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
        }
        else if (target == GL_READ_FRAMEBUFFER)
        {
            target_fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].readframebuffer);
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Invalid target for %s!\n", (int)call->mCallNo, call->mCallName.c_str());
        }
        const GLenum attachment = call->mArgs[1]->GetAsUInt();
        const GLenum renderbuffertarget = call->mArgs[2]->GetAsUInt();
        assert(renderbuffertarget == GL_RENDERBUFFER);
        const GLuint renderbuffer = call->mArgs[3]->GetAsUInt();

        if (contexts[context_index].render_passes.back().active)
        {
            new_renderpass(call, contexts[context_index], false);
        }

        if (renderbuffer != 0)
        {
            const int rb_index = contexts[context_index].renderbuffers.remap(renderbuffer);
            if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(GL_STENCIL_ATTACHMENT, StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index));
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(GL_DEPTH_ATTACHMENT, StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index));
            }
            else
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.emplace(attachment, StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index));
            }
        }
        else // remove it
        {
            if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(GL_STENCIL_ATTACHMENT);
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(GL_DEPTH_ATTACHMENT);
            }
            else
            {
                contexts[context_index].framebuffers[target_fb_index].attachments.erase(attachment);
            }
        }
        contexts[context_index].framebuffers[target_fb_index].attachment_calls++;
    }
    else if (call->mCallName == "glBindRenderbuffer" || call->mCallName == "glBindRenderbufferOES")
    {
        contexts[context_index].state_change(frames);
        GLenum target = call->mArgs[0]->GetAsUInt();
        assert(target == GL_RENDERBUFFER);
        GLuint rb = call->mArgs[1]->GetAsUInt();
        if (rb != 0 && !contexts[context_index].renderbuffers.contains(rb))
        {
            // It is legal to create renderbuffers with a call to this function.
            contexts[context_index].renderbuffers.add(rb);
        }
        if (rb != 0)
        {
            contexts[context_index].renderbuffer_index = contexts[context_index].renderbuffers.remap(rb);
        }
        else
        {
            contexts[context_index].renderbuffer_index = UNBOUND;
        }
    }
    else if (call->mCallName == "glRenderbufferStorage" || call->mCallName == "glRenderbufferStorageOES")
    {
        contexts[context_index].state_change(frames);
        GLenum target = call->mArgs[0]->GetAsUInt();
        assert(target == GL_RENDERBUFFER);
        const int renderbuffer_index = contexts[context_index].renderbuffer_index;
        if (renderbuffer_index != UNBOUND)
        {
            contexts[context_index].renderbuffers[renderbuffer_index].internalformat = call->mArgs[1]->GetAsUInt();
            contexts[context_index].renderbuffers[renderbuffer_index].width = call->mArgs[2]->GetAsInt();
            contexts[context_index].renderbuffers[renderbuffer_index].height = call->mArgs[3]->GetAsInt();
        }
        else
        {
            DBG_LOG("API ERROR [%d]: %s attempts to operate on an unbound renderbuffer\n", (int)call->mCallNo, call->mCallName.c_str());
        }
    }
    else if (call->mCallName == "glRenderbufferStorageMultisample" || call->mCallName == "glRenderbufferStorageMultisampleEXT")
    {
        contexts[context_index].state_change(frames);
        GLenum target = call->mArgs[0]->GetAsUInt();
        assert(target == GL_RENDERBUFFER);
        const int renderbuffer_index = contexts[context_index].renderbuffer_index;
        if (renderbuffer_index != UNBOUND)
        {
            contexts[context_index].renderbuffers[renderbuffer_index].samples = call->mArgs[1]->GetAsInt();
            contexts[context_index].renderbuffers[renderbuffer_index].internalformat = call->mArgs[2]->GetAsUInt();
            contexts[context_index].renderbuffers[renderbuffer_index].width = call->mArgs[3]->GetAsInt();
            contexts[context_index].renderbuffers[renderbuffer_index].height = call->mArgs[4]->GetAsInt();
        }
        else
        {
            DBG_LOG("API ERROR [%d]: %s attempts to operate on an unbound renderbuffer\n", (int)call->mCallNo, call->mCallName.c_str());
        }
    }
    else if (call->mCallName == "glGenSamplers")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].samplers.add(id);
        }
    }
    else if (call->mCallName == "glDeleteSamplers")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].samplers.contains(id) > 0)
            {
                contexts[context_index].samplers.remove(id);
            }
        }
    }
    else if (call->mCallName == "glBindSampler")
    {
        // The usual rule that you can create objects with glBind*() calls apparently does not apply to to glBindSampler()
        const GLuint unit = call->mArgs[0]->GetAsUInt();
        const GLuint sampler = call->mArgs[1]->GetAsUInt();
        if (contexts[context_index].sampler_binding[unit] != sampler)
        {
            contexts[context_index].sampler_binding[unit] = sampler;
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
    }
    else if (call->mCallName == "glGenQueries" || call->mCallName == "glGenQueriesEXT")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].queries.add(id);
        }
    }
    else if (call->mCallName == "glDeleteQueries" || call->mCallName == "glDeleteQueriesEXT")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].queries.contains(id))
            {
                contexts[context_index].queries.remove(id);
            }
        }
    }
    else if (call->mCallName == "glBeginQuery" || call->mCallName == "glBeginQueryEXT")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = call->mArgs[1]->GetAsUInt();
        assert(id != 0);
        contexts[context_index].query_binding[target] = id;
        const int query_index = contexts[context_index].queries.remap(id);
        contexts[context_index].queries[query_index].target = target;
    }
    else if (call->mCallName == "glEndQuery" || call->mCallName == "glEndQueryEXT")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        contexts[context_index].query_binding[target] = UNBOUND;
    }
    else if (call->mCallName == "glGenTransformFeedbacks")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].transform_feedbacks.add(id);
        }
    }
    else if (call->mCallName == "glDeleteTransformFeedbacks")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].transform_feedbacks.contains(id))
            {
                contexts[context_index].transform_feedbacks.remove(id);
            }
        }
    }
    else if (call->mCallName == "glBindTransformFeedback")
    {
        contexts[context_index].state_change(frames);
        assert(call->mArgs[0]->GetAsUInt() == GL_TRANSFORM_FEEDBACK);
        GLuint id = call->mArgs[1]->GetAsUInt();
        if (id != 0 && !contexts[context_index].transform_feedbacks.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].transform_feedbacks.add(id);
        }
        contexts[context_index].transform_feedback_binding = id;
    }
    else if (call->mCallName == "glBeginTransformFeedback")
    {
        contexts[context_index].state_change(frames);
        const GLuint id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].primitiveMode = call->mArgs[0]->GetAsUInt();
        contexts[context_index].transform_feedbacks[index].active = true;
    }
    else if (call->mCallName == "glPauseTransformFeedback")
    {
        contexts[context_index].state_change(frames);
        const GLuint id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].active = false;
    }
    else if (call->mCallName == "glResumeTransformFeedback")
    {
        contexts[context_index].state_change(frames);
        const GLuint id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].active = true;
    }
    else if (call->mCallName == "glEndTransformFeedback")
    {
        const int id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].primitiveMode = GL_NONE;
        contexts[context_index].transform_feedbacks[index].active = false;
    }
    else if (call->mCallName == "glGenBuffers")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].buffers.add(id);
        }
    }
    else if (call->mCallName == "glDeleteBuffers")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].buffers.contains(id))
            {
                contexts[context_index].buffers.remove(id);
            }
        }
        // TBD: "When a buffer, texture, or renderbuffer object is deleted, it is unbound from any
        // bind points it is bound to in the current context, and detached from any attachments
        // of container objects that are bound to the current context"
    }
    else if (call->mCallName == "glBufferData")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        contexts[context_index].state_change(frames);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = vao.boundBufferIds[target][0].buffer;
        if (id != 0) // issue seen in BlackDesert
        {
            const int index = contexts[context_index].buffers.remap(id);
            const GLsizeiptr size = call->mArgs[1]->GetAsUInt();
            StateTracker::Buffer &buffer = contexts[context_index].buffers[index];
            buffer.usages.insert(call->mArgs[3]->GetAsUInt());
            buffer.size = size;
            buffer.updated();
        }
    }
    else if (call->mCallName == "glBufferSubData")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLuint id = vao.boundBufferIds[target][0].buffer;
        if (id != 0)
        {
            const int index = contexts[context_index].buffers.remap(id);
            contexts[context_index].buffers.at(index).partial_update();
        }
        contexts[context_index].state_change(frames);
    }
    else if (call->mCallName == "glMapBufferRange" || call->mCallName == "glMapBufferOES" || call->mCallName == "glMapBuffer")
    {
        contexts[context_index].state_change(frames);
        // TBD - check flags, set the appropriate update status
    }
    else if (call->mCallName == "glCopyClientSideBuffer")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint cs_id = call->mArgs[1]->GetAsUInt();
        const GLuint buffer_id = vao.boundBufferIds[target][0].buffer;
        assert(contexts[context_index].buffers.contains(buffer_id));
        const int buffer_index = contexts[context_index].buffers.remap(buffer_id);
        contexts[context_index].buffers[buffer_index].clientsidebuffer = cs_id;
        client_side_last_use[call->mTid][cs_id] = call->mCallNo;
        client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
    }
    else if (call->mCallName == "glClientSideBufferData")
    {
        const GLuint cs_id = call->mArgs[0]->GetAsUInt();
        const GLsizei size = call->mArgs[1]->GetAsUInt();
        client_side_last_use[call->mTid][cs_id] = call->mCallNo;
        client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
    }
    else if (call->mCallName == "glClientSideBufferSubData")
    {
        const GLuint cs_id = call->mArgs[0]->GetAsUInt();
        const GLsizei offset = call->mArgs[1]->GetAsUInt();
        const GLsizei size = call->mArgs[2]->GetAsUInt();
        client_side_last_use[call->mTid][cs_id] = call->mCallNo;
        client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
    }
    else if (call->mCallName == "glPatchClientSideBuffer")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLsizei size = call->mArgs[1]->GetAsUInt();
        const GLuint buffer_id = vao.boundBufferIds[target][0].buffer;
        const int buffer_index = contexts[context_index].buffers.remap(buffer_id);
        const GLuint cs_id = contexts[context_index].buffers[buffer_index].clientsidebuffer;
        client_side_last_use[call->mTid][cs_id] = call->mCallNo;
        client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
    }
    else if (call->mCallName == "glDeleteClientSideBuffer")
    {
        const GLuint cs_id = call->mArgs[0]->GetAsUInt();
        client_side_last_use[call->mTid].erase(cs_id);
    }
    else if (call->mCallName == "glBindBuffer")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = call->mArgs[1]->GetAsUInt();
        if (vao.boundBufferIds.count(target) == 0
            || vao.boundBufferIds[target].count(0) == 0
            || vao.boundBufferIds[target][0].buffer != id)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id);
        }
        if (id != 0)
        {
            vao.boundBufferIds[target][0] = { id, 0, 0 };
            const int buffer_index = contexts[context_index].buffers.remap(id);
            contexts[context_index].buffers[buffer_index].bindings.insert(target);
        }
        else
        {
            vao.boundBufferIds.erase(target);
        }
    }
    else if (call->mCallName == "glBindBufferBase")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint index = call->mArgs[1]->GetAsUInt();
        const GLuint id = call->mArgs[2]->GetAsUInt();
        if (vao.boundBufferIds.count(target) == 0
            || vao.boundBufferIds[target].count(index) == 0
            || vao.boundBufferIds[target][index].buffer != id)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id);
        }
        if (id != 0)
        {
            vao.boundBufferIds[target][index] = { id, 0, 0 };
            const int buffer_index = contexts[context_index].buffers.remap(id);
            contexts[context_index].buffers[buffer_index].bindings.insert(target);
        }
        else
        {
            vao.boundBufferIds.erase(target);
        }
    }
    else if (call->mCallName == "glBindBufferRange")
    {
        // TBD: GL_ARRAY_BUFFER is *not* part of VAO state! fix this later... not much content uses VAOs
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const int vao_index = contexts[context_index].vao_index;
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(vao_index);
        const GLuint index = call->mArgs[1]->GetAsUInt();
        const GLuint id = call->mArgs[2]->GetAsUInt();
        const GLintptr offset = call->mArgs[3]->GetAsUInt();
        const GLsizeiptr size = call->mArgs[4]->GetAsUInt();
        if (vao.boundBufferIds.count(target) == 0
            || vao.boundBufferIds[target].count(index) == 0
            || vao.boundBufferIds[target][index].buffer != id
            || vao.boundBufferIds[target][index].offset != offset
            || vao.boundBufferIds[target][index].size != size)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id);
        }
        if (id != 0)
        {
            const int buffer_index = contexts[context_index].buffers.remap(id);
            contexts[context_index].buffers[buffer_index].bindings.insert(target);
            vao.boundBufferIds[target][index] = { id, offset, size };
        }
        else
        {
            vao.boundBufferIds.erase(target);
        }
    }
    else if (call->mCallName == "glVertexAttribPointer" || call->mCallName == "glVertexAttribIPointer")
    {
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        const int ptr_idx = call->mCallName == "glVertexAttribIPointer" ? 4 : 5;
        const GLuint index = call->mArgs[0]->GetAsUInt();
        const GLuint buffer_id = vao.boundBufferIds[GL_ARRAY_BUFFER][0].buffer;
        const bool gpubuffer = (buffer_id != 0);
        const bool clientsidebuffer = (call->mArgs[ptr_idx]->mOpaqueType == common::ClientSideBufferObjectReferenceType);
        const GLenum type = call->mArgs[2]->GetAsUInt();
        const GLint size = call->mArgs[1]->GetAsInt();
        const int stride_idx = (call->mCallName == "glVertexAttribPointer") ? 4 : 3;
        const GLsizei stride = call->mArgs[stride_idx]->GetAsInt();
        const uint64_t offset = clientsidebuffer ? call->mArgs[ptr_idx]->mOpaqueIns->mClientSideBufferOffset : (gpubuffer ? call->mArgs[ptr_idx]->GetAsUInt64() : 0);
        const int cs_id = clientsidebuffer ? call->mArgs[ptr_idx]->mOpaqueIns->mClientSideBufferName : UNBOUND;
        const auto tuple = std::make_tuple(type, size, stride, offset, buffer_id, cs_id);

        assert(!(clientsidebuffer && gpubuffer)); // cannot be both at the same time

        if (vao.boundVertexAttribs.count(index) == 0 || vao.boundVertexAttribs.at(index) != tuple)
        {
            // Check if we are overwriting some old clientside state
            if (vao.boundVertexAttribs.count(index) != 0)
            {
                const int old_cs_id = std::get<5>(vao.boundVertexAttribs.at(index));
                if (old_cs_id != UNBOUND)
                {
                    client_side_last_use[call->mTid][old_cs_id] = call->mCallNo;
                    client_side_last_use_reason[call->mTid][old_cs_id] = call->mCallName + " (overwrite)";
                }
            }
            // Update current state
            contexts[context_index].state_change(frames);
            vao.boundVertexAttribs[index] = tuple;
        }
        //else callstats[call->mCallName].dupes++;
        if (buffer_id != 0 && contexts[context_index].buffers.contains(buffer_id))
        {
            const int buffer_index = contexts[context_index].buffers.remap(buffer_id);
            contexts[context_index].buffers[buffer_index].types.insert(std::make_tuple(type, size, stride)); // this is for reverse-engineering later how it was used
            contexts[context_index].buffers[buffer_index].clientsidebuffer = buffer_id;
        }
        if (clientsidebuffer)
        {
            client_side_last_use[call->mTid][cs_id] = call->mCallNo;
            client_side_last_use_reason[call->mTid][cs_id] = call->mCallName;
        }
    }
    else if (call->mCallName == "glGenTextures")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].textures.add(id);
        }
    }
    else if (call->mCallName == "glDeleteTextures")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            if (id != 0 && contexts[context_index].textures.contains(id))
            {
                contexts[context_index].textures.remove(id);
                // "When a buffer, texture, or renderbuffer object is deleted, it is unbound from any
                // bind points it is bound to in the current context, and detached from any attachments
                // of container objects that are bound to the current context"
                for (auto& unit : contexts[context_index].textureUnits)
                {
                    for (auto& binding : unit.second)
                    {
                        if (binding.second == id)
                        {
                            binding.second = 0;
                        }
                    }
                }
                unbind_renderbuffers_if(contexts[context_index],
                                        contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer), false, id);
                unbind_renderbuffers_if(contexts[context_index],
                                        contexts[context_index].framebuffers.remap(contexts[context_index].readframebuffer), false, id);
            }
        }
    }
    else if (call->mCallName == "glTexStorage3D" || call->mCallName == "glTexStorage2D" || call->mCallName == "glTexStorage1D"
             || call->mCallName == "glTexStorage3DEXT" || call->mCallName == "glTexStorage2DEXT" || call->mCallName == "glTexStorage1DEXT"
             || call->mCallName == "glTexImage3D" || call->mCallName == "glTexImage2D" || call->mCallName == "glTexImage1D" || call->mCallName == "glTexImage3DOES"
             || call->mCallName == "glCompressedTexImage3D" || call->mCallName == "glCompressedTexImage2D" || call->mCallName == "glCompressedTexImage1DOES"
             || call->mCallName == "glCompressedTexImage1D" || call->mCallName == "glCompressedTexImage3DOES" || call->mCallName == "glCompressedTexImage2DOES")
    {
        const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        contexts[context_index].state_change(frames);
        if (contexts[context_index].textures.contains(tex_id) && tex_id != 0)
        {
            const int target_texture_index = contexts[context_index].textures.remap(tex_id);
            StateTracker::Texture& tex = contexts[context_index].textures[target_texture_index];
            if (call->mCallName.find("glTexImage") != std::string::npos || call->mCallName.find("glCompressedTexImage") != std::string::npos)
            {
                tex.initialized = true;
                int valno = -1;
                if (call->mCallName == "glTexImage1D") valno = 7;
                else if (call->mCallName == "glTexImage2D") valno = 8;
                else if (call->mCallName == "glTexImage3D") valno = 9;
                else if (call->mCallName == "glTexImage3DOES") valno = 9;
                else if (call->mCallName == "glCompressedTexImage1D") valno = 6;
                else if (call->mCallName == "glCompressedTexImage2D") valno = 7;
                else if (call->mCallName == "glCompressedTexImage3D") valno = 8;
                else if (call->mCallName == "glCompressedTexImage1DOES") valno = 6;
                else if (call->mCallName == "glCompressedTexImage2DOES") valno = 7;
                else if (call->mCallName == "glCompressedTexImage3DOES") valno = 8;
                assert(valno != -1);

                if (valno > 0)
                {
                    assert(call->mArgs[valno]->mType == common::Opaque_Type);
                    // very old trace files will have call->mArgs[valno]->mOpaqueIns->mType == common::Uint_Type, not sure how to handle here
                    if (call->mArgs[valno]->mOpaqueIns->mType == common::Blob_Type && call->mArgs[valno]->mOpaqueIns->mBlobLen == 0)
                    {
                        tex.initialized = false;
                    }
                }
            }
            tex.binding_point = target;
            if (call->mCallName.find("TexStorage") != std::string::npos)
            {
                tex.levels = call->mArgs[1]->GetAsUInt();
                tex.immutable = true;
            }
            else
            {
                tex.levels = std::max<int>(tex.levels, call->mArgs[1]->GetAsUInt());
            }
            tex.internal_format = call->mArgs[2]->GetAsUInt();
            tex.width = call->mArgs[3]->GetAsInt();
            if (call->mCallName.find("2D") != std::string::npos)
            {
                tex.height = call->mArgs[4]->GetAsInt();
            }
            else if (call->mCallName.find("3D") != std::string::npos)
            {
                tex.height = call->mArgs[4]->GetAsInt();
                tex.depth = call->mArgs[5]->GetAsInt();
            }
            tex.updated();
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Cannot find texture id %u for %s on context %d\n", (int)call->mCallNo, tex_id, call->mCallName.c_str(), context_index);
        }
    }
    else if (call->mCallName == "glTexSubImage1D" || call->mCallName == "glTexSubImage2D" || call->mCallName == "glTexSubImage3D"
             || call->mCallName == "glCompressedTexSubImage2D" || call->mCallName == "glCompressedTexSubImage3D")
    {
        const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        contexts[context_index].state_change(frames);
        if (contexts[context_index].textures.contains(tex_id) && tex_id != 0)
        {
            const int target_texture_index = contexts[context_index].textures.remap(tex_id);
            StateTracker::Texture& tex = contexts[context_index].textures[target_texture_index];
            tex.partial_update();
            tex.initialized = true;
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Cannot find texture id %u for %s on context %d\n", (int)call->mCallNo, tex_id, call->mCallName.c_str(), context_index);
        }
    }
    else if (call->mCallName == "glTexStorage2DMultisample")
    {
        contexts[context_index].state_change(frames);
        const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt()); // must be GL_TEXTURE_2D_MULTISAMPLE
        assert(target == GL_TEXTURE_2D_MULTISAMPLE);
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        const int target_texture_index = contexts[context_index].textures.remap(tex_id);
        StateTracker::Texture& tex = contexts[context_index].textures[target_texture_index];
        assert(!tex.immutable);
        tex.immutable = true;
        tex.internal_format = call->mArgs[2]->GetAsUInt();
        tex.width = call->mArgs[3]->GetAsInt();
        tex.height = call->mArgs[4]->GetAsInt();
        tex.levels = 0;
        tex.updated();
    }
    else if (call->mCallName == "glBindImageTexture")
    {
        const GLuint unit = call->mArgs[0]->GetAsUInt();
        const GLuint texid = call->mArgs[1]->GetAsUInt();
        const GLuint access = call->mArgs[5]->GetAsUInt();
        if (texid != 0)
        {
            const int target_texture_index = contexts[context_index].textures.remap(texid); // touch
        }
        contexts[context_index].image_binding[unit] = texid;
    }
    else if (call->mCallName == "glTexBufferEXT")
    {
        const unsigned unit = contexts[context_index].activeTextureUnit;
        const GLuint target = call->mArgs[0]->GetAsUInt();
        const GLuint buf_id = call->mArgs[2]->GetAsUInt();
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        if (tex_id != 0 && buf_id != 0)
        {
            const int index = contexts[context_index].textures.remap(tex_id);
            StateTracker::Texture& tx = contexts[context_index].textures.at(index);
            tx.initialized = true; // assume it is initialized here
        }
    }
    else if (call->mCallName == "glBindTexture")
    {
        const unsigned unit = contexts[context_index].activeTextureUnit;
        const unsigned target = call->mArgs[0]->GetAsUInt();
        const unsigned tex_id = call->mArgs[1]->GetAsUInt();
        if (!contexts[context_index].textures.contains(tex_id)
            || contexts[context_index].textureUnits.count(unit) == 0
            || contexts[context_index].textureUnits[unit].count(target) == 0
            || contexts[context_index].textureUnits[unit][target] != tex_id)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        contexts[context_index].textureUnits[unit][target] = tex_id;
        if (tex_id != 0 && !contexts[context_index].textures.contains(tex_id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].textures.add(tex_id);
        }
        else if (tex_id != 0)
        {
            const int index = contexts[context_index].textures.remap(tex_id);
            (void)contexts[context_index].textures.at(index); // just touch it
        }
    }
    else if (call->mCallName == "eglDestroyImageKHR" || call->mCallName == "eglDestroyImage")
    {
        const unsigned image_id = call->mArgs[1]->GetAsUInt();
        int ctxidx = context_index;
        if (!contexts[ctxidx].textures.contains(image_id)) ctxidx = 0; // our default storage location for external images
        if (contexts[ctxidx].textures.contains(image_id))
        {
            const int image_index = contexts[ctxidx].images.remap(image_id);
            StateTracker::Image& image = contexts[ctxidx].images.at(image_index);
            if (image.type == EGL_GL_TEXTURE_2D_KHR && image.value != 0)
            {
                const int index = contexts[ctxidx].textures.remap(image.value);
                (void)contexts[ctxidx].textures.at(index); // just touch it
            }
            contexts[ctxidx].images.remove(image_id);
        }
    }
    else if (call->mCallName == "eglCreateImageKHR" || call->mCallName == "eglCreateImage")
    {
        const unsigned our_context_id = call->mArgs[1]->GetAsUInt();
        const int our_context_index = (our_context_id == (int64_t)EGL_NO_CONTEXT) ? 0 : context_remapping.at(our_context_id);
        const unsigned target = call->mArgs[2]->GetAsUInt();
        const unsigned value = call->mArgs[3]->GetAsUInt();
        const unsigned image_id = call->mRet.GetAsUInt();
        if (target == EGL_GL_TEXTURE_2D_KHR && value != 0)
        {
            const int index = contexts[our_context_index].textures.remap(value);
            (void)contexts[our_context_index].textures.at(index); // just touch it
        }
        if (value != 0)
        {
            StateTracker::Image& image = contexts[our_context_index].images.add(image_id);
            image.type = target;
            image.value = value;
        }
    }
    else if (call->mCallName == "glEGLImageTargetTexture2DOES")
    {
        const unsigned target = call->mArgs[0]->GetAsUInt();
        const unsigned image_id = call->mArgs[1]->GetAsUInt();
        if (target == GL_TEXTURE_EXTERNAL_OES && contexts[context_index].images.contains(image_id))
        {
            const StateTracker::Image& image = contexts[context_index].images.id(image_id);
            assert(image.type != EGL_GL_RENDERBUFFER_KHR); // matches EGL_GL_TEXTURE*
            const unsigned texture_id = image.value;
            StateTracker::Texture& tx = contexts[context_index].textures.id(texture_id);
            if (texture_id != 0)
            {
                tx.used = true; // Textures create by glEGLImageTargetTexture2DOES have dependencies with the source textures of eglImage,
                                // so even the source textures not in target frame range should be marked as used.
            }
        }
    }
    else if (call->mCallName == "glActiveTexture")
    {
        const GLuint unit = call->mArgs[0]->GetAsUInt() - GL_TEXTURE0;
        if (contexts[context_index].activeTextureUnit != unit) contexts[context_index].activeTextureUnit = unit;
        else callstats[call->mCallName].dupes++;
    }
    else if (call->mCallName == "glTexParameteri" || call->mCallName == "glTexParameterf" || call->mCallName == "glTexParameteriv" || call->mCallName == "glTexParameterfv")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLenum pname = call->mArgs[1]->GetAsUInt();
        const GLuint texture_id = contexts[context_index].textureUnits[contexts[context_index].activeTextureUnit][target];
        if (texture_id == 0)
        {
            return;
        }
        contexts[context_index].state_change(frames);
        const int texture_index = contexts[context_index].textures.remap(texture_id);
        adjust_sampler_state(call, pname, contexts[context_index].textures[texture_index].state, call->mArgs[2]);
        contexts[context_index].textures.at(texture_index).updated();
    }
    else if (call->mCallName == "glSamplerParameterf" || call->mCallName == "glSamplerParameteri" || call->mCallName == "glSamplerParameterfv" || call->mCallName == "glSamplerParameteriv")
    {
        const GLenum sampler = call->mArgs[0]->GetAsUInt();
        if (!contexts[context_index].samplers.contains(sampler))
        {
            DBG_LOG("API ERROR [%d]: Invalid sampler id=%u\n", (int)call->mCallNo, sampler);
            return;
        }
        contexts[context_index].state_change(frames);
        const GLenum pname = call->mArgs[1]->GetAsUInt();
        const GLuint sampler_idx = contexts[context_index].samplers.remap(sampler);
        adjust_sampler_state(call, pname, contexts[context_index].samplers[sampler_idx].state, call->mArgs[2]);
        contexts[context_index].samplers.at(sampler_idx).updated();
    }
    else if (call->mCallName == "glCreateShaderProgramv")
    {
        GLuint id = call->mRet.GetAsUInt();
        GLenum type = call->mArgs[0]->GetAsUInt();
        GLsizei count = call->mArgs[1]->GetAsInt();
        assert((int)call->mArgs[2]->mArrayLen == count);
        for (unsigned i = 0; i < call->mArgs[2]->mArrayLen; i++)
        {
            // TBD
        }
        assert(false); // not yet supported
    }
    else if (call->mCallName == "glCreateProgram")
    {
        GLuint id = call->mRet.GetAsUInt();
        contexts[context_index].programs.add(id);
    }
    else if (call->mCallName == "glUseProgram")
    {
        GLuint id = call->mArgs[0]->GetAsUInt();
        if (id != 0)
        {
            if (!contexts[context_index].programs.contains(id)) // sanity check
            {
                DBG_LOG("API ERROR [%d]: Calling glUseProgram(%u) on context %d but no such program exists!\n", (int)call->mCallNo, id, context_index);
                contexts[context_index].program_index = UNBOUND; // to prevent crashes later
                if (mDebug) for (const auto& ctx : contexts) if (ctx.programs.contains(id)) DBG_LOG("\tbut program %u exists in context %d!\n", id, ctx.index);
            }
            else
            {
                const int program_index = contexts[context_index].programs.remap(id);
                if (contexts[context_index].program_index != program_index)
                {
                    contexts[context_index].state_change(frames);
                    contexts[context_index].program_index = program_index;
                }
                else callstats[call->mCallName].dupes++;
            }
        }
        else if (contexts[context_index].program_index != UNBOUND)
        {
            contexts[context_index].program_index = UNBOUND;
        }
        else callstats[call->mCallName].dupes++;
    }
    else if (call->mCallName == "glDeleteProgram")
    {
        GLuint id = call->mArgs[0]->GetAsUInt();
        // "DeleteProgram will silently ignore the value zero". Also, some content assumes it will also
        // ignore invalid values, even though the standard does not guarantee this.
        if (id != 0)
        {
            contexts[context_index].programs.remove(id);
        }
    }
    else if (call->mCallName == "glAttachShader")
    {
        GLuint program = call->mArgs[0]->GetAsUInt();
        GLuint shader = call->mArgs[1]->GetAsUInt();
        if (contexts[context_index].shaders.contains(shader) && contexts[context_index].programs.contains(program))
        {
            const int target_program_index = contexts[context_index].programs.remap(program);
            const int target_shader_index = contexts[context_index].shaders.remap(shader);
            if (contexts[context_index].shaders.ssize() > target_shader_index) // this check is for Egypt...
            {
                const GLenum type = contexts[context_index].shaders[target_shader_index].shader_type;
                contexts[context_index].programs[target_program_index].shaders[type] = target_shader_index;
            }
            else // egypt again
            {
                DBG_LOG("API ERROR [%d]: No shader ID %u (idx %d) to attach to program ID %u, context %d (%u shaders)\n",
                        (int)call->mCallNo, shader, target_shader_index, program, context_index, (unsigned)contexts[context_index].shaders.size());
            }
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Failed to attach shader ID %u to program ID %u, context %d (program or shader not found by IDs)\n",
                    (int)call->mCallNo, shader, program, context_index);
        }
    }
    else if (call->mCallName == "glCreateShader")
    {
        GLuint id = call->mRet.GetAsUInt();
        GLenum type = call->mArgs[0]->GetAsInt();
        StateTracker::Shader& s = contexts[context_index].shaders.add(id);
        s.shader_type = type;
    }
    else if (call->mCallName == "glDeleteShader")
    {
        GLuint id = call->mArgs[0]->GetAsUInt();
        // "DeleteShader will silently ignore the value zero". Also, some content assumes it will also
        // ignore invalid values, even though the standard does not guarantee this.
        if (id != 0)
        {
            contexts[context_index].shaders.remove(id);
        }
    }
    else if (call->mCallName == "glLinkProgram" || call->mCallName == "glLinkProgram2")
    {
        GLuint program = call->mArgs[0]->GetAsUInt();
        int target_program_index = contexts[context_index].programs.remap(program);
        std::string code;
        StateTracker::Program& p = contexts[context_index].programs[target_program_index];
        p.updated();
        // These need to be concatenated in the right order
        GLenum types[] = { GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, GL_TESS_EVALUATION_SHADER, GL_TESS_CONTROL_SHADER };
        for (GLenum type : types)
        {
            if (p.shaders.count(type) > 0)
            {
                const int shader_index = p.shaders[type];
                code += contexts[context_index].shaders[shader_index].source_code;
            }
        }
        p.md5sum = common::MD5Digest(code).text_lower();
    }
    else if (call->mCallName == "glShaderSource")
    {
        const GLuint shader = call->mArgs[0]->GetAsUInt();
        int target_shader_index = contexts[context_index].shaders.remap(shader);
        std::string code;
        for (unsigned i = 0; i < call->mArgs[2]->mArrayLen; i++)
        {
            int maxLen = -1;
            if (call->mArgs[3]->mType == common::Array_Type && call->mArgs[3]->mArrayLen > i)
            {
                maxLen = call->mArgs[3]->mArray[i].GetAsInt();
            }
            code += call->mArgs[2]->mArray[i].GetAsString(maxLen);
        }
        StateTracker::Shader& s = contexts[context_index].shaders[target_shader_index];
        s.source_code = code; // original shader
        s.call = call->mCallNo;
        GLSLParser parser;
        std::string stripped = parser.strip_comments(code);
        GLSLShader sh = parser.preprocessor(stripped, s.shader_type);
        if (sh.version / 10 > highest_gles_version && highest_gles_version > 10)
        {
            DBG_LOG("The use of shader in call %d increases GLES version from %d to %d\n", (int)call->mCallNo, (int)highest_gles_version, (int)sh.version / 10);
            highest_gles_version = sh.version / 10;
        }
        s.source_compressed = parser.compressed(sh);
        s.source_preprocessed = sh.code;
        GLSLRepresentation repr = parser.parse(sh);
        s.contains_invariants = repr.contains_invariants;
        s.contains_optimize_off_pragma = sh.contains_optimize_off_pragma;
        s.contains_debug_on_pragma = sh.contains_debug_on_pragma;
        s.contains_invariant_all_pragma = sh.contains_invariant_all_pragma;

        for (const auto& var : repr.global.members)
        {
            if (var.storage == Keyword::Out || var.storage == Keyword::Varying)
            {
                StateTracker::GLSLPrecision precision = StateTracker::HIGHP;
                if (var.precision == Keyword::Mediump) precision = StateTracker::MEDIUMP;
                else if (var.precision == Keyword::Lowp) precision = StateTracker::LOWP;
                s.varyings.emplace(var.name, StateTracker::GLSLVarying{ precision, lookup_get_gles_type(var.type), lookup_get_string(var.type) });
            }
            else if (lookup_is_sampler(var.type))
            {
                StateTracker::GLSLPrecision precision = StateTracker::HIGHP;
                if (var.precision == Keyword::Mediump) precision = StateTracker::MEDIUMP;
                else if (var.precision == Keyword::Lowp) precision = StateTracker::LOWP;
                s.samplers.emplace(var.name, StateTracker::GLSLSampler{ precision, var.binding, lookup_get_gles_type(var.type) });
            }
        }
        if (s.shader_type == GL_FRAGMENT_SHADER)
        {
            s.varying_locations_used = count_varying_locations_used(repr);
            s.lowp_varyings = count_varyings_by_precision(repr, Keyword::Lowp);
            s.mediump_varyings = count_varyings_by_precision(repr, Keyword::Mediump);
            s.highp_varyings = count_varyings_by_precision(repr, Keyword::Highp);
        }
        s.extensions = sh.extensions;
        for (const std::string& e : sh.extensions)
        {
            used_extensions.insert(e);
        }
    }
    else if (call->mCallName == "glGetUniformLocation") // these are injected if necessary by tracer
    {
        GLint location = call->mRet.GetAsInt();
        GLuint program = call->mArgs[0]->GetAsUInt();
        if (contexts[context_index].programs.contains(program))
        {
            const std::string name = call->mArgs[1]->GetAsString();
            const int target_program_index = contexts[context_index].programs.remap(program);
            contexts[context_index].programs[target_program_index].uniformNames[location] = name;
            contexts[context_index].programs[target_program_index].uniformLocations[name] = location;
        }
    }
    // Standard: "Sampler values must be set by calling Uniform1i{v}". That's why we only save those.
    else if (call->mCallName == "glUniform1i")
    {
        contexts[context_index].uniform_change(frames);
        const GLint location = call->mArgs[0]->GetAsInt();
        if (location != -1)
        {
            const GLint value = call->mArgs[1]->GetAsInt();
            const int program_index = contexts[context_index].program_index;
            if (program_index != UNBOUND)
            {
                contexts[context_index].programs[program_index].uniformValues[location].resize(1);
                contexts[context_index].programs[program_index].uniformValues[location][0] = value;
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
                assert(contexts[context_index].programs[program_index].uniformValues[location].size() == 1);
                if (contexts[context_index].programs[program_index].uniformNames.count(location))
                {
                    const std::string& name = contexts[context_index].programs[program_index].uniformNames.at(location);
                    if (contexts[context_index].programs[program_index].texture_bindings.count(name))
                    {
                        contexts[context_index].programs[program_index].texture_bindings[name].value = value;
                    }
                }
            }
        }
    }
    else if (call->mCallName == "glUniform1iv")
    {
        contexts[context_index].uniform_change(frames);
        const GLint location = call->mArgs[0]->GetAsInt();
        if (location != -1)
        {
            const GLsizei count = call->mArgs[1]->GetAsInt();
            assert(call->mArgs[2]->IsArray());
            assert(count == (int)call->mArgs[2]->mArrayLen);
            const int program_index = contexts[context_index].program_index;
            if (program_index != UNBOUND)
            {
                contexts[context_index].programs[program_index].uniformValues[location].resize(count);
                for (int i = 0; i < count; i++)
                {
                    const GLint value = call->mArgs[2]->mArray[i].GetAsInt();
                    contexts[context_index].programs[program_index].uniformValues[location][i] = value;
                    const std::string& name = contexts[context_index].programs[program_index].uniformNames.at(location);
                    if (contexts[context_index].programs[program_index].texture_bindings.count(name))
                    {
                        contexts[context_index].programs[program_index].texture_bindings[name].value = value;
                    }
                }
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
                contexts[context_index].programs[program_index].updated();
            }
        }
    }
    // glProgramUniform1i and glProgramUniform1iv also apply because they are functional mirrors of the above.
    else if (call->mCallName == "glProgramUniform1i")
    {
        contexts[context_index].uniform_change(frames);
        const GLuint program = call->mArgs[0]->GetAsUInt();
        const GLint location = call->mArgs[1]->GetAsInt();
        if (location != -1)
        {
            const GLint value = call->mArgs[2]->GetAsInt();
            const int program_index = contexts[context_index].programs.remap(program);
            if (program_index != UNBOUND)
            {
                contexts[context_index].programs[program_index].uniformValues[location].resize(1);
                contexts[context_index].programs[program_index].uniformValues[location][0] = value;
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
                contexts[context_index].programs[program_index].updated();
                const std::string& name = contexts[context_index].programs[program_index].uniformNames.at(location);
                if (contexts[context_index].programs[program_index].texture_bindings.count(name))
                {
                    contexts[context_index].programs[program_index].texture_bindings[name].value = value;
                }
            }
        }
    }
    else if (call->mCallName == "glProgramUniform1iv")
    {
        contexts[context_index].uniform_change(frames);
        const GLuint program = call->mArgs[0]->GetAsUInt();
        const GLint location = call->mArgs[1]->GetAsInt();
        if (location != -1)
        {
            const GLsizei count = call->mArgs[2]->GetAsInt();
            assert(call->mArgs[3]->IsArray());
            assert(count == (int)call->mArgs[3]->mArrayLen);
            const int program_index = contexts[context_index].programs.remap(program);
            if (program_index != UNBOUND)
            {
                contexts[context_index].programs[program_index].uniformValues[location].resize(count);
                for (int i = 0; i < count; i++)
                {
                    const GLint value = call->mArgs[3]->mArray[i].GetAsInt();
                    contexts[context_index].programs[program_index].uniformValues[location][i] = value;
                    const std::string& name = contexts[context_index].programs[program_index].uniformNames.at(location);
                    if (contexts[context_index].programs[program_index].texture_bindings.count(name))
                    {
                        contexts[context_index].programs[program_index].texture_bindings[name].value = value;
                    }
                }
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
                contexts[context_index].programs[program_index].updated();
            }
        }
    }
    else if (call->mCallName.find("glUniform") != std::string::npos || call->mCallName.find("glProgramUniform") != std::string::npos) // other uniforms
    {
        contexts[context_index].uniform_change(frames);
        GLint location = 0;
        GLuint program_id = 0;
        int program_index = UNBOUND;
        if (call->mCallName.find("glUniform") != std::string::npos)
        {
            program_index = contexts[context_index].program_index;
            if (program_index != UNBOUND)
            {
                program_id = contexts[context_index].programs[program_index].id;
                location = call->mArgs[0]->GetAsInt();
            }
        }
        else // glProgramUniform*()
        {
            program_id = call->mArgs[0]->GetAsUInt();
            if (contexts[context_index].programs.contains(program_id))
            {
                program_index = contexts[context_index].programs.remap(program_id);
                location = call->mArgs[1]->GetAsInt();
            }
        }
        if (program_index != UNBOUND)
        {
            contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
            contexts[context_index].programs[program_index].updated();
            // special dupe check
            if (call->mCallName == "glUniform1f" || call->mCallName == "glUniform2f" || call->mCallName == "glUniform3f" || call->mCallName == "glUniform4f")
            {
                std::vector<GLfloat> v;
                v.push_back(call->mArgs[1]->GetAsFloat());
                if (call->mCallName == "glUniform2f" || call->mCallName == "glUniform3f" || call->mCallName == "glUniform4f") v.push_back(call->mArgs[2]->GetAsFloat());
                if (call->mCallName == "glUniform3f" || call->mCallName == "glUniform4f") v.push_back(call->mArgs[3]->GetAsFloat());
                if (call->mCallName == "glUniform4f") v.push_back(call->mArgs[4]->GetAsFloat());
                auto& v2 = contexts[context_index].programs[program_index].uniformfValues[location];
                bool dupe = (v.size() == v2.size());
                if (dupe) for (unsigned i = 0; i < v.size(); i++) { if (v[i] != v2[i]) dupe = false; }
                if (dupe) callstats[call->mCallName].dupes++;
                v2 = v;
            }
        }
    }
    else if (call->mCallName == "glGenProgramPipelines" || call->mCallName == "glGenProgramPipelinesEXT")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].program_pipelines.add(id);
        }
    }
    else if (call->mCallName == "glDeleteProgramPipelines" || call->mCallName == "glDeleteProgramPipelinesEXT")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const GLuint id = call->mArgs[1]->mArray[i].GetAsUInt();
            // "Unused names in pipelines are silently ignored, as is the value zero."
            if (id != 0 && contexts[context_index].program_pipelines.contains(id))
            {
                contexts[context_index].program_pipelines.remove(id);
            }
            // "If a program pipeline object that is currently bound is deleted, the binding for that object
            // reverts to zero and no program pipeline becomes current."
            if (contexts[context_index].program_pipeline_index != UNBOUND
                && contexts[context_index].program_pipelines[contexts[context_index].program_pipeline_index].id == id)
            {
                contexts[context_index].program_pipeline_index = UNBOUND;
            }
        }
    }
    else if (call->mCallName == "glBindProgramPipeline" || call->mCallName == "glBindProgramPipelineEXT")
    {
        contexts[context_index].state_change(frames);
        GLuint id = call->mArgs[0]->GetAsUInt();
        if (id != 0)
        {
            if (!contexts[context_index].program_pipelines.contains(id)) // then create it
            {
                const auto& p = contexts[context_index].program_pipelines.add(id);
                contexts[context_index].program_pipeline_index = p.index;
            }
            else
            {
                contexts[context_index].program_pipeline_index = contexts[context_index].program_pipelines.remap(id);
            }
        }
        else
        {
            contexts[context_index].program_pipeline_index = UNBOUND;
        }
    }
    else if (call->mCallName == "glUseProgramStages" || call->mCallName == "glUseProgramStagesEXT")
    {
        contexts[context_index].state_change(frames);
        const GLuint pipeline = call->mArgs[0]->GetAsUInt();
        const GLuint stages = call->mArgs[1]->GetAsUInt();
        const GLuint program = call->mArgs[2]->GetAsUInt();
        const std::unordered_map<GLenum, GLenum> b2s = { { GL_VERTEX_SHADER_BIT, GL_VERTEX_SHADER }, { GL_FRAGMENT_SHADER_BIT, GL_FRAGMENT_SHADER },
              { GL_COMPUTE_SHADER_BIT, GL_COMPUTE_SHADER }, { GL_GEOMETRY_SHADER_BIT, GL_GEOMETRY_SHADER }, { GL_TESS_CONTROL_SHADER_BIT, GL_TESS_CONTROL_SHADER },
              { GL_TESS_EVALUATION_SHADER_BIT, GL_TESS_EVALUATION_SHADER } };
        if (pipeline != 0 && program != 0 && contexts[context_index].programs.contains(program))
        {
            const int pipeline_index = contexts[context_index].program_pipelines.remap(pipeline);
            const int program_index = contexts[context_index].programs.remap(program);
            StateTracker::ProgramPipeline& pipe = contexts[context_index].program_pipelines[pipeline_index];
            for (const auto pair : b2s)
            {
                if (stages & pair.first)
                {
                    pipe.program_stages[pair.second] = program_index;
                }
            }
        }
        else if (pipeline != 0) // invalid or zero program id => remove program from pipeline
        {
            const int pipeline_index = contexts[context_index].program_pipelines.remap(pipeline);
            StateTracker::ProgramPipeline& pipe = contexts[context_index].program_pipelines[pipeline_index];
            for (const auto pair : b2s)
            {
                if (stages & pair.first)
                {
                    pipe.program_stages.erase(pair.second);
                }
            }
        }
    }
    else if (call->mCallName == "glDrawBuffers")
    {
        contexts[context_index].state_change(frames);
        unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        contexts[context_index].draw_buffers.resize(count);
        for (unsigned i = 0; i < count; i++)
        {
            contexts[context_index].draw_buffers[i] = call->mArgs[1]->mArray[i].GetAsUInt();
        }
    }
    else if (call->mCallName == "glObjectLabel")
    {
        const GLenum identifier = call->mArgs[0]->GetAsUInt();
        const GLuint name = call->mArgs[1]->GetAsUInt();
        const GLsizei length = call->mArgs[2]->GetAsInt();
        const std::string label = call->mArgs[3]->GetAsString();
        switch (identifier)
        {
        case GL_FRAMEBUFFER:
            if (name != 0 && contexts[context_index].framebuffers.contains(name))
            {
                const int target_fb_index = contexts[context_index].framebuffers.remap(name);
                contexts[context_index].framebuffers[target_fb_index].label = label;
            }
            break;
        case GL_RENDERBUFFER:
            if (name != 0 && contexts[context_index].renderbuffers.contains(name))
            {
                const int target_rb_index = contexts[context_index].renderbuffers.remap(name);
                contexts[context_index].renderbuffers[target_rb_index].label = label;
            }
            break;
        case GL_TEXTURE:
            if (name != 0 && contexts[context_index].textures.contains(name))
            {
                const int target_tex_index = contexts[context_index].textures.remap(name);
                contexts[context_index].textures[target_tex_index].label = label;
            }
            break;
        case GL_PROGRAM:
            if (name != 0 && contexts[context_index].programs.contains(name))
            {
                const int target_index = contexts[context_index].programs.remap(name);
                contexts[context_index].programs[target_index].label = label;
            }
            break;
        default:
            break;
        }
    }
    else if (call->mCallName == "glFinish" && context_index != UNBOUND)
    {
        contexts[context_index].finish_calls_per_frame[frames]++;
    }
    else if (call->mCallName == "glFlush" && context_index != UNBOUND)
    {
        contexts[context_index].flush_calls_per_frame[frames]++;
    }
    else if (call->mCallName == "glPatchParameteri" || call->mCallName == "glPatchParameteriEXT")
    {
        contexts[context_index].state_change(frames);
        GLenum pname = call->mArgs[0]->GetAsUInt();
        check_enum(call->mCallName, pname);
        if (pname == GL_PATCH_VERTICES)
        {
            contexts[context_index].patchSize = call->mArgs[1]->GetAsInt();
        }
    }
    else if (call->mCallName == "glDepthMask")
    {
        contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.depthmask = call->mArgs[0]->GetAsUInt();
    }
    else if (call->mCallName == "glDepthFunc")
    {
        const GLenum depthfunc = call->mArgs[0]->GetAsUInt();
        check_enum(call->mCallName, depthfunc);
        if (depthfunc != contexts[context_index].fillstate.depthfunc)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        contexts[context_index].fillstate.depthfunc = depthfunc;
    }
    else if (call->mCallName == "glStencilMask")
    {
        contexts[context_index].state_change(frames);
        const GLuint mask = call->mArgs[0]->GetAsUInt();
        contexts[context_index].fillstate.stencilwritemask[GL_FRONT] = mask;
        contexts[context_index].fillstate.stencilwritemask[GL_BACK] = mask;
    }
    else if (call->mCallName == "glStencilMaskSeparate")
    {
        contexts[context_index].state_change(frames);
        const GLenum face = call->mArgs[0]->GetAsUInt();
        check_enum(call->mCallName, face);
        const GLuint mask = call->mArgs[1]->GetAsUInt();
        if (face == GL_FRONT_AND_BACK)
        {
            contexts[context_index].fillstate.stencilwritemask[GL_FRONT] = mask;
            contexts[context_index].fillstate.stencilwritemask[GL_BACK] = mask;
        }
        else
        {
            contexts[context_index].fillstate.stencilwritemask[face] = mask;
        }
    }
    else if (call->mCallName == "glStencilFuncSeparate")
    {
        contexts[context_index].state_change(frames);
        const GLenum face = call->mArgs[0]->GetAsUInt();
        check_enum(call->mCallName, face);
        const GLuint func = call->mArgs[1]->GetAsUInt();
        const GLint ref = call->mArgs[2]->GetAsUInt();
        const GLuint mask = call->mArgs[1]->GetAsUInt();
        if (face == GL_FRONT_AND_BACK)
        {
            contexts[context_index].fillstate.stencilfunc[GL_FRONT] = func;
            contexts[context_index].fillstate.stencilfunc[GL_BACK] = func;
            contexts[context_index].fillstate.stencilref[GL_FRONT] = ref;
            contexts[context_index].fillstate.stencilref[GL_BACK] = ref;
            contexts[context_index].fillstate.stencilcomparemask[GL_FRONT] = mask;
            contexts[context_index].fillstate.stencilcomparemask[GL_BACK] = mask;
        }
        else
        {
            contexts[context_index].fillstate.stencilfunc[face] = func;
            contexts[context_index].fillstate.stencilref[face] = ref;
            contexts[context_index].fillstate.stencilcomparemask[face] = mask;
        }
    }
    else if (call->mCallName == "glColorMask")
    {
        contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.colormask[0] = call->mArgs[0]->GetAsUInt();
        contexts[context_index].fillstate.colormask[1] = call->mArgs[1]->GetAsUInt();
        contexts[context_index].fillstate.colormask[2] = call->mArgs[2]->GetAsUInt();
        contexts[context_index].fillstate.colormask[3] = call->mArgs[3]->GetAsUInt();
    }
    else if (call->mCallName == "glClearColor")
    {
        contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.clearcolor[0] = call->mArgs[0]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[1] = call->mArgs[1]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[2] = call->mArgs[2]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[3] = call->mArgs[3]->GetAsFloat();
    }
    else if (call->mCallName == "glClearStencil")
    {
        contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.clearstencil = call->mArgs[0]->GetAsInt();
    }
    else if (call->mCallName == "glClearDepthf")
    {
        contexts[context_index].state_change(frames);
        contexts[context_index].fillstate.cleardepth = call->mArgs[0]->GetAsFloat();
    }
    else if (call->mCallName.compare(0, 10, "glDispatch") == 0)
    {
        contexts[context_index].state_change(frames);
        const int current_program = contexts[context_index].program_index;
        contexts[context_index].programs[current_program].stats.dispatches++;
        contexts[context_index].programs[current_program].stats_per_frame[frames].dispatches++;
        StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
        rp.active = true;
        rp.used_programs.insert(current_program);
        for (auto pair : contexts[context_index].image_binding)
        {
            if (pair.second == 0) continue;
            // assume it is used if referenced in this manner, although it would be better to do this with introspection
            const int target_texture_index = contexts[context_index].textures.remap(pair.second);
            StateTracker::Texture& tx = contexts[context_index].textures.at(target_texture_index);
            if (frames >= ff_startframe && frames <= ff_endframe) tx.used = true;
            if (tx.mipmaps.rbegin() != tx.mipmaps.rend())
            {
                if (frames >= ff_startframe && frames <= ff_endframe) tx.mipmaps.rbegin()->second.used = true;
            }
        }
    }
    else if (call->mCallName == "glClear")
    {
       contexts[context_index].state_change(frames);
       GLbitfield mask = call->mArgs[0]->GetAsUInt();
       if (mask & GL_COLOR_BUFFER_BIT) // interacts with glDrawBuffers()
       {
           for (GLenum e : contexts[context_index].draw_buffers)
           {
               GLenum attachment = (e == GL_BACK) ? GL_COLOR_ATTACHMENT0 : e; // we store backbuffer as attachment 0
               if (e != GL_NONE)
               {
                   const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
                   StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
                   if (fbo.attachments.count(attachment) > 0) // "If a buffer is not present, then a glClear directed at that buffer has no effect."
                   {
                       StateTracker::Attachment& at = fbo.attachments.at(attachment);
                       contexts[context_index].fillstate.call_stored = call->mCallNo - 1;
                       find_duplicate_clears(contexts[context_index].fillstate, at, GL_COLOR, fbo, call->mCallName);
                       at.clears.push_back(contexts[context_index].fillstate);
                       contexts[context_index].updated_fbo_attachment(index, attachment);
                   }
               }
           }
       }
       if ((mask & GL_DEPTH_BUFFER_BIT) && contexts[context_index].fillstate.depthmask)
       {
           const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
           StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
           if (fbo.attachments.count(GL_DEPTH_ATTACHMENT) > 0)
           {
               StateTracker::Attachment& at = fbo.attachments.at(GL_DEPTH_ATTACHMENT);
               contexts[context_index].fillstate.call_stored = call->mCallNo - 1;
               find_duplicate_clears(contexts[context_index].fillstate, at, GL_DEPTH, fbo, call->mCallName);
               at.clears.push_back(contexts[context_index].fillstate);
               contexts[context_index].updated_fbo_attachment(index, GL_DEPTH_ATTACHMENT);
           }
       }
       if ((mask & GL_STENCIL_BUFFER_BIT) && (contexts[context_index].fillstate.stencilwritemask[GL_BACK] || contexts[context_index].fillstate.stencilwritemask[GL_FRONT]))
       {
           const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
           StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
           if (fbo.attachments.count(GL_STENCIL_ATTACHMENT) > 0)
           {
               StateTracker::Attachment& at = fbo.attachments.at(GL_STENCIL_ATTACHMENT);
               contexts[context_index].fillstate.call_stored = call->mCallNo - 1;
               find_duplicate_clears(contexts[context_index].fillstate, at, GL_STENCIL, fbo, call->mCallName);
               at.clears.push_back(contexts[context_index].fillstate);
               contexts[context_index].updated_fbo_attachment(index, GL_STENCIL_ATTACHMENT);
           }
       }
    }
    else if (call->mCallName == "glClearBufferfi")
    {
       contexts[context_index].state_change(frames);
       GLenum buffertype = call->mArgs[0]->GetAsUInt();
       assert(buffertype == GL_DEPTH_STENCIL);
       GLint drawbuffer = call->mArgs[1]->GetAsInt();
       assert(drawbuffer == 0);
       const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
       StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
       StateTracker::FillState& fillstate = contexts[context_index].fillstate;
       fillstate.call_stored = call->mCallNo - 1;
       fillstate.cleardepth = call->mArgs[2]->GetAsFloat();
       fillstate.clearstencil = call->mArgs[3]->GetAsInt();
       if (fbo.attachments.count(GL_STENCIL_ATTACHMENT) > 0)
       {
           StateTracker::Attachment& stencil = fbo.attachments.at(GL_STENCIL_ATTACHMENT);
           find_duplicate_clears(fillstate, stencil, GL_STENCIL, fbo, call->mCallName);
           stencil.clears.push_back(fillstate);
           contexts[context_index].updated_fbo_attachment(index, GL_STENCIL_ATTACHMENT);
       }
       if (fbo.attachments.count(GL_DEPTH_ATTACHMENT) > 0)
       {
           StateTracker::Attachment& depth = fbo.attachments.at(GL_DEPTH_ATTACHMENT);
           find_duplicate_clears(fillstate, depth, GL_DEPTH, fbo, call->mCallName);
           depth.clears.push_back(fillstate);
           contexts[context_index].updated_fbo_attachment(index, GL_DEPTH_ATTACHMENT);
       }
    }
    else if (call->mCallName.compare(0, 13, "glClearBuffer") == 0) // except glClearBufferfi which is handled above
    {
       contexts[context_index].state_change(frames);
       GLenum buffertype = call->mArgs[0]->GetAsUInt();
       GLint drawbuffer = call->mArgs[1]->GetAsInt();
       GLenum attachment;
       switch (buffertype)
       {
       case GL_COLOR: attachment = GL_COLOR_ATTACHMENT0 + drawbuffer; break;
       case GL_STENCIL: attachment = GL_STENCIL_ATTACHMENT; break;
       case GL_DEPTH: attachment = GL_DEPTH_ATTACHMENT; break;
       default: assert(false); attachment = GL_COLOR_ATTACHMENT0; break;
       }
       const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
       StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
       fbo.updated();
       if (fbo.attachments.count(attachment) > 0)
       {
           contexts[context_index].updated_fbo_attachment(index, attachment);
           StateTracker::Attachment& at = fbo.attachments.at(attachment);
           StateTracker::FillState& fillstate = contexts[context_index].fillstate;
           fillstate.call_stored = call->mCallNo - 1;
           switch (buffertype)
           {
           case GL_COLOR:
               assert(call->mArgs[2]->mArrayLen == 4);
               fillstate.clearcolor[0] = call->mArgs[2]->mArray[0].GetAsFloat();
               fillstate.clearcolor[1] = call->mArgs[2]->mArray[1].GetAsFloat();
               fillstate.clearcolor[2] = call->mArgs[2]->mArray[2].GetAsFloat();
               fillstate.clearcolor[3] = call->mArgs[2]->mArray[3].GetAsFloat();
               break;
           case GL_STENCIL:
               assert(call->mArgs[2]->mArrayLen == 1);
               fillstate.clearstencil = call->mArgs[2]->mArray[0].GetAsInt();
               break;
           case GL_DEPTH:
               assert(call->mArgs[2]->mArrayLen == 1);
               fillstate.cleardepth = call->mArgs[2]->mArray[0].GetAsInt();
               break;
           default: assert(false); break;
           }
           find_duplicate_clears(fillstate, at, buffertype, fbo, call->mCallName);
           at.clears.push_back(fillstate);
       }
    }
    else if (call->mCallName == "glBlendFuncSeparate")
    {
        StateTracker::FillState& fillstate = contexts[context_index].fillstate;
        GLenum srcRGB = call->mArgs[0]->GetAsUInt();
        GLenum dstRGB = call->mArgs[1]->GetAsUInt();
        GLenum srcAlpha = call->mArgs[2]->GetAsUInt();
        GLenum dstAlpha = call->mArgs[3]->GetAsUInt();
        check_enum(call->mCallName, srcRGB);
        check_enum(call->mCallName, dstRGB);
        check_enum(call->mCallName, srcAlpha);
        check_enum(call->mCallName, dstAlpha);
        if (srcRGB != fillstate.blend_rgb.source || fillstate.blend_rgb.destination != dstRGB
            || fillstate.blend_alpha.source != srcAlpha || fillstate.blend_alpha.destination != dstAlpha)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        fillstate.blend_rgb.source = srcRGB;
        fillstate.blend_rgb.destination = dstRGB;
        fillstate.blend_alpha.source = srcAlpha;
        fillstate.blend_alpha.destination = dstAlpha;
    }
    else if (call->mCallName == "glBlendFunc")
    {
        StateTracker::FillState& fillstate = contexts[context_index].fillstate;
        GLenum src = call->mArgs[0]->GetAsUInt();
        GLenum dst = call->mArgs[1]->GetAsUInt();
        check_enum(call->mCallName, src);
        check_enum(call->mCallName, dst);
        if (fillstate.blend_rgb.source != src || fillstate.blend_rgb.destination != dst
            || fillstate.blend_alpha.source != src || fillstate.blend_alpha.destination != dst)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        fillstate.blend_rgb.source = src;
        fillstate.blend_rgb.destination = dst;
        fillstate.blend_alpha.source = src;
        fillstate.blend_alpha.destination = dst;
    }
    else if (call->mCallName == "glBlendEquationSeparate")
    {
        StateTracker::FillState& fillstate = contexts[context_index].fillstate;
        contexts[context_index].state_change(frames);
        GLenum modeRGB = call->mArgs[0]->GetAsUInt();
        GLenum modeAlpha = call->mArgs[1]->GetAsUInt();
        check_enum(call->mCallName, modeRGB);
        check_enum(call->mCallName, modeAlpha);
        fillstate.blend_rgb.operation = modeRGB;
        fillstate.blend_alpha.operation = modeAlpha;
    }
    else if (call->mCallName == "glBlendEquation")
    {
        StateTracker::FillState& fillstate = contexts[context_index].fillstate;
        contexts[context_index].state_change(frames);
        GLenum mode = call->mArgs[0]->GetAsUInt();
        check_enum(call->mCallName, mode);
        fillstate.blend_rgb.operation = mode;
        fillstate.blend_alpha.operation = mode;
    }
    else if (call->mCallName == "glBlendColor")
    {
        StateTracker::FillState& fillstate = contexts[context_index].fillstate;
        GLfloat red = call->mArgs[0]->GetAsFloat();
        GLfloat green = call->mArgs[1]->GetAsFloat();
        GLfloat blue = call->mArgs[2]->GetAsFloat();
        GLfloat alpha = call->mArgs[3]->GetAsFloat();
        if (fillstate.blendFactor[0] != red || fillstate.blendFactor[1] != green || fillstate.blendFactor[2] != blue || fillstate.blendFactor[3] != alpha)
        {
            contexts[context_index].state_change(frames);
        }
        else callstats[call->mCallName].dupes++;
        fillstate.blendFactor = { red, green, blue, alpha };
    }
    else if (call->mCallName == "glDepthRangef")
    {
        contexts[context_index].viewport.near = call->mArgs[0]->GetAsFloat();
        contexts[context_index].viewport.far = call->mArgs[1]->GetAsFloat();
    }
    else if (call->mCallName == "glInvalidateFramebuffer" || call->mCallName == "glDiscardFramebufferEXT" || call->mCallName == "glInvalidateSubFramebuffer")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        assert(target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER);
        const GLsizei numAttachments = call->mArgs[1]->GetAsInt();
        assert(call->mArgs[2]->IsArray());
        assert(call->mArgs[2]->mArrayLen == (unsigned)numAttachments);
        for (int i = 0; i < numAttachments; i++)
        {
            GLenum attachment = call->mArgs[2]->mArray[i].GetAsUInt();
            if (((attachment == GL_DEPTH_ATTACHMENT || attachment == GL_STENCIL_ATTACHMENT || (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT31)) && contexts[context_index].drawframebuffer == 0)
                || ((attachment == GL_COLOR || attachment == GL_DEPTH || attachment == GL_STENCIL) && contexts[context_index].drawframebuffer != 0))
            {
                if (mDebug) DBG_LOG("%s(%s, %d, ...) used %s with framebuffer %u!\n", call->mCallName.c_str(), texEnum(target).c_str(), (int)numAttachments,
                                    texEnum(attachment).c_str(), contexts[context_index].drawframebuffer);
                continue; // buggy
            }
            StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
            const int fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
            StateTracker::Framebuffer &fbo = contexts[context_index].framebuffers.at(fb_index);
            if (attachment == GL_COLOR) attachment = GL_COLOR_ATTACHMENT0;
            else if (attachment == GL_DEPTH) attachment = GL_DEPTH_ATTACHMENT;
            else if (attachment == GL_STENCIL) attachment = GL_STENCIL_ATTACHMENT;
            if (fbo.attachments.count(attachment) == 0)
            {
                if (mDebug)
                {
                    DBG_LOG("%s(%s, %d, ...) used %s with framebuffer %u -- which does not exist!\n", call->mCallName.c_str(), texEnum(target).c_str(), (int)numAttachments, texEnum(attachment).c_str(), contexts[context_index].drawframebuffer);
                    for (const auto& pair : fbo.attachments) DBG_LOG("\thas %s type=%s id=%u idx=%d\n", texEnum(pair.first).c_str(), texEnum(pair.second.type).c_str(), pair.second.id, pair.second.index);
                }
                continue; // buggy
            }
            fbo.attachments.at(attachment).invalidated = true;
            fbo.updated();
            contexts[context_index].updated_fbo_attachment(fb_index, attachment);
            if (call->mCallName == "glInvalidateFramebuffer" || attachment != GL_COLOR_ATTACHMENT0) continue; // using the sane version - don't need the test below
            for (GLenum e : contexts[context_index].draw_buffers)
            {
                GLenum attachment = (e == GL_BACK) ? GL_COLOR_ATTACHMENT0 : e; // we store backbuffer as attachment 0
                if (e != GL_NONE && e >= GL_COLOR_ATTACHMENT0 && e <= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS && fbo.attachments.count(e) > 0)
                {
                    fbo.attachments.at(e).invalidated = true;
                    contexts[context_index].updated_fbo_attachment(fb_index, e);
                }
            }
        }
    }
    else if (call->mCallName == "glBindVertexBuffer" || call->mCallName == "glVertexAttribFormat" || call->mCallName == "glVertexAttribIFormat"
             || call->mCallName == "glVertexLAttribFormat")
    {
        static bool has_printed = false;
        if (!has_printed) { DBG_LOG("Unsupported: %s\n", call->mCallName.c_str()); has_printed = true; }
    }
    else if (call->mCallName.compare(0, 6, "glDraw") == 0 && call->mCallName != "glDrawBuffers")
    {
        draws++;
        contexts[context_index].draws_since_last_state_change++;
        contexts[context_index].draws_since_last_state_or_uniform_change++;
        contexts[context_index].draw_calls_per_frame[frames]++;
        contexts[context_index].draws_since_last_state_or_index_buffer_change++;
        if (contexts[context_index].program_index == UNBOUND && contexts[context_index].program_pipeline_index == UNBOUND)
        {
            return; // valid for GLES1 content!
        }
        StateTracker::VertexArrayObject& vao = contexts[context_index].vaos.at(contexts[context_index].vao_index);
        StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
        rp.active = true;
        rp.drawframebuffer = contexts[context_index].drawframebuffer;
        rp.last_call = call->mCallNo;
        std::unordered_map<GLenum, int> program_stages; // to handle both single program and program pipelines with same code
        if (contexts[context_index].program_index != UNBOUND)
        {
            program_stages[0] = contexts[context_index].program_index;
        }
        else
        {
            program_stages = contexts[context_index].program_pipelines.at(contexts[context_index].program_pipeline_index).program_stages;
        }
        const int fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
        contexts[context_index].framebuffers[fb_index].used++;
        // resuse does happen in the NBA games... need to handle this somehow
        //assert(rp.attachments.size() == 0 || contexts[context_index].framebuffers[fb_index].attachments.size() == rp.attachments.size()); // no resizing!
        rp.attachments.resize(contexts[context_index].framebuffers[fb_index].attachments.size());
        int i = 0;
        for (auto& rb : contexts[context_index].framebuffers[fb_index].attachments)
        {
            if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN && rb.second.invalidated)
            {
                rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_DONT_CARE;
            }
            else if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN && rb.second.clears.size() > 0)
            {
                rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_CLEAR;
            }
            else if (rp.attachments[i].load_op == StateTracker::RenderPass::LOAD_OP_UNKNOWN)
            {
                rp.attachments[i].load_op = StateTracker::RenderPass::LOAD_OP_LOAD;
            }
            rp.attachments[i].id = rb.second.id;
            rp.attachments[i].slot = rb.first;
            rb.second.clears.clear();
            rb.second.invalidated = false;
            if (rb.second.id > 0 && rb.second.type == GL_RENDERBUFFER && contexts[context_index].renderbuffers.contains(rb.second.id))
            {
                const int index = contexts[context_index].renderbuffers.remap(rb.second.id);
                rp.used_renderbuffers.insert(index);
                rp.render_targets[rb.first].first = GL_RENDERBUFFER;
                rp.render_targets[rb.first].second = index;
                rp.attachments[i].index = index;
                assert(rp.attachments[i].type == GL_RENDERBUFFER || rp.attachments[i].type == GL_NONE);
                rp.attachments[i].type = GL_RENDERBUFFER;
                StateTracker::Renderbuffer& r = contexts[context_index].renderbuffers.at(index);
                rp.attachments[i].format = r.internalformat;
                rp.width = r.width;
                rp.height = r.height;
                rp.depth = 1;
            }
            else if (rb.second.id > 0 && contexts[context_index].textures.contains(rb.second.id))
            {
                const int index = contexts[context_index].textures.remap(rb.second.id);
                rp.used_texture_targets.insert(index);
                rp.render_targets[rb.first].first = GL_TEXTURE;
                rp.render_targets[rb.first].second = index;
                rp.attachments[i].index = index;
                StateTracker::Texture& tx = contexts[context_index].textures.at(index);
                tx.initialized = true; // assume it is used and thus initialized
                if (frames >= ff_startframe && frames <= ff_endframe) tx.used = true;
                if (tx.mipmaps.rbegin() != tx.mipmaps.rend())
                {
                    if (frames >= ff_startframe && frames <= ff_endframe) tx.mipmaps.rbegin()->second.used = true;
                }
                rp.attachments[i].format = tx.internal_format;
                assert(rp.attachments[i].type == tx.binding_point || rp.attachments[i].type == GL_NONE);
                rp.attachments[i].type = tx.binding_point;
                rp.width = tx.width;
                rp.height = tx.height;
                rp.depth = tx.depth;
            }
            else if (surface_index != UNBOUND) // backbuffer
            {
                rp.width = surfaces[surface_index].width;
                rp.height = surfaces[surface_index].height;
                rp.depth = 1;
                rp.attachments[i].slot = GL_BACK;
                rp.attachments[i].type = rb.second.type;
                assert(rb.second.type != GL_NONE);
                // Convert from egl config to framebuffer format
                if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 8 && jsonConfig.depth == 24) rp.attachments[i].slot = GL_NONE; // remove
                else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 8) rp.attachments[i].format = GL_UNSIGNED_BYTE;
                else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 16) rp.attachments[i].format = GL_UNSIGNED_SHORT;
                else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 32) rp.attachments[i].format = GL_UNSIGNED_INT;
                else if (rb.first == GL_STENCIL_ATTACHMENT && jsonConfig.stencil == 0) rp.attachments[i].slot = GL_NONE; // remove
                else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 8) rp.attachments[i].format = GL_UNSIGNED_BYTE;
                else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 24) { rp.attachments[i].format = GL_DEPTH24_STENCIL8; rp.attachments[i].type = GL_DEPTH_STENCIL; }
                else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 16) rp.attachments[i].format = GL_UNSIGNED_SHORT;
                else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 32) rp.attachments[i].format = GL_UNSIGNED_INT;
                else if (rb.first == GL_DEPTH_ATTACHMENT && jsonConfig.depth == 0) rp.attachments[i].slot = GL_NONE; // remove
                else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 5) rp.attachments[i].format = GL_RGB565;
                else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 8 && jsonConfig.alpha == 0) rp.attachments[i].format = GL_RGB8;
                else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 8 && jsonConfig.alpha == 8) rp.attachments[i].format = GL_RGBA8;
                else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 10 && jsonConfig.alpha == 2) rp.attachments[i].format = GL_RGB10_A2;
                else if (rb.first == GL_COLOR_ATTACHMENT0 && jsonConfig.red == 11) rp.attachments[i].format = GL_R11F_G11F_B10F;
            }
            i++;
        }
        rp.draw_calls++;
        update_renderpass(call, contexts[context_index], rp, fb_index);
        DrawParams params = getDrawCallCount(call);
        if (params.client_side_buffer_name != UNBOUND)
        {
            client_side_last_use[call->mTid][params.client_side_buffer_name] = call->mCallNo;
            client_side_last_use_reason[call->mTid][params.client_side_buffer_name] = call->mCallName;
        }
        if (vao.boundVertexAttribs.count(GL_ARRAY_BUFFER) > 0)
        {
            for (const auto pair : vao.boundVertexAttribs) // assume any enabled arrays are accessed
            {
                if (vao.array_enabled.count(pair.first))
                {
                    client_side_last_use[call->mTid][std::get<5>(pair.second)] = call->mCallNo;
                    client_side_last_use_reason[call->mTid][std::get<5>(pair.second)] = call->mCallName;
                }
            }
        }
        if (params.value_type != GL_NONE) // if is indexed call
        {
            if (contexts[context_index].last_index_count != params.vertices || contexts[context_index].last_index_buffer != params.index_buffer
                || contexts[context_index].last_index_type != params.value_type)
            {
                contexts[context_index].index_buffer_change(frames);
            }
            if (vao.boundBufferIds.count(GL_ELEMENT_ARRAY_BUFFER) > 0
                && vao.boundBufferIds[GL_ELEMENT_ARRAY_BUFFER][0].buffer != 0) // bound gpu index buffer
            {
                if (params.value_type == GL_UNSIGNED_BYTE) contexts[context_index].bound_index_ui8_calls_per_frame[frames]++;
                else if (params.value_type == GL_UNSIGNED_SHORT) contexts[context_index].bound_index_ui16_calls_per_frame[frames]++;
                else if (params.value_type == GL_UNSIGNED_INT) contexts[context_index].bound_index_ui32_calls_per_frame[frames]++;
            }
            else // client buffer
            {
                if (params.value_type == GL_UNSIGNED_BYTE) contexts[context_index].client_index_ui8_calls_per_frame[frames]++;
                else if (params.value_type == GL_UNSIGNED_SHORT) contexts[context_index].client_index_ui16_calls_per_frame[frames]++;
                else if (params.value_type == GL_UNSIGNED_INT) contexts[context_index].client_index_ui32_calls_per_frame[frames]++;
            }
            contexts[context_index].last_index_count = params.vertices;
            contexts[context_index].last_index_type = params.value_type;
            contexts[context_index].last_index_buffer = params.index_buffer;
        }
        int program_idx = contexts[context_index].program_index;
        if (program_idx == UNBOUND && contexts[context_index].program_pipeline_index != UNBOUND) // separate shader programs
        {
            program_idx = contexts[context_index].program_pipelines.at(contexts[context_index].program_pipeline_index).program_stages.at(GL_FRAGMENT_SHADER);
        }
        StateTracker::Program& p = contexts[context_index].programs.at(program_idx);
        for (auto& unitpair : contexts[context_index].textureUnits)
        {
            const GLenum unit = unitpair.first;
            for (auto& targetpair : unitpair.second)
            {
                const GLenum target = targetpair.first;
                const GLuint texture_id = targetpair.second;
                if (texture_id == 0 || !contexts[context_index].textures.contains(texture_id)) continue;
                const int idx = contexts[context_index].textures.remap(texture_id);
                StateTracker::Texture& tx = contexts[context_index].textures.at(idx); // we really shouldn't touch it here...
                if (!tx.initialized)
                {
                    tx.uninit_usage = true;
                }
            }
        }
        for (auto& pair : p.texture_bindings) // these are only set if replayed for real
        {
            const StateTracker::Program::ProgramSampler& s = pair.second;
            const GLenum binding = samplerTypeToBindingType(s.type);
            if (binding != GL_NONE && contexts[context_index].textureUnits.count(s.value) > 0 && contexts[context_index].textureUnits[s.value].count(binding) > 0)
            {
                const GLuint texture_id = contexts[context_index].textureUnits.at(s.value).at(binding);
                if (texture_id == 0) continue;
                const int idx = contexts[context_index].textures.remap(texture_id);
                StateTracker::Texture& tx = contexts[context_index].textures.at(idx); // touch it
                if (!tx.initialized)
                {
                    tx.uninit_usage = true;
                }
                if (frames >= ff_startframe && frames <= ff_endframe) tx.used = true;
                if (tx.mipmaps.rbegin() != tx.mipmaps.rend())
                {
                    if (frames >= ff_startframe && frames <= ff_endframe) tx.mipmaps.rbegin()->second.used = true;
                }
            }
        }
        // TBD the below does not take into account primitive restart
        for (const auto pair : program_stages)
        {
            const int program_index = pair.second;
            rp.used_programs.insert(pair.second);
            contexts[context_index].programs[program_index].stats.drawcalls++;
            contexts[context_index].programs[program_index].stats.vertices += params.vertices;
            contexts[context_index].programs[program_index].stats.primitives += params.primitives;
            contexts[context_index].programs[program_index].stats_per_frame[frames].drawcalls++;
            contexts[context_index].programs[program_index].stats_per_frame[frames].vertices += params.vertices;
            contexts[context_index].programs[program_index].stats_per_frame[frames].primitives += params.primitives;
        }
        rp.vertices += params.vertices;
        rp.primitives += params.primitives;
        if (mDumpRenderpassJson) completed_drawcall(frames, params, rp);
    }
    // this one should be last
    else if (call->mCallName == "glCullFace"
             || call->mCallName == "glFrontFace"
             || call->mCallName == "glPolygonOffset"
             || call->mCallName == "glPixelStorei"
             || call->mCallName == "glSampleCoverage"
             || call->mCallName.compare(0, 9, "glStencil") == 0
             || call->mCallName.compare(0, 9, "glEnablei") == 0
             || call->mCallName.compare(0, 10, "glDisablei") == 0
             || call->mCallName.compare(0, 14, "glVertexAttrib") == 0
             || call->mCallName == "glPolygonOffsetClampEXT"
             || call->mCallName == "glVertexAttribDivisorEXT")
    {
        contexts[context_index].state_change(frames);
    }
    else if (mDebug)
    {
        static std::set<std::string> handled;
        if (handled.count(call->mCallName) == 0) DBG_LOG("Not handled: %s\n", call->mCallName.c_str());
        handled.insert(call->mCallName);
    }
}

void ParseInterface::close()
{
    inputFile.Close();
    outputFile.Close();
}

void ParseInterface::loop(Callback c, void *data)
{
    bool cont = false;
    common::CallTM* call;
    do
    {
        call = next_call();
        if (call) cont = c(*this, call, data);
    }
    while (call && cont);
}

void ParseInterface::cleanup()
{
}
