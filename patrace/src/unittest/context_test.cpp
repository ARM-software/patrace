#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "context_test.hpp"
#include "eglstate/context.hpp"
//#include "eglstate/fbo.hpp"
//#include "eglstate/render_target.hpp"
//#include "eglstate/frame.hpp"
//#include "eglstate/vbo.hpp"

using namespace pat;

ContextTest::ContextTest()
{
}

void ContextTest::setUp()
{
    InitializeContexts();
}

void ContextTest::tearDown()
{
    UninitializeContexts();
}

void ContextTest::testEnumString()
{
    CPPUNIT_ASSERT(EnumString(GL_POINTS, "glDrawArrays"));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_POINTS, "glDrawArrays"), "GL_POINTS") == 0);
    CPPUNIT_ASSERT(EnumString(GL_UNSIGNED_BYTE, "glDrawArrays"));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_UNSIGNED_BYTE, "glDrawArrays"), "GL_UNSIGNED_BYTE") == 0);
    CPPUNIT_ASSERT(EnumString(GL_NONE));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_NONE), "GL_NONE") == 0);
    CPPUNIT_ASSERT(EnumString(GL_NONE, "glDrawBuffers"));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_NONE, "glDrawBuffers"), "GL_NONE") == 0);
    CPPUNIT_ASSERT(EnumString(GL_TEXTURE_CUBE_MAP_NEGATIVE_X));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_TEXTURE_CUBE_MAP_NEGATIVE_X), "GL_TEXTURE_CUBE_MAP_NEGATIVE_X") == 0);
    CPPUNIT_ASSERT(EnumString(GL_LINEAR_MIPMAP_NEAREST));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_LINEAR_MIPMAP_NEAREST), "GL_LINEAR_MIPMAP_NEAREST") == 0);
    CPPUNIT_ASSERT(EnumString(GL_ETC1_RGB8_OES));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_ETC1_RGB8_OES), "GL_ETC1_RGB8_OES") == 0);
    CPPUNIT_ASSERT(EnumString(GL_COMPRESSED_RGBA_ASTC_4x4_KHR));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_COMPRESSED_RGBA_ASTC_4x4_KHR), "GL_COMPRESSED_RGBA_ASTC_4x4_KHR") == 0);

    CPPUNIT_ASSERT(EnumString(EGL_NONE));
    CPPUNIT_ASSERT(strcmp(EnumString(EGL_NONE), "EGL_NONE") == 0);

    CPPUNIT_ASSERT(EnumString(GL_TEXTURE_COORD_ARRAY));
    CPPUNIT_ASSERT(strcmp(EnumString(GL_TEXTURE_COORD_ARRAY), "GL_TEXTURE_COORD_ARRAY") == 0);
}

void ContextTest::testShader()
{
    ContextPtr manager = GetStateMangerForThread(1);

    manager->SetCurrentCallNumber(150);
    manager->CreateShader(GL_VERTEX_SHADER, 2);
    manager->ShaderSource(2, "test content 1");

    manager->SetCurrentCallNumber(250);
    manager->CreateShader(GL_VERTEX_SHADER, 2);
    manager->ShaderSource(2, "test content 2");

    CPPUNIT_ASSERT(manager->FindShaderObject(2, 100) == NULL);
    CPPUNIT_ASSERT(manager->FindShaderObject(2, 150) != NULL);
    CPPUNIT_ASSERT(manager->FindShaderObject(2, 150)->source == "test content 1");
    CPPUNIT_ASSERT(manager->FindShaderObject(2, 152) != NULL);
    CPPUNIT_ASSERT(manager->FindShaderObject(2, 252) != NULL);
    CPPUNIT_ASSERT(manager->FindShaderObject(2, 252)->source == "test content 2");
    CPPUNIT_ASSERT(manager->FindShaderObject(1, 150) == NULL);
    CPPUNIT_ASSERT(GetStateMangerForThread(0)->FindShaderObject(2, 150) == NULL);

    //Context::Instance()->DumpShaderObjects();
}

void ContextTest::testInitialState()
{
    ContextPtr manager= GetStateMangerForThread(0);

    CPPUNIT_ASSERT(manager->GetCurrentCallNumber() == 0);

    CPPUNIT_ASSERT(manager->GetActiveTextureUnit() == GL_TEXTURE0);

    //CPPUNIT_ASSERT(instance->FramebufferObjectCount() == 1);

    //const FramebufferObject * curFBO = instance->BoundFramebufferObject(GL_FRAMEBUFFER);
    //CPPUNIT_ASSERT(curFBO != NULL);
    //CPPUNIT_ASSERT(curFBO->BoundRenderTarget(GL_COLOR_ATTACHMENT0) == NULL);
    //CPPUNIT_ASSERT(curFBO->BoundRenderTarget(GL_DEPTH_ATTACHMENT) == NULL);
    //CPPUNIT_ASSERT(curFBO->BoundRenderTarget(GL_STENCIL_ATTACHMENT) == NULL);

    //CPPUNIT_ASSERT(instance->TextureObjectCount() == 0);
    //CPPUNIT_ASSERT(instance->BoundTextureObject(GL_TEXTURE_2D) == NULL);
    //CPPUNIT_ASSERT(instance->BoundTextureObject(GL_TEXTURE_CUBE_MAP) == NULL);
    //CPPUNIT_ASSERT(instance->RenderbufferObjectCount() == 0);
    //CPPUNIT_ASSERT(instance->BoundRenderbufferObject(GL_RENDERBUFFER) == NULL);
}

void ContextTest::testFBO()
{
    //Context * instance = Context::Instance();

    //instance->CreateFramebufferObject(1);
    //CPPUNIT_ASSERT(instance->FramebufferObjectCount() == 2);
    //instance->DestroyFramebufferObject(1);
    //CPPUNIT_ASSERT(instance->FramebufferObjectCount() == 2);
    //instance->CreateFramebufferObject(1);
    //CPPUNIT_ASSERT(instance->FramebufferObjectCount() == 3);
    //instance->CreateFramebufferObject(2);
    //CPPUNIT_ASSERT(instance->FramebufferObjectCount() == 4);

    //instance->CreateTextureObject(2);
    //instance->BindRenderbufferObject(GL_RENDERBUFFER, 5);

    //instance->BindFramebufferObject(GL_FRAMEBUFFER, 1);
    //const FramebufferObject * curFBO = instance->BoundFramebufferObject(GL_FRAMEBUFFER);
    //instance->BindTextureObjectToFramebufferObject(GL_COLOR_ATTACHMENT0, 2);
    //const RenderTarget * target = curFBO->BoundRenderTarget(GL_COLOR_ATTACHMENT0);
    //CPPUNIT_ASSERT(target != NULL);
    //CPPUNIT_ASSERT(target->type() == GL_TEXTURE);
    //instance->BindRenderbufferObjectToFramebufferObject(GL_DEPTH_ATTACHMENT, 5);
    //target = curFBO->BoundRenderTarget(GL_DEPTH_ATTACHMENT);
    //CPPUNIT_ASSERT(target != NULL);
    //CPPUNIT_ASSERT(target->type() == GL_RENDERBUFFER);
}

//void ContextTest::testRenderbufferObject()
//{
    //Context *instance = Context::Instance();

    //instance->CreateRenderbufferObject(3);
    //instance->BindRenderbufferObject(GL_RENDERBUFFER, 3);
    //const RenderbufferObject *obj = instance->BoundRenderbufferObject(GL_RENDERBUFFER);
    //CPPUNIT_ASSERT(obj);
    //CPPUNIT_ASSERT(obj->Format() == GL_NONE);
    //CPPUNIT_ASSERT(obj->Width() == 0);
    //CPPUNIT_ASSERT(obj->Height() == 0);

    //instance->SetRenderbufferObjectStorage(GL_RGB5_A1, 1280, 720);
    //obj = instance->BoundRenderbufferObject(GL_RENDERBUFFER);
    //CPPUNIT_ASSERT(obj->Format() == GL_RGB5_A1);
    //CPPUNIT_ASSERT(obj->Width() == 1280);
    //CPPUNIT_ASSERT(obj->Height() == 720);

    //instance->DestroyRenderbufferObject(3);
//}

void ContextTest::testTexturObject()
{
    ContextPtr manager = GetStateMangerForThread(1);

    manager->SetCurrentCallNumber(150);
    manager->SetActiveTextureUnit(GL_TEXTURE1);
    CPPUNIT_ASSERT(manager->GetActiveTextureUnit() == GL_TEXTURE1);
    CPPUNIT_ASSERT(manager->GetCurrentCallNumber() == 150);

    manager->SetCurrentCallNumber(152);
    manager->CreateTextureObject(2);
    CPPUNIT_ASSERT(manager->FindTextureObject(2, 152));
    CPPUNIT_ASSERT(GetStateMangerForThread(2)->FindTextureObject(2, 152) == NULL);
    CPPUNIT_ASSERT(manager->FindTextureObject(2, 150) == NULL);
    CPPUNIT_ASSERT(manager->FindTextureObject(1, 152) == NULL);

    manager->SetCurrentCallNumber(154);
    CPPUNIT_ASSERT(manager->BindTextureObject(GL_TEXTURE_2D, 1));
    CPPUNIT_ASSERT(manager->BindTextureObject(GL_TEXTURE_2D, 2));
    CPPUNIT_ASSERT(manager->GetBoundTextureObject(GL_TEXTURE0, GL_TEXTURE_2D) == NULL);
    CPPUNIT_ASSERT(manager->GetBoundTextureObject(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP) == NULL);
    TextureObjectPtr tex = manager->GetBoundTextureObject(GL_TEXTURE1, GL_TEXTURE_2D);
    CPPUNIT_ASSERT(tex);

    ContextPtr manager2 = GetStateMangerForThread(2);
    manager2->SetCurrentCallNumber(160);
    manager2->SetActiveTextureUnit(GL_TEXTURE1);
    manager2->SetCurrentCallNumber(162);
    manager2->CreateTextureObject(5);
    manager2->SetCurrentCallNumber(164);
    CPPUNIT_ASSERT(manager2->BindTextureObject(GL_TEXTURE_2D, 5));
    TextureObjectPtr tex5 = manager2->GetBoundTextureObject(GL_TEXTURE1, GL_TEXTURE_2D);
    CPPUNIT_ASSERT(tex5);

    CPPUNIT_ASSERT(tex5 != tex);
    CPPUNIT_ASSERT(manager->GetBoundTextureObject(GL_TEXTURE1, GL_TEXTURE_2D) == tex);

    //CPPUNIT_ASSERT(obj->GetParameter(GL_TEXTURE_MIN_FILTER) == GL_NEAREST_MIPMAP_LINEAR);
    //CPPUNIT_ASSERT(obj->GetParameter(GL_TEXTURE_MAG_FILTER) == GL_LINEAR);
    //CPPUNIT_ASSERT(cobj->GetParameter(GL_TEXTURE_WRAP_S) == GL_REPEAT);
    //CPPUNIT_ASSERT(cobj->GetParameter(GL_TEXTURE_WRAP_T) == GL_REPEAT);

    //manager->SetTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //manager->SetTexParameter(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //CPPUNIT_ASSERT(obj->GetParameter(GL_TEXTURE_MIN_FILTER) == GL_NEAREST);
    //CPPUNIT_ASSERT(cobj->GetParameter(GL_TEXTURE_MAG_FILTER) == GL_LINEAR);
    //CPPUNIT_ASSERT(obj->GetParameter(GL_TEXTURE_WRAP_S) == GL_REPEAT);
    //CPPUNIT_ASSERT(cobj->GetParameter(GL_TEXTURE_WRAP_T) == GL_CLAMP_TO_EDGE);

    const unsigned char IMAGE_DATA[] = {
        0xFF, 0x00, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x00,
    };
    const unsigned char SUB_IMAGE_DATA[] = {
        0xFF, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0x00,
    };
    const unsigned char FINAL_IMAGE_DATA[] = {
        0xFF, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0x00,
        0xFF, 0xFF, 0xFF, 0x00,
    };
    manager->SetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 2, 2, NULL);
    CPPUNIT_ASSERT(!(tex->GetLevel(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0)));
    ImagePtr level = tex->GetLevel(GL_TEXTURE_2D, 0);
    CPPUNIT_ASSERT(level->Format() == GL_RGBA);
    CPPUNIT_ASSERT(level->Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(level->Width() == 2);
    CPPUNIT_ASSERT(level->Height() == 2);
    CPPUNIT_ASSERT(level->DataSize() == sizeof(IMAGE_DATA));
    CPPUNIT_ASSERT(level->Data() == NULL);
    manager->SetTexSubImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 1, 2, SUB_IMAGE_DATA);

    manager->SetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 2, 2, IMAGE_DATA);
    manager->SetTexSubImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 1, 2, SUB_IMAGE_DATA);
    level = tex->GetLevel(GL_TEXTURE_2D, 0);
    CPPUNIT_ASSERT(level->Format() == GL_RGBA);
    CPPUNIT_ASSERT(level->Type() == GL_UNSIGNED_BYTE);
    CPPUNIT_ASSERT(level->Width() == 2);
    CPPUNIT_ASSERT(level->Height() == 2);
    CPPUNIT_ASSERT(level->DataSize() == sizeof(IMAGE_DATA));
    CPPUNIT_ASSERT(memcmp(level->Data(), FINAL_IMAGE_DATA, sizeof(FINAL_IMAGE_DATA)) == 0);

    manager->GenerateMipmap(GL_TEXTURE_2D);

    unsigned char compressed_data[] = {
        0xFC, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};
    manager->SetCompressedTexImage(GL_TEXTURE_2D, 0, COMPRESSED_ASTC_RGBA_4x4_OES, 2, 1, sizeof(compressed_data), compressed_data);
    level = tex->GetLevel(GL_TEXTURE_2D, 0);
    CPPUNIT_ASSERT(level->Format() == COMPRESSED_ASTC_RGBA_4x4_OES);
    CPPUNIT_ASSERT(level->Type() == GL_NONE);
    CPPUNIT_ASSERT(level->Width() == 2);
    CPPUNIT_ASSERT(level->Height() == 1);
    CPPUNIT_ASSERT(level->DataSize() == sizeof(compressed_data));
    CPPUNIT_ASSERT(memcmp(level->Data(), compressed_data, sizeof(compressed_data)) == 0);

    manager->SetCurrentCallNumber(200);
    manager->CreateTextureObject(2);
    manager->SetCurrentCallNumber(202);
    manager->BindTextureObjectToFramebufferObject(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 2, 0);
    manager->SetCurrentCallNumber(204);
    CPPUNIT_ASSERT(manager->BindTextureObject(GL_TEXTURE_CUBE_MAP, 2));
    manager->SetCurrentCallNumber(206);
    manager->SetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, COMPRESSED_ASTC_RGBA_4x4_OES, 2, 1, sizeof(compressed_data), NULL);

    tex = manager->FindTextureObject(2, 155);
    CPPUNIT_ASSERT(tex);
    CPPUNIT_ASSERT(tex->UsedAsRenderTarget() == false);
    CPPUNIT_ASSERT(tex->HaveSetSubImage() == true);
    CPPUNIT_ASSERT(tex->HaveGeneratedMipmap() == true);

    tex = manager->FindTextureObject(2, 201);
    CPPUNIT_ASSERT(tex);
    CPPUNIT_ASSERT(tex == manager->GetBoundTextureObject(manager->GetActiveTextureUnit(), GL_TEXTURE_CUBE_MAP_POSITIVE_X));
    CPPUNIT_ASSERT(tex->UsedAsRenderTarget() == true);
    CPPUNIT_ASSERT(tex->HaveSetSubImage() == false);
    CPPUNIT_ASSERT(tex->HaveGeneratedMipmap() == false);

    //manager->DumpTextureObjects();
}

//void ContextTest::testVBO()
//{
    //Context *instance = Context::Instance();

    //CPPUNIT_ASSERT(instance->BufferObjectCount() == 0);
    //CPPUNIT_ASSERT(instance->BoundBufferObject(GL_ARRAY_BUFFER) == NULL);
    //CPPUNIT_ASSERT(instance->BoundBufferObject(GL_ELEMENT_ARRAY_BUFFER) == NULL);

    //instance->BindBufferObject(GL_ARRAY_BUFFER, 1);
    //BufferObject *vbo = instance->BoundBufferObject(GL_ARRAY_BUFFER);
    //CPPUNIT_ASSERT(vbo);
    //CPPUNIT_ASSERT(vbo->GetData() == NULL);
    //CPPUNIT_ASSERT(vbo->GetSize() == 0);

    //const unsigned char DATA[] = {0, 1, 2, 3, 4, 5};
    //const unsigned int DATA_SIZE = sizeof(DATA) / sizeof(unsigned char);
    //instance->SetBufferData(GL_ARRAY_BUFFER, DATA_SIZE, NULL, GL_STREAM_DRAW);
    //CPPUNIT_ASSERT(vbo->GetUsage() == GL_STREAM_DRAW);
    //CPPUNIT_ASSERT(vbo->GetSize() == DATA_SIZE);
    //CPPUNIT_ASSERT(vbo->GetData() != NULL);
    //CPPUNIT_ASSERT(vbo->GetType() == GL_ARRAY_BUFFER);

    //instance->SetBufferSubData(GL_ARRAY_BUFFER, 0, DATA_SIZE, DATA);
    //CPPUNIT_ASSERT(vbo->GetUsage() == GL_STREAM_DRAW);
    //CPPUNIT_ASSERT(vbo->GetSize() == DATA_SIZE);
    //CPPUNIT_ASSERT(memcmp(vbo->GetData(), DATA, DATA_SIZE) == 0);
    //CPPUNIT_ASSERT(vbo->GetType() == GL_ARRAY_BUFFER);
//}
