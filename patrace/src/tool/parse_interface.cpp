#include <cassert>
#include "common/memory.hpp"

#include "parse_interface.h"
#include "specs/pa_func_to_version.h"
#include "glsl_parser.h"
#include "glsl_utils.h"

static const bool print_dupes = false;

// Does not set number of primitives. This needs to be handled separately, since we cannot query patch size.
DrawParams ParseInterfaceBase::getDrawCallCount(common::CallTM *call)
{
    DrawParams ret;

    if (call->mCallName == "glDrawElements" || call->mCallName == "glDrawElementsInstanced" || call->mCallName == "glDrawElementsBaseVertex"
        || call->mCallName == "glDrawElementsInstancedBaseVertex")
    {
        ret.vertices = call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawArrays" || call->mCallName == "glDrawArraysInstanced")
    {
        ret.vertices = call->mArgs[2]->GetAsUInt();
    }
    else if (call->mCallName == "glDrawRangeElements" || call->mCallName == "glDrawRangeElementsBaseVertex")
    {
        ret.vertices = call->mArgs[3]->GetAsUInt();
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

    ret.mode = call->mArgs[0]->GetAsUInt();
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
            same &= (i.stencilmask.at(GL_FRONT) == f.stencilmask.at(GL_FRONT) || f.stencilmask.at(GL_FRONT) == false);
            same &= (i.stencilmask.at(GL_BACK) == f.stencilmask.at(GL_BACK) || f.stencilmask.at(GL_BACK) == false);
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
    if (!inputFile.Open(input.c_str(), false, "tmp.pat.ra"))
    {
        DBG_LOG("Failed to open for reading: %s\n", input.c_str());
        return false;
    }
    _curFrame = inputFile.mFrames[0];
    _curFrame->LoadCalls(inputFile.mpInFileRA);
    if (!output.empty() && !outputFile.Open(output.c_str()))
    {
        DBG_LOG("Failed to open for writing: %s\n", output.c_str());
        return false;
    }
    header = inputFile.mpInFileRA->getJSONHeader();
    threadArray = header["threads"];
    defaultTid = header["defaultTid"].asUInt();
    highest_gles_version = header["glesVersion"].asInt() * 10;
    numThreads = threadArray.size();
    if (numThreads == 0)
    {
        DBG_LOG("Bad number of threads: %d\n", numThreads);
        return false;
    }
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
    if (_curCallIndexInFrame >= _curFrame->GetLoadedCallCount())
    {
        _curFrameIndex++;
        if (_curFrameIndex >= inputFile.mFrames.size())
            return NULL;
        if (_curFrame) _curFrame->UnloadCalls();
        _curFrame = inputFile.mFrames[_curFrameIndex];
        _curFrame->LoadCalls(inputFile.mpInFileRA);
        _curCallIndexInFrame = 0;
    }
    common::CallTM *call = _curFrame->mCalls[_curCallIndexInFrame];
    _curCallIndexInFrame++;
    context_index = current_context[call->mTid];

    if (!only_default || call->mTid == defaultTid)
    {
        interpret_call(call);
    }
    return call;
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
            DBG_LOG("Unsupported texture parameter for %s: %s\n", call->mCallName.c_str(), texEnum(pname).c_str());
            break;
        }
    }
    else // non-length-of-one array values - none supported yet
    {
        DBG_LOG("Unsupported texture array parameter for %s: %s\n", call->mCallName.c_str(), texEnum(pname).c_str());
    }
}

void ParseInterfaceBase::interpret_call(common::CallTM *call)
{
    // Check versions and extensions used
    if (map_func_to_version.count(call->mCallName))
    {
        int funcver = map_func_to_version.at(call->mCallName);
        highest_gles_version = std::max(funcver, highest_gles_version);
    }
    if (map_func_to_extension.count(call->mCallName))
    {
        const auto its = map_func_to_extension.equal_range(call->mCallName);
        for (auto it = its.first; it != its.second; ++it)
        {
            used_extensions.insert(it->second);
        }
    }

    if (call->mCallName == "eglMakeCurrent") // find contexts that are used
    {
        int surface = call->mArgs[1]->GetAsInt();
        int readsurface = call->mArgs[2]->GetAsInt();
        int context = call->mArgs[3]->GetAsInt();

        if (surface != readsurface)
        {
            DBG_LOG("WARNING: Read surface != write surface not supported at call %u!\n", call->mCallNo);
        }

        if (context == (int64_t)EGL_NO_CONTEXT)
        {
            current_context[call->mTid] = UNBOUND;
            context_index = UNBOUND;
        }
        else
        {
            context_index = context_remapping.at(context);
            current_context[call->mTid] = context_index;
        }

        if (surface == (int64_t)EGL_NO_SURFACE)
        {
            current_surface[call->mTid] = UNBOUND;
            surface_index = UNBOUND;
        }
        else
        {
            surface_index = surface_remapping.at(surface);
            current_surface[call->mTid] = surface_index;

        }

        // "The first time a OpenGL or OpenGL ES context is made current the viewport
        // and scissor dimensions are set to the size of the draw surface" (EGL standard 3.7.3)
        if (surface_index != UNBOUND && context_index != UNBOUND && contexts[context_index].fillstate.scissor.width == -1)
        {
            contexts[context_index].fillstate.scissor.width = surfaces[surface_index].width;
            contexts[context_index].fillstate.scissor.height = surfaces[surface_index].height;
            contexts[context_index].viewport.width = surfaces[surface_index].width;
            contexts[context_index].viewport.height = surfaces[surface_index].height;
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
            const int share_idx = context_remapping.at(share);
            contexts.emplace_back(mret, display, contexts.size(), share, call->mCallNo, frames, &contexts.at(share_idx));
        } else {
            contexts.emplace_back(mret, display, contexts.size(), call->mCallNo, frames);
        }
    }
    else if (call->mCallName == "eglCreateWindowSurface" || call->mCallName =="eglCreateWindowSurface2"
             || call->mCallName == "eglCreatePbufferSurface" || call->mCallName == "eglCreatePixmapSurface")
    {
        int mret = call->mRet.GetAsInt();
        int display = call->mArgs[0]->GetAsInt();
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
        surfaces.push_back(StateTracker::Surface(mret, display, surfaces.size(), call->mCallNo, frames, type, attribs, width, height));
    }
    else if (call->mCallName == "eglDestroySurface")
    {
        // "surface is destroyed when it becomes not current to any thread"
        int surface = call->mArgs[1]->GetAsInt();
        int target_surface_index = surface_remapping.at(surface);
        surfaces[target_surface_index].call_destroyed = call->mCallNo;
        surfaces[target_surface_index].frame_destroyed = frames;
    }
    else if (call->mCallName == "eglDestroyContext")
    {
        // "context is destroyed when it becomes not current to any thread"
        int context = call->mArgs[1]->GetAsInt();
        int target_context_index = context_remapping.at(context);
        // Fix weird error from ThunderAssault, where it calls eglDestroyContext before any context is created
        if (contexts.size() > (unsigned)target_context_index)
        {
            contexts[target_context_index].call_destroyed = call->mCallNo;
            contexts[target_context_index].frame_destroyed = frames;
        }
    }
    else if (call->mCallName.compare(0, 14, "eglSwapBuffers") == 0)
    {
        const int surface = call->mArgs[1]->GetAsInt();
        const int target_surface_index = surface_remapping.at(surface);
        surfaces[target_surface_index].swap_calls.push_back(call->mCallNo);
        if (context_index != UNBOUND && contexts[context_index].render_passes.size() > 0 && contexts[context_index].render_passes.back().active)
        {
            contexts[context_index].render_passes.back().last_call = call->mCallNo;
            contexts[context_index].render_passes.back().active = false;
        }
        frames++;
        for (auto& pair : contexts[context_index].framebuffers[0].attachments)
        {
            pair.second.clears.clear(); // after swap you should probably repeat clears
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
        contexts[context_index].viewport.x = call->mArgs[0]->GetAsInt();
        contexts[context_index].viewport.y = call->mArgs[1]->GetAsInt();
        contexts[context_index].viewport.width = call->mArgs[2]->GetAsInt();
        contexts[context_index].viewport.height = call->mArgs[3]->GetAsInt();
    }
    else if (call->mCallName == "glScissor")
    {
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
            contexts[context_index].vaos.add(id, call->mCallNo, frames);
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
                contexts[context_index].vaos.remove(id, call->mCallNo, frames);
            }
            // "If a vertex array object that is currently bound is deleted, the binding for that object
            // reverts to zero and the default vertex array becomes current."
            if (contexts[context_index].vao_index != UNBOUND
                && contexts[context_index].vaos[contexts[context_index].vao_index].id == id)
            {
                contexts[context_index].vao_index = UNBOUND;
            }
        }
    }
    else if (call->mCallName == "glBindVertexArray" || call->mCallName == "glBindVertexArrayOES")
    {
        const GLuint vao = call->mArgs[0]->GetAsUInt();
        if (vao != 0 && !contexts[context_index].vaos.contains(vao))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].vaos.add(vao, call->mCallNo, frames);
        }
        if (vao != 0)
        {
            contexts[context_index].vao_index = contexts[context_index].vaos.remap(vao);
        }
        else
        {
            contexts[context_index].vao_index = UNBOUND;
        }
    }
    else if (call->mCallName == "glIsEnabled")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        bool retval = call->mRet.GetAsInt();
        assert(retval == contexts[context_index].enabled[target]); // sanity check
    }
    else if (call->mCallName == "glEnable")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        contexts[context_index].enabled[target] = true;
    }
    else if (call->mCallName == "glDisable")
    {
        GLenum target = call->mArgs[0]->GetAsInt();
        contexts[context_index].enabled[target] = false;
    }
    else if (call->mCallName == "glEnableVertexAttribArray")
    {
        const GLuint index = call->mArgs[0]->GetAsUInt();
        contexts[context_index].vao_enabled.insert(index);
    }
    else if (call->mCallName == "glDisableVertexAttribArray")
    {
        const GLuint index = call->mArgs[0]->GetAsUInt();
        contexts[context_index].vao_enabled.erase(index);
    }
    else if (call->mCallName == "glGenFramebuffers" || call->mCallName == "glGenFramebuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].framebuffers.add(id, call->mCallNo, frames);
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
                contexts[context_index].framebuffers.remove(id, call->mCallNo, frames);
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
        GLenum target = call->mArgs[0]->GetAsUInt();
        GLuint fb = call->mArgs[1]->GetAsUInt();
        if (fb != 0 && !contexts[context_index].framebuffers.contains(fb))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].framebuffers.add(fb, call->mCallNo, frames);
        }
        if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
        {
            contexts[context_index].prev_drawframebuffer = contexts[context_index].drawframebuffer;
            contexts[context_index].drawframebuffer = fb;
        }
        if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER)
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
            texture_index = contexts[context_index].textures.remap(texture);
        }
        else
        {
            texture = call->mArgs[2]->GetAsUInt();
            texture_index = contexts[context_index].textures.remap(texture);
            textarget = contexts[context_index].textures[texture_index].binding_point;
        }

        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
        {
            contexts[context_index].framebuffers[target_fb_index].attachments[GL_STENCIL_ATTACHMENT] = StateTracker::Attachment(texture, textarget, texture_index);
            contexts[context_index].framebuffers[target_fb_index].attachments[GL_DEPTH_ATTACHMENT] = StateTracker::Attachment(texture, textarget, texture_index);
        }
        else
        {
            contexts[context_index].framebuffers[target_fb_index].attachments[attachment] = StateTracker::Attachment(texture, textarget, texture_index);
        }
        contexts[context_index].framebuffers[target_fb_index].attachment_calls++;
    }
    else if (call->mCallName == "glGenerateMipmap" || call->mCallName == "glGenerateMipmapOES")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        const int target_texture_index = contexts[context_index].textures.remap(tex_id);
        contexts[context_index].mipmaps.push_back({ call->mCallNo, frames, target_texture_index });
    }
    else if (call->mCallName == "glGenRenderbuffers" || call->mCallName == "glGenRenderbuffersOES")
    {
        const unsigned count = call->mArgs[0]->GetAsUInt();
        assert(call->mArgs[1]->IsArray());
        assert(call->mArgs[1]->mArrayLen == count);
        for (unsigned i = 0; i < count; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].renderbuffers.add(id, call->mCallNo, frames);
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
                contexts[context_index].renderbuffers.remove(id, call->mCallNo, frames);
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
        const int rb_index = contexts[context_index].renderbuffers.remap(renderbuffer);
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
        {
            contexts[context_index].framebuffers[target_fb_index].attachments[GL_STENCIL_ATTACHMENT] = StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index);
            contexts[context_index].framebuffers[target_fb_index].attachments[GL_DEPTH_ATTACHMENT] = StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index);
        }
        else
        {
            contexts[context_index].framebuffers[target_fb_index].attachments[attachment] = StateTracker::Attachment(renderbuffer, renderbuffertarget, rb_index);
        }
        contexts[context_index].framebuffers[target_fb_index].attachment_calls++;
    }
    else if (call->mCallName == "glBindRenderbuffer" || call->mCallName == "glBindRenderbufferOES")
    {
        GLenum target = call->mArgs[0]->GetAsUInt();
        assert(target == GL_RENDERBUFFER);
        GLuint rb = call->mArgs[1]->GetAsUInt();
        if (rb != 0 && !contexts[context_index].renderbuffers.contains(rb))
        {
            // It is legal to create renderbuffers with a call to this function.
            contexts[context_index].renderbuffers.add(rb, call->mCallNo, frames);
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
            contexts[context_index].samplers.add(id, call->mCallNo, frames);
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
                contexts[context_index].samplers.remove(id, call->mCallNo, frames);
            }
        }
    }
    else if (call->mCallName == "glBindSampler")
    {
        // The usual rule that you can create objects with glBind*() calls apparently does not apply to to glBindSampler()
        const GLuint unit = call->mArgs[0]->GetAsUInt();
        contexts[context_index].sampler_binding[unit] = call->mArgs[1]->GetAsUInt();
    }
    else if (call->mCallName == "glGenQueries" || call->mCallName == "glGenQueriesEXT")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].queries.add(id, call->mCallNo, frames);
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
                contexts[context_index].queries.remove(id, call->mCallNo, frames);
            }
        }
    }
    else if (call->mCallName == "glBeginQuery" || call->mCallName == "glBeginQueryEXT")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = call->mArgs[1]->GetAsUInt();
        contexts[context_index].query_binding[target] = id;
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
            contexts[context_index].transform_feedbacks.add(id, call->mCallNo, frames);
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
                contexts[context_index].transform_feedbacks.remove(id, call->mCallNo, frames);
            }
        }
    }
    else if (call->mCallName == "glBindTransformFeedback")
    {
        assert(call->mArgs[0]->GetAsUInt() == GL_TRANSFORM_FEEDBACK);
        GLuint id = call->mArgs[1]->GetAsUInt();
        if (id != 0 && !contexts[context_index].transform_feedbacks.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].transform_feedbacks.add(id, call->mCallNo, frames);
        }
        contexts[context_index].transform_feedback_binding = id;
    }
#if 0 // this was not working as it should; apparently non-object transform feedback is legal in GLES
    else if (call->mCallName == "glBeginTransformFeedback")
    {
        const GLuint id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].primitiveMode = call->mArgs[0]->GetAsUInt();
    }
    else if (call->mCallName == "glEndTransformFeedback")
    {
        const int id = contexts[context_index].transform_feedback_binding;
        const int index = contexts[context_index].transform_feedbacks.remap(id);
        contexts[context_index].transform_feedbacks[index].primitiveMode = GL_NONE;
    }
#endif
    else if (call->mCallName == "glGenBuffers")
    {
        GLuint count = call->mArgs[0]->GetAsInt();
        assert(call->mArgs[1]->IsArray());
        assert(count == call->mArgs[1]->mArrayLen);
        for (unsigned i = 0; i < call->mArgs[1]->mArrayLen; i++)
        {
            const unsigned id = call->mArgs[1]->mArray[i].GetAsUInt();
            contexts[context_index].buffers.add(id, call->mCallNo, frames);
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
                contexts[context_index].buffers.remove(id, call->mCallNo, frames);
            }
        }
        // TBD: "When a buffer, texture, or renderbuffer object is deleted, it is unbound from any
        // bind points it is bound to in the current context, and detached from any attachments
        // of container objects that are bound to the current context"
    }
    else if (call->mCallName == "glBufferData")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = contexts[context_index].boundBufferIds[target][0].buffer;
        const int index = contexts[context_index].buffers.remap(id);
        //const GLsizeiptr size = call->mArgs[1]->GetAsUInt();
        StateTracker::Buffer &buffer = contexts[context_index].buffers[index];
        buffer.usages.insert(call->mArgs[3]->GetAsUInt());
        //call->mArgs[2]->GetAsBlob() contains the buffer itself
    }
    else if (call->mCallName == "glBindBuffer")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint id = call->mArgs[1]->GetAsUInt();
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id, call->mCallNo, frames);
        }
        contexts[context_index].boundBufferIds[target][0] = { id, 0, 0 };
    }
    else if (call->mCallName == "glBindBufferBase")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint index = call->mArgs[1]->GetAsUInt();
        const GLuint id = call->mArgs[2]->GetAsUInt();
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id, call->mCallNo, frames);
        }
        contexts[context_index].boundBufferIds[target][index] = { id, 0, 0 };
    }
    else if (call->mCallName == "glBindBufferRange")
    {
        const GLenum target = call->mArgs[0]->GetAsUInt();
        const GLuint index = call->mArgs[1]->GetAsUInt();
        const GLuint id = call->mArgs[2]->GetAsUInt();
        const GLintptr offset = call->mArgs[3]->GetAsUInt();
        const GLsizeiptr size = call->mArgs[4]->GetAsUInt();
        if (id != 0 && !contexts[context_index].buffers.contains(id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].buffers.add(id, call->mCallNo, frames);
        }
        contexts[context_index].boundBufferIds[target][index] = { id, offset, size };
    }
    else if (call->mCallName == "glVertexAttribPointer" || call->mCallName == "glVertexAttribIPointer")
    {
        const GLuint id = contexts[context_index].boundBufferIds[GL_ARRAY_BUFFER][0].buffer;
        if (id != 0)
        {
            if (contexts[context_index].buffers.contains(id))
            {
                const int buffer_index = contexts[context_index].buffers.remap(id);
                const GLenum type = call->mArgs[2]->GetAsUInt();
                const GLint size = call->mArgs[1]->GetAsInt();
                const int stride_idx = (call->mCallName == "glVertexAttribPointer") ? 4 : 3;
                const GLsizei stride = call->mArgs[stride_idx]->GetAsInt();
                contexts[context_index].buffers[buffer_index].types.insert(std::make_tuple(type, size, stride));
            }
            else // should never get here, as it should have been caught on bind
            {
                DBG_LOG("API ERROR [%d]: %s specifies a bad target %u\n", (int)call->mCallNo, call->mCallName.c_str(), id);
            }
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
            contexts[context_index].textures.add(id, call->mCallNo, frames);
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
                const int target_texture_index = contexts[context_index].textures.remap(id);
                if (contexts[context_index].textures.ssize() > target_texture_index) // egypt again
                {
                    contexts[context_index].textures[target_texture_index].frame_destroyed = frames;
                    contexts[context_index].textures[target_texture_index].call_destroyed = call->mCallNo;
                }
                else
                {
                    DBG_LOG("API ERROR [%d]: Deleting non-existent texture ID %u (idx %d) from context %d\n", (int)call->mCallNo, id, target_texture_index, context_index);
                }
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
             || call->mCallName == "glCompressedTexImage3D" || call->mCallName == "glCompressedTexImage2D"
             || call->mCallName == "glCompressedTexImage1D")
    {
        const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
        const GLuint unit = contexts[context_index].activeTextureUnit;
        const GLuint tex_id = contexts[context_index].textureUnits[unit][target];
        if (contexts[context_index].textures.contains(tex_id))
        {
            const int target_texture_index = contexts[context_index].textures.remap(tex_id);
            StateTracker::Texture& tex = contexts[context_index].textures[target_texture_index];
            assert(!tex.immutable);
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
        }
        else
        {
            DBG_LOG("API ERROR [%d]: Cannot find texture id %u for %s on context %d\n", (int)call->mCallNo, tex_id, call->mCallName.c_str(), context_index);
        }
    }
    else if (call->mCallName == "glTexStorage2DMultisample")
    {
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
    }
    else if (call->mCallName == "glBindTexture")
    {
        const unsigned unit = contexts[context_index].activeTextureUnit;
        const unsigned target = call->mArgs[0]->GetAsUInt();
        const unsigned tex_id = call->mArgs[1]->GetAsUInt();
        contexts[context_index].textureUnits[unit][target] = tex_id;
        if (tex_id != 0 && !contexts[context_index].textures.contains(tex_id))
        {
            // It is legal to create objects with a call to this function.
            contexts[context_index].textures.add(tex_id, call->mCallNo, frames);
        }
    }
    else if (call->mCallName == "glActiveTexture")
    {
        const GLuint unit = call->mArgs[0]->GetAsUInt() - GL_TEXTURE0;
        contexts[context_index].activeTextureUnit = unit;
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
        const int texture_index = contexts[context_index].textures.remap(texture_id);
        adjust_sampler_state(call, pname, contexts[context_index].textures[texture_index].state, call->mArgs[2]);
    }
    else if (call->mCallName == "glSamplerParameterf" || call->mCallName == "glSamplerParameteri" || call->mCallName == "glSamplerParameterfv" || call->mCallName == "glSamplerParameteriv")
    {
        const GLenum sampler = call->mArgs[0]->GetAsUInt();
        if (!contexts[context_index].samplers.contains(sampler))
        {
            DBG_LOG("API ERROR [%d]: Invalid sampler id=%u\n", (int)call->mCallNo, sampler);
            return;
        }
        const GLenum pname = call->mArgs[1]->GetAsUInt();
        const GLuint sampler_idx = contexts[context_index].samplers.remap(sampler);
        adjust_sampler_state(call, pname, contexts[context_index].samplers[sampler_idx].state, call->mArgs[2]);
    }
    else if (call->mCallName == "glCreateProgram")
    {
        GLuint id = call->mRet.GetAsUInt();
        contexts[context_index].programs.add(id, call->mCallNo, frames);
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
            }
            else
            {
                contexts[context_index].program_index = contexts[context_index].programs.remap(id);
            }
        }
        else
        {
            contexts[context_index].program_index = UNBOUND;
        }
    }
    else if (call->mCallName == "glDeleteProgram")
    {
        GLuint id = call->mArgs[0]->GetAsUInt();
        // "DeleteProgram will silently ignore the value zero". Also, some content assumes it will also
        // ignore invalid values, even though the standard does not guarantee this.
        if (id != 0)
        {
            if (contexts[context_index].programs.contains(id))
            {
                contexts[context_index].programs.remove(id, call->mCallNo, frames);
            }
            else
            {
                DBG_LOG("API ERROR [%d]: Deleting non-existent program ID %u in context %d\n", (int)call->mCallNo, id, context_index);
            }
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
        StateTracker::Shader& s = contexts[context_index].shaders.add(id, call->mCallNo, frames);
        s.shader_type = type;
    }
    else if (call->mCallName == "glDeleteShader")
    {
        GLuint id = call->mArgs[0]->GetAsUInt();
        // "DeleteShader will silently ignore the value zero". Also, some content assumes it will also
        // ignore invalid values, even though the standard does not guarantee this.
        if (id != 0)
        {
            if (contexts[context_index].shaders.contains(id))
            {
                contexts[context_index].shaders.remove(id, call->mCallNo, frames);
            }
            else
            {
                DBG_LOG("API ERROR [%d]: Deleting non-existent shader ID %u in context %d\n", (int)call->mCallNo, id, context_index);
            }
        }
    }
    else if (call->mCallName == "glLinkProgram")
    {
        GLuint program = call->mArgs[0]->GetAsUInt();
        int target_program_index = contexts[context_index].programs.remap(program);
        std::string code;
        StateTracker::Program& p = contexts[context_index].programs[target_program_index];
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
        s.source_compressed = parser.compressed(sh);
        GLSLRepresentation repr = parser.parse(sh);
        s.contains_invariants = repr.contains_invariants;
        s.contains_optimize_off_pragma = sh.contains_optimize_off_pragma;
        s.contains_debug_on_pragma = sh.contains_debug_on_pragma;
        s.contains_invariant_all_pragma = sh.contains_invariant_all_pragma;
        for (const auto& var : repr.global.members)
        {
            if (lookup_is_sampler(var.type))
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
        std::string name = call->mArgs[1]->GetAsString();
        int target_program_index = contexts[context_index].programs.remap(program);
        contexts[context_index].programs[target_program_index].uniformNames[location] = name;
        contexts[context_index].programs[target_program_index].uniformLocations[name] = location;
    }
    // Standard: "Sampler values must be set by calling Uniform1i{v}". That's why we only save those.
    else if (call->mCallName == "glUniform1i")
    {
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
            }
        }
    }
    else if (call->mCallName == "glUniform1iv")
    {
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
                }
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
            }
        }
    }
    // glProgramUniform1i and glProgramUniform1iv also apply because they are functional mirrors of the above.
    else if (call->mCallName == "glProgramUniform1i")
    {
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
            }
        }
    }
    else if (call->mCallName == "glProgramUniform1iv")
    {
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
                }
                contexts[context_index].programs[program_index].uniformLastChanged[location] = call->mCallNo;
            }
        }
    }
    else if (call->mCallName.find("glUniform") != std::string::npos || call->mCallName.find("glProgramUniform") != std::string::npos) // other uniforms
    {
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
            contexts[context_index].program_pipelines.add(id, call->mCallNo, frames);
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
                contexts[context_index].program_pipelines.remove(id, call->mCallNo, frames);
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
        GLuint id = call->mArgs[0]->GetAsUInt();
        if (id != 0)
        {
            if (!contexts[context_index].program_pipelines.contains(id)) // then create it
            {
                const auto& p = contexts[context_index].program_pipelines.add(id, call->mCallNo, frames);
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
        //const GLsizei length = call->mArgs[2]->GetAsInt();
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
        GLenum pname = call->mArgs[0]->GetAsUInt();
        if (pname == GL_PATCH_VERTICES)
        {
            contexts[context_index].patchSize = call->mArgs[1]->GetAsInt();
        }
    }
    else if (call->mCallName == "glDepthMask")
    {
        contexts[context_index].fillstate.depthmask = call->mArgs[0]->GetAsUInt();
    }
    else if (call->mCallName == "glDepthFunc")
    {
        contexts[context_index].fillstate.depthfunc = call->mArgs[0]->GetAsUInt();
    }
    else if (call->mCallName == "glStencilMask")
    {
        const GLuint mask = call->mArgs[0]->GetAsUInt();
        contexts[context_index].fillstate.stencilmask[GL_FRONT] = mask;
        contexts[context_index].fillstate.stencilmask[GL_BACK] = mask;
    }
    else if (call->mCallName == "glStencilMaskSeparate")
    {
        const GLenum face = call->mArgs[0]->GetAsUInt();
        const GLuint mask = call->mArgs[1]->GetAsUInt();
        if (face == GL_FRONT_AND_BACK)
        {
            contexts[context_index].fillstate.stencilmask[GL_FRONT] = mask;
            contexts[context_index].fillstate.stencilmask[GL_BACK] = mask;
        }
        else
        {
            contexts[context_index].fillstate.stencilmask[face] = mask;
        }
    }
    else if (call->mCallName == "glColorMask")
    {
        contexts[context_index].fillstate.colormask[0] = call->mArgs[0]->GetAsUInt();
        contexts[context_index].fillstate.colormask[1] = call->mArgs[1]->GetAsUInt();
        contexts[context_index].fillstate.colormask[2] = call->mArgs[2]->GetAsUInt();
        contexts[context_index].fillstate.colormask[3] = call->mArgs[3]->GetAsUInt();
    }
    else if (call->mCallName == "glClearColor")
    {
        contexts[context_index].fillstate.clearcolor[0] = call->mArgs[0]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[1] = call->mArgs[1]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[2] = call->mArgs[2]->GetAsFloat();
        contexts[context_index].fillstate.clearcolor[3] = call->mArgs[3]->GetAsFloat();
    }
    else if (call->mCallName == "glClearStencil")
    {
        contexts[context_index].fillstate.clearstencil = call->mArgs[0]->GetAsInt();
    }
    else if (call->mCallName == "glClearDepthf")
    {
        contexts[context_index].fillstate.cleardepth = call->mArgs[0]->GetAsFloat();
    }
    else if (call->mCallName.compare(0, 10, "glDispatch") == 0)
    {
        const int current_program = contexts[context_index].program_index;
        contexts[context_index].programs[current_program].stats.dispatches++;
        contexts[context_index].programs[current_program].stats_per_frame[frames].dispatches++;
        if (contexts[context_index].render_passes.size() == 0 || !contexts[context_index].render_passes.back().active)
        {
            contexts[context_index].render_passes.push_back(StateTracker::RenderPass(contexts[context_index].renderpasses_per_frame[frames], call->mCallNo, frames));
            contexts[context_index].renderpasses_per_frame[frames]++;
        }
        StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
        rp.used_programs.insert(contexts[context_index].program_index);
    }
    else if (call->mCallName == "glClear")
    {
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
           }
       }
       if ((mask & GL_STENCIL_BUFFER_BIT) && (contexts[context_index].fillstate.stencilmask[GL_BACK] || contexts[context_index].fillstate.stencilmask[GL_FRONT]))
       {
           const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
           StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
           if (fbo.attachments.count(GL_STENCIL_ATTACHMENT) > 0)
           {
               StateTracker::Attachment& at = fbo.attachments.at(GL_STENCIL_ATTACHMENT);
               contexts[context_index].fillstate.call_stored = call->mCallNo - 1;
               find_duplicate_clears(contexts[context_index].fillstate, at, GL_STENCIL, fbo, call->mCallName);
               at.clears.push_back(contexts[context_index].fillstate);
           }
       }
    }
    else if (call->mCallName == "glClearBufferfi")
    {
       GLenum buffertype = call->mArgs[0]->GetAsUInt();
       assert(buffertype == GL_DEPTH_STENCIL);
       GLint drawbuffer = call->mArgs[1]->GetAsInt();
       assert(drawbuffer == 0);
       const int index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
       StateTracker::Framebuffer& fbo = contexts[context_index].framebuffers.at(index);
       StateTracker::Attachment& stencil = fbo.attachments.at(GL_STENCIL_ATTACHMENT);
       StateTracker::Attachment& depth = fbo.attachments.at(GL_DEPTH_ATTACHMENT);
       StateTracker::FillState fillstate = contexts[context_index].fillstate;
       fillstate.call_stored = call->mCallNo - 1;
       fillstate.cleardepth = call->mArgs[2]->GetAsFloat();
       fillstate.clearstencil = call->mArgs[3]->GetAsInt();
       find_duplicate_clears(fillstate, stencil, GL_STENCIL, fbo, call->mCallName);
       stencil.clears.push_back(fillstate);
       find_duplicate_clears(fillstate, depth, GL_DEPTH, fbo, call->mCallName);
       depth.clears.push_back(fillstate);
    }
    else if (call->mCallName.compare(0, 13, "glClearBuffer") == 0) // except glClearBufferfi which is handled above
    {
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
       StateTracker::Attachment& at = fbo.attachments.at(attachment);
       StateTracker::FillState fillstate = contexts[context_index].fillstate;
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
    else if (call->mCallName.compare(0, 6, "glDraw") == 0 && call->mCallName != "glDrawBuffers")
    {
        contexts[context_index].draw_calls_per_frame[frames]++;
        if (contexts[context_index].program_index == UNBOUND && contexts[context_index].program_pipeline_index == UNBOUND)
        {
            return; // valid for GLES1 content!
        }
        if (contexts[context_index].render_passes.size() == 0 || !contexts[context_index].render_passes.back().active
            || contexts[context_index].render_passes.back().drawframebuffer != contexts[context_index].drawframebuffer)
        {
            contexts[context_index].render_passes.push_back(StateTracker::RenderPass(contexts[context_index].renderpasses_per_frame[frames], call->mCallNo, frames));
            contexts[context_index].renderpasses_per_frame[frames]++;
        }
        StateTracker::RenderPass &rp = contexts[context_index].render_passes.back();
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
        for (const auto pair : program_stages)
        {
            rp.used_programs.insert(pair.second);
        }
        const int fb_index = contexts[context_index].framebuffers.remap(contexts[context_index].drawframebuffer);
        contexts[context_index].framebuffers[fb_index].used++;
        contexts[context_index].framebuffers[fb_index].last_used = frames;
        for (auto& rb : contexts[context_index].framebuffers[fb_index].attachments)
        {
            rb.second.clears.clear();
            // TBD - don't add a target as a target if the write mask for it is off
            if (rb.second.id > 0 && rb.second.type == GL_RENDERBUFFER && contexts[context_index].renderbuffers.contains(rb.second.id))
            {
                const int index = contexts[context_index].renderbuffers.remap(rb.second.id);
                rp.used_renderbuffers.insert(index);
                rp.render_targets[rb.first].first = GL_RENDERBUFFER;
                rp.render_targets[rb.first].second = index;
            }
            else if (rb.second.id > 0 && contexts[context_index].textures.contains(rb.second.id))
            {
                const int index = contexts[context_index].textures.remap(rb.second.id);
                rp.used_texture_targets.insert(index);
                rp.render_targets[rb.first].first = GL_TEXTURE;
                rp.render_targets[rb.first].second = index;
            }
        }
        rp.draw_calls++;
        DrawParams params = getDrawCallCount(call);
        // TBD the below does not take into account primitive restart
        for (const auto pair : program_stages)
        {
            const int program_index = pair.second;
            contexts[context_index].programs[program_index].stats.drawcalls++;
            contexts[context_index].programs[program_index].stats.vertices += params.vertices;
            contexts[context_index].programs[program_index].stats.primitives += params.primitives;
            contexts[context_index].programs[program_index].stats_per_frame[frames].drawcalls++;
            contexts[context_index].programs[program_index].stats_per_frame[frames].vertices += params.vertices;
            contexts[context_index].programs[program_index].stats_per_frame[frames].primitives += params.primitives;
        }
        rp.vertices += params.vertices;
        rp.primitives += params.primitives;
    }
}

void ParseInterface::close()
{
    inputFile.Close();
    outputFile.Close();
}
