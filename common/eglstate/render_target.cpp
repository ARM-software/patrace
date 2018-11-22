#include "render_target.hpp"
#include "common.hpp"
#include "image/image.hpp"
#include "image/image_io.hpp"

#include <cstdio>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace pat
{

/////////////////////////////////////////////////////////////////
// class RenderTarget
/////////////////////////////////////////////////////////////////

bool RenderTarget::UsedAsRenderTarget() const
{
    return _usedAsRenderTarget;
}

void RenderTarget::BoundToFramebufferObject()
{
    _usedAsRenderTarget = true;
}

/////////////////////////////////////////////////////////////////
// class TextureObject
/////////////////////////////////////////////////////////////////

const std::string TextureObject::LevelKey::AsString() const
{
    char buffer[128];
    sprintf(buffer, "%s_%d", EnumString(target), level);
    return buffer;
}

TextureObject::TextureObject()
: _haveSetSubImage(false), _haveGeneratedMipmap(false)
{
    _parameters[GL_TEXTURE_MIN_FILTER] = GL_NEAREST_MIPMAP_LINEAR;
    _parameters[GL_TEXTURE_MAG_FILTER] = GL_LINEAR;
    _parameters[GL_TEXTURE_WRAP_S] = GL_REPEAT;
    _parameters[GL_TEXTURE_WRAP_T] = GL_REPEAT;
}

TextureObject::~TextureObject()
{
    _parameters.clear();
    _levels.clear();
}

bool TextureObject::HaveSetSubImage() const
{
    return _haveSetSubImage;
}

bool TextureObject::HaveGeneratedMipmap() const
{
    return _haveGeneratedMipmap;
}

unsigned int TextureObject::GetParameter(unsigned int pname) const
{
    ParameterMapType::const_iterator iter = _parameters.find(pname);
    assert(iter != _parameters.end());
    return iter->second;
}

void TextureObject::SetParameter(unsigned int pname, unsigned int param)
{
    _parameters[pname] = param;
}

void TextureObject::SetImage(unsigned int target, unsigned int level, unsigned int format, unsigned int type, unsigned int width, unsigned int height, const void *data)
{
    ImagePtr image(new Image(width, height, format, type, GetImageDataSize(width, height, format, type), (unsigned char *)data));
    SetLevel(target, level, image);
}

void TextureObject::SetSubImage(unsigned int target, unsigned int level, unsigned int format, unsigned int type, unsigned int xoffset, unsigned int yoffset, unsigned int width, unsigned int height, const void *data)
{
    _haveSetSubImage = true;

    ImagePtr l = GetLevel(target, level);

    if (!l)
    {
        // in case, the app calls glTexSubImage2d directly without calling the glTexImage2D firstly.
        assert(xoffset == 0 && yoffset == 0);
        l = ImagePtr(new Image(width, height, format, type, GetImageDataSize(width, height, format, type), (unsigned char *)data));
        SetLevel(target, level, l);
    }

    if (l->Format() == format and l->Type() == type)
        l->SetSubData(xoffset, yoffset, width, height, (unsigned char *)data);
}

void TextureObject::SetCompressedImage(unsigned int target, unsigned int level, unsigned int format, unsigned int width, unsigned int height, const void *data, unsigned int dataSize)
{
    ImagePtr image(new Image(width, height, format, GL_NONE, dataSize, (unsigned char *)data));
    SetLevel(target, level, image);
}

void TextureObject::GenerateMipmap()
{
    _haveGeneratedMipmap = true;
}

void TextureObject::SetLevel(unsigned int target, unsigned int level, ImagePtr tl)
{
    _levels[LevelKey(target, level)] = tl;
}

const ImagePtr TextureObject::GetLevel(unsigned int target, unsigned int level) const
{
    LevelMapType::const_iterator citer = _levels.find(LevelKey(target, level));
    if (citer == _levels.end())
        return ImagePtr();
    else
        return citer->second;
}

ImagePtr TextureObject::GetLevel(unsigned int target, unsigned int level)
{
    LevelMapType::iterator iter = _levels.find(LevelKey(target, level));
    if (iter == _levels.end())
        return ImagePtr();
    else
        return iter->second;
}

void TextureObject::Dump(const ObjectID &id, std::ostream &of) const
{
    const UInt32 handle = id.handle;
    const UInt32 callNum = id.callNumber;
    char buffer[256];
    sprintf(buffer, "TextureObject%04d_Call%08d_", handle, callNum);
    const std::string prefix(buffer);

    LevelMapType::const_iterator citer = _levels.begin();
    for (; citer != _levels.end(); ++citer)
    {
        const std::string filename = prefix + citer->first.AsString() + ".ktx";
        const ImagePtr image = citer->second;
        WriteKTX(*image, filename.c_str(), false);

        const UInt32 width = image->Width();
        const UInt32 height = image->Height();
        const UInt32 format = image->Format();
        const UInt32 type = image->Type();
        const UInt32 dataSize = image->DataSize();
        of << handle << "," << callNum << ","
           << width << "," << height << "," << EnumString(format) << ","
           << EnumString(type) << "," << dataSize << std::endl;
    }
}

/////////////////////////////////////////////////////////////////
// class RenderbufferObject
/////////////////////////////////////////////////////////////////

unsigned int RenderbufferObject::NextID = 0;

RenderbufferObject::RenderbufferObject()
: _id(NextID++), _format(GL_NONE), _width(0), _height(0)
{
}

unsigned int RenderbufferObject::type() const
{
    return GL_RENDERBUFFER;
}

const std::string RenderbufferObject::IDString() const
{
    char buffer[32];
    sprintf(buffer, "RenderbufferObject[%d]", _id);
    return buffer;
}

} // namespace state
