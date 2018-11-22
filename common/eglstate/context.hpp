#ifndef _CONTEXT_HPP_
#define _CONTEXT_HPP_

#include <vector>
#include <string>
#include <map>

#include "common.hpp"
#include "program.hpp"
#include "render_target.hpp"

namespace pat
{

class Context
{
public:
    Context();
    ~Context();

    void Initialize();
    void Uninitialize();

    void SetCurrentCallNumber(UInt32 cn);
    UInt32 GetCurrentCallNumber() const;
    
    // general states
    //void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
    //const Cube2D GetViewport() const;

    // FramebufferObject
    //RESOURCE_MANAGEMENT_DECLARATION(FramebufferObject, framebufferObject)
    // Operations on bound framebuffer object
    void BindTextureObjectToFramebufferObject(UInt32 target, UInt32 attachment, UInt32 textureHandle, UInt32 textureLevle);
    //void BindRenderbufferObjectToFramebufferObject(unsigned int attachment, unsigned int rboSlotNum);

    // TextureObject
    void SetActiveTextureUnit(UInt32 unit);
    UInt32 GetActiveTextureUnit() const;
    void CreateTextureObject(UInt32 handle);
    bool BindTextureObject(UInt32 target, UInt32 handle);

    TextureObjectPtr FindTextureObject(UInt32 handle, UInt32 callNum);
    TextureObjectPtr GetBoundTextureObject(UInt32 unit, UInt32 target);

    // Operations on bound texture object
    //void SetTexParameter(unsigned int target, unsigned int pname, unsigned int param);
    void SetTexImage(UInt32 target, UInt32 level, UInt32 format, UInt32 type, UInt32 width, UInt32 height, const void *data);
    void SetTexSubImage(UInt32 target, UInt32 level, UInt32 format, UInt32 type, UInt32 xoffset, UInt32 yoffset, UInt32 width, UInt32 height, const void *data);
    void SetCompressedTexImage(UInt32 target, UInt32 level, UInt32 format, UInt32 width, UInt32 height, UInt32 dataSize, const void *data);
    void GenerateMipmap(UInt32 target);
    
    void DumpTextureObjects() const;

    // RenderbufferObject
    //RESOURCE_MANAGEMENT_DECLARATION(RenderbufferObject, renderbufferObject)
    // Operations on bound renderbuffer object
    //void SetRenderbufferObjectStorage(unsigned int format, unsigned int widht, unsigned int height);

    // BufferObject
    //RESOURCE_MANAGEMENT_DECLARATION(BufferObject, bufferObject)
    // Operations on bound buffer object
    //void SetBufferData(unsigned int target, unsigned int size, const void *data, unsigned int usage);
    //void SetBufferSubData(unsigned int target, unsigned int offset, unsigned int size, const void *data);

    // ProgramObject


    // ShaderObject
    ShaderObjectPtr FindShaderObject(UInt32 handle, UInt32 callNum);
    void CreateShader(UInt32 shaderType, UInt32 name);
    void ShaderSource(UInt32 name, const std::string &source);
    void DumpShaderObjects() const;

private:
    void Reset();

    //Cube2D _viewport;
    UInt32 _currentCallNumber;

    typedef std::map<ObjectID, ShaderObjectPtr> ShaderObjectMap;
    ShaderObjectMap _shaderObjects;

    UInt32 _activeTextureUnit;
    typedef std::map<ObjectID, TextureObjectPtr> TextureObjectMap;
    TextureObjectMap _textureObjects;
    typedef std::pair<UInt32, UInt32> BoundTextureObjectKey;
    typedef std::map<BoundTextureObjectKey, TextureObjectPtr> BoundTextureObjectMap;
    BoundTextureObjectMap _boundTextureObjects;
};

typedef std::shared_ptr<Context> ContextPtr;
typedef std::vector<ContextPtr> ContextPtrList;
void InitializeContexts();
void UninitializeContexts();
ContextPtr GetStateMangerForThread(UInt32 threadID);
const ContextPtrList GetAllContexts();

}

#endif
