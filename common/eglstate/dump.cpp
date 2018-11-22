#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <fstream>

//#include "common/json.hpp"
#include "image/image.hpp"
#include "image/image_io.hpp"
#include "dump.hpp"

#include "state_manager.hpp"
#include "fbo.hpp"
#include "render_target.hpp"
#include "vbo.hpp"
using namespace state;

#include "frame_recorder.hpp"
#include "frame.hpp"
using namespace record;

namespace dump
{

static void DumpFramebufferObject(JSONWriter &json, const state::FramebufferObject *obj)
{
    json.beginObject();
    json.writeStringMember("ID", obj->IDString().c_str());

    const state::RenderTarget *colorAttachment = obj->BoundRenderTarget(GL_COLOR_ATTACHMENT0);
    if (colorAttachment)
        json.writeStringMember("ColorAttachment", colorAttachment->IDString().c_str());

    const state::RenderTarget *depthAttachment = obj->BoundRenderTarget(GL_DEPTH_ATTACHMENT);
    if (depthAttachment)
        json.writeStringMember("DepthAttachment", depthAttachment->IDString().c_str());

    const state::RenderTarget *stencilAttachment = obj->BoundRenderTarget(GL_STENCIL_ATTACHMENT);
    if (stencilAttachment)
        json.writeStringMember("StencilAttachment", stencilAttachment->IDString().c_str());

    json.endObject();
}

static void DumpTextureLevel(JSONWriter &json, const Image *obj, const std::string &filename)
{
    json.beginObject();
    json.writeStringMember("Format", EnumString(obj->Format()));
    json.writeStringMember("Type", EnumString(obj->Type()));
    json.writeNumberMember("Width", obj->Width());
    json.writeNumberMember("Height", obj->Height());

    if (obj->Data())
    {
        //image::WritePNG(*obj, filename.c_str(), false);
        json.writeStringMember("Data", filename.c_str());
    }

    json.endObject();
}

static void DumpTextureObject(JSONWriter &json, const TextureObject *obj)
{
    json.beginObject();

    json.writeStringMember("ID", obj->IDString().c_str());
    json.writeStringMember("Type", EnumString(obj->GetType()));

    const unsigned int PARAMETER_LIST[] = {
        GL_TEXTURE_MIN_FILTER,
        GL_TEXTURE_MAG_FILTER,
        GL_TEXTURE_WRAP_S,
        GL_TEXTURE_WRAP_T,
    };
    const unsigned int PARAMETER_LIST_SIZE = sizeof(PARAMETER_LIST) / sizeof(unsigned int);

    json.beginMember("Parameters");
    json.beginObject();
    for (unsigned int p = 0; p < PARAMETER_LIST_SIZE; ++p)
    {
        const unsigned int para = PARAMETER_LIST[p];
        json.writeStringMember(EnumString(para), EnumString(obj->GetParameter(para)));
    }
    json.endObject();
    json.endMember();

    const unsigned int TARGET_LIST[] = {
        GL_TEXTURE_2D,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };
    const unsigned int TARGET_LIST_SIZE = sizeof(TARGET_LIST) / sizeof(unsigned int);
    json.beginMember("Levels");
    json.beginObject();
    for (unsigned int t = 0; t < TARGET_LIST_SIZE; ++t)
    {
        const unsigned int target = TARGET_LIST[t];
        unsigned int level = 0;
        while (1)
        {
            const Image* tl = obj->GetLevel(target, level);
            if (!tl) break;

            char buffer[128];
            sprintf(buffer, "%s[%d]", EnumString(target), level);
            json.beginMember(buffer);

            char filename[128];
            sprintf(filename, "texture_%d_%s_%d.png",
                obj->ID(), EnumString(target), level);
            DumpTextureLevel(json, tl, filename);

            json.endMember();

            ++level;
        }
    }
    json.endObject();
    json.endMember();

    json.endObject();
    json.endMember();

}

static void DumpRenderbufferObject(JSONWriter &json, const RenderbufferObject *obj)
{
    json.beginObject();

    json.writeStringMember("ID", obj->IDString().c_str());
    json.writeStringMember("Format", EnumString(obj->Format()));
    json.writeNumberMember("Width", obj->Width());
    json.writeNumberMember("Height", obj->Height());

    json.endObject();
}

static void DumpFBOJob(JSONWriter &json, const FBOJob *obj)
{
    json.beginObject();
    json.writeStringMember("BoundFBO", obj->BoundFramebufferObject()->IDString().c_str());
    const Cube2D viewport = obj->GetViewport();
    json.writeStringMember("Viewport", viewport.asString().c_str());
    json.endObject();
}

static void DumpBufferObject(JSONWriter &json, const BufferObject *obj)
{
    json.beginObject();

    json.writeStringMember("ID", obj->IDString().c_str());
    json.writeStringMember("Type", EnumString(obj->GetType()));
    json.writeStringMember("Usage", EnumString(obj->GetUsage()));
    json.writeNumberMember("Size", obj->GetSize());

    json.endObject();
}

static void DumpFrame(JSONWriter &json, const Frame *obj)
{
    json.beginObject();

    json.writeNumberMember("ID", obj->IDString());
    json.beginMember("FBOJobs");
    json.beginArray();
    for (unsigned int i = 0; i < obj->FBOJobCount(); ++i)
    {
        const FBOJob *job = obj->GetFBOJob(i);
        if (job) DumpFBOJob(json, job);
    }
    json.endArray();
    json.endMember();

    json.endObject();
}

void DumpStates(const char *filename)
{
    std::ofstream of(filename);
    JSONWriter json(of);

    RESOURCE_MANAGEMENT_DUMP(FramebufferObject, framebufferObject)
    RESOURCE_MANAGEMENT_DUMP(TextureObject, textureObject)
    RESOURCE_MANAGEMENT_DUMP(RenderbufferObject, renderbufferObject)
    RESOURCE_MANAGEMENT_DUMP(BufferObject, bufferObject)
    
    json.~JSONWriter();
    of.close();
}

void DumpFrames(const char *filename)
{
    std::ofstream of(filename);
    JSONWriter json(of);

    if (FrameRecorderInstance->FrameCount())
    {
        json.beginMember("Frames");
        json.beginArray();
        for (unsigned int i = 0; i < FrameRecorderInstance->FrameCount(); ++i)
        {
            const Frame *f = FrameRecorderInstance->GetFrame(i);
            if (f) DumpFrame(json, f);
        }
        json.endArray();
        json.endMember();
    }

    json.~JSONWriter();
    of.close();
}

void DumpCurrentState(const char *filename)
{
}

} // namespace dump
