#ifndef _STATE_RENDER_TARGET_HPP_
#define _STATE_RENDER_TARGET_HPP_

#include <vector>
#include <string>
#include <map>

#include "image/image.hpp"
#include "common.hpp"

namespace pat
{

class RenderTarget
{
public:
    RenderTarget() : _usedAsRenderTarget(false) {}
    virtual ~RenderTarget() {}

    bool UsedAsRenderTarget() const;
    void BoundToFramebufferObject();

private:
    bool _usedAsRenderTarget;
};

class TextureObject : public RenderTarget
{
public:
    TextureObject();
    virtual ~TextureObject();

    bool HaveSetSubImage() const;
    bool HaveGeneratedMipmap() const;

    unsigned int GetParameter(unsigned int pname) const;
    void SetParameter(unsigned int pname, unsigned int param);
    void SetImage(unsigned int target, unsigned int level, unsigned int format, unsigned int type, unsigned int width, unsigned int height, const void *data);
    void SetSubImage(unsigned int target, unsigned int level, unsigned int format, unsigned int type, unsigned int xoffset, unsigned int yoffset, unsigned int width, unsigned int height, const void *data);
    void SetCompressedImage(unsigned int target, unsigned int level, unsigned int format, unsigned int width, unsigned int height, const void *data, unsigned int dataSize);
    void GenerateMipmap();

    void SetLevel(unsigned int target, unsigned int level, ImagePtr tl);
    const ImagePtr GetLevel(unsigned int target, unsigned int level) const;
    ImagePtr GetLevel(unsigned int target, unsigned int level);

    // The object ID of this texture object and output stream of the csv file
    void Dump(const ObjectID &id, std::ostream &of) const;

private:
    typedef std::map<unsigned int, unsigned int> ParameterMapType;
    ParameterMapType _parameters;

    struct LevelKey
    {
        LevelKey(unsigned int t, unsigned int l)
        : target(t), level(l)
        {
        }
        bool operator <(const LevelKey &rhs) const
        {
            if (target < rhs.target)
                return true;
            else if (target == rhs.target && level < rhs.level)
                return true;
            else 
                return false;
        }
        const std::string AsString() const;
        unsigned int target;
        unsigned int level;
    };
    typedef std::map<LevelKey, ImagePtr> LevelMapType;
    LevelMapType _levels;

    bool _haveSetSubImage;
    bool _haveGeneratedMipmap;
};
typedef std::shared_ptr<TextureObject> TextureObjectPtr;

class RenderbufferObject : public RenderTarget
{
public:
    RenderbufferObject();
    
    virtual const std::string IDString() const;
    virtual unsigned int type() const;

    void SetFormat(unsigned int format) { _format = format; }
    void SetWidth(unsigned int width) { _width = width; }
    void SetHeight(unsigned int height) { _height = height; }
    void SetType(unsigned int) {}

    unsigned int Format() const { return _format; }
    unsigned int Width() const { return _width; }
    unsigned int Height() const { return _height; }

private:
    static unsigned int NextID;
    unsigned int _id;

    unsigned int _format, _width, _height;
};


}

#endif // _STATE_RENDER_TARGET_HPP_
