#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "image/image.hpp"
#include "image/image_compression.hpp"
#include "eglstate/context.hpp"
#include "tool/trace_interface.hpp"
#include "tool/config.hpp"
#include "json/json.h"

using namespace pat;

namespace
{

bool CallToImage(CallInterface *call, pat::Image &image)
{
    if (strcmp(call->GetName(), "glTexImage2D") == 0)
    {
        const unsigned int width = call->arg_to_uint(3);
        const unsigned int height = call->arg_to_uint(4);
        const unsigned int format = call->arg_to_uint(6);
        const unsigned int type = call->arg_to_uint(7);
        bool bufferObject = false;
        unsigned char *pixels = call->arg_to_pointer(8, &bufferObject);
        const unsigned int size = pat::GetImageDataSize(width, height, format, type);

        if (width && height && size == 0)
            return false;

        image.Set(width, height, format, type, size, pixels, false, false);
        return true;
    }
    else if (strcmp(call->GetName(), "glCompressedTexImage2D") == 0)
    {
        const unsigned int format = call->arg_to_uint(2);
        const unsigned int width = call->arg_to_uint(3);
        const unsigned int height = call->arg_to_uint(4);
        const unsigned int image_size = call->arg_to_uint(6);
        bool bufferObject = false;
        unsigned char *pixels = call->arg_to_pointer(7, &bufferObject);
        image.Set(width, height, format, GL_NONE, image_size, pixels, false, false);
        return true;
    }
    else
    {
        printf("Unknown image call : %s\n", call->GetName());
        return false;
    }
}

bool ImageToCall(const pat::Image &image, CallInterface *call)
{
    const unsigned int format = image.Format();
    const unsigned int type = image.Type();
    if (pat::IsImageCompression(format))
    {
        call->set_signature("glCompressedTexImage2D");
        call->clear_args(2);
        call->push_back_enum_arg(format);
        call->push_back_sint_arg(image.Width());
        call->push_back_sint_arg(image.Height());
        call->push_back_sint_arg(0);
        call->push_back_sint_arg(image.DataSize());
        if (image.Data())
        {
            call->push_back_pointer_arg(image.DataSize(), image.Data());
        }
        else
        {
            call->push_back_pointer_arg(0, NULL);
        }
        return true;
    }
    else
    {
        call->set_signature("glTexImage2D");
        call->clear_args(2);
        call->push_back_enum_arg(format);
        call->push_back_sint_arg(image.Width());
        call->push_back_sint_arg(image.Height());
        call->push_back_sint_arg(0);
        call->push_back_enum_arg(format);
        call->push_back_enum_arg(type);
        if (image.Data())
        {
            call->push_back_pointer_arg(image.DataSize(), image.Data());
        }
        else
        {
            call->push_back_pointer_arg(0, NULL);
        }
        return true;
    }
}

void printHelp()
{
    std::cout <<
        "Usage : trace_modifier [OPTIONS] source_trace target_trace\n"
        "Options:\n"
        "  -h            print help\n"
        "  -v            print version\n"
        "  -mode MODE    select compression mode, controlling which textures to compress\n"
        "    Supported modes: default is INPUT\n"
        "     INPUT         Only compress the textures already compressed in the input trace\n"
        "     NOALPHA       Compress as much textures as possible, but ignore the ones with alpha channels\n"
        "     COMPLETE      Compress as much textures as possible\n"
        "  -com FORMAT   compress as specific texture compression format\n"
        "    Supported formats:\n";
    const char **optionList = NULL;
    unsigned int optionNum = 0;
    pat::GetCompressionOptionList(optionList, optionNum);
    for (unsigned int i = 0; i < optionNum; ++i)
    {
        std::cout << "     " << optionList[i] << std::endl;
    }
}

void printVersion()
{
    std::cout << "Version: " PATRACE_VERSION << std::endl;
}

} // unamed namespace

extern "C"
int main(int argc, char **argv)
{
    std::string encode_format;
    std::string mode = "INPUT";

    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (!strcmp(arg, "-h"))
        {
            printHelp();
            return 1;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 1;
        }
        else if (!strcmp(arg, "-mode"))
        {
            mode = argv[++argIndex];
        }
        else if (!strcmp(arg, "-com"))
        {
            encode_format = argv[++argIndex];
        }
        else
        {
            printf("Error: Unknow option %s\n", arg);
            printHelp();
            return 1;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }
    const char * source_trace_filename = argv[argIndex++];
    const char * target_trace_filename = argv[argIndex++];

    if (pat::IsValidCompressionOption(encode_format) == false)
    {
        printf("Error : Unknown encode format %s\n", encode_format.c_str());
        printHelp();
        return -1;
    }

    // Check the availability of external tools
    if (pat::CheckCompressionOptionSupport(encode_format) == false)
    {
        return -1;
    }

    // First pass: find the textures using glGenerateMipmap & glTexSubImage2D & glFramebufferTexture2D
    UInt32 texImageCallNumber = 0;
    UInt32 compressedTexImageCallNumber = 0;
    printf("LOG : Pre-scan the trace file to collect texture usage info ...\n");

    std::shared_ptr<InputFileInterface> inputFile(GenerateInputFile());
    if (!inputFile->open(source_trace_filename))
    {
        printf("Error : failed to open %s\n", source_trace_filename);
        return 1;
    }

    CallInterface *call = NULL;
    while ((call = inputFile->next_call()))
    {
        const UInt32 callNo = call->GetNumber();
        const UInt32 thread = call->GetThreadID();
        pat::ContextPtr context = pat::GetStateMangerForThread(thread);
        context->SetCurrentCallNumber(callNo);

        if (strcmp(call->GetName(), "glGenTextures") == 0)
        {
            const UInt32 n = call->arg_to_uint(0);
            for (UInt32 i = 0; i < n; i++)
            {
                const UInt32 tex = call->array_arg_to_uint(1, i);
                context->CreateTextureObject(tex);
            }
        }
        else if (strcmp(call->GetName(), "glActiveTexture") == 0)
        {
            const UInt32 unit = call->arg_to_uint(0);
            context->SetActiveTextureUnit(unit);
        }
        else if (strcmp(call->GetName(), "glBindTexture") == 0)
        {
            const UInt32 target = call->arg_to_uint(0);
            const UInt32 texture = call->arg_to_uint(1);
            context->BindTextureObject(target, texture);
        }
        else if (strcmp(call->GetName(), "glGenerateMipmap") == 0)
        {
            const UInt32 target = call->arg_to_uint(0);
            pat::TextureObjectPtr boundTex = context->GetBoundTextureObject(context->GetActiveTextureUnit(), target);
            PAT_DEBUG_ASSERT(boundTex, "No texture object is bound no.%d(%s)\n", callNo, call->GetName());
            boundTex->GenerateMipmap();
        }
        else if (strcmp(call->GetName(), "glTexImage2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int level = call->arg_to_uint(1);
            const unsigned int format = call->arg_to_uint(6);
            const unsigned int type = call->arg_to_uint(7);
            context->SetTexImage(target, level, format, type, 0, 0, NULL);

            if (mode == "COMPLETE") // only compress this texture if the selected mode is COMPLETE
                texImageCallNumber += 1;
            else if (mode == "NOALPHA" && !pat::WithAlphaChannel(format))
                texImageCallNumber += 1;
        }
        else if (strcmp(call->GetName(), "glTexSubImage2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int level = call->arg_to_uint(1);
            const unsigned int format = call->arg_to_uint(6);
            const unsigned int type = call->arg_to_uint(7);
            pat::TextureObjectPtr boundTex = context->GetBoundTextureObject(context->GetActiveTextureUnit(), target);
            PAT_DEBUG_ASSERT(boundTex, "No texture object is bound no.%d(%s)\n", callNo, call->GetName());
            boundTex->SetSubImage(target, level, format, type, 0, 0, 0, 0, NULL);
        }
        else if (strcmp(call->GetName(), "glCompressedTexImage2D") == 0)
        {
            compressedTexImageCallNumber += 1;
        }
        else if (strcmp(call->GetName(), "glFramebufferTexture2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int attachment = call->arg_to_uint(1);
            const unsigned int handle = call->arg_to_uint(3);
            const unsigned int level = call->arg_to_uint(4);
            context->BindTextureObjectToFramebufferObject(target, attachment, handle, level);
        }

        delete call;
    }

    // Second pass
    printf("LOG : Begin the compression/uncompression pass ...\n");
    const UInt32 totalCall = (encode_format == "UNCOMPRESSED") ? compressedTexImageCallNumber :
        compressedTexImageCallNumber + texImageCallNumber;
    UInt32 finishedCall = 0;

    inputFile->reset();

    std::shared_ptr<OutputFileInterface> outputFile(GenerateOutputFile());
    if (!outputFile->open(target_trace_filename))
    {
        printf("Error : failed to open %s\n", target_trace_filename);
        return 1;
    }

    // copy the json string of the header from the input file to the output file
    std::string json_header = inputFile->json_header();
    unsigned int compressCompleted = 0;

    while ((call = inputFile->next_call()))
    {
        const UInt32 callNo = call->GetNumber();
        const UInt32 thread = call->GetThreadID();
        pat::ContextPtr context = pat::GetStateMangerForThread(thread);
        context->SetCurrentCallNumber(callNo);

        if (strcmp(call->GetName(), "glBindTexture") == 0) // record the state of bound texture
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int texture = call->arg_to_uint(1);
            context->BindTextureObject(target, texture);
        }
        else if (strcmp(call->GetName(), "glActiveTexture") == 0)
        {
            const unsigned int unit = call->arg_to_uint(0);
            context->SetActiveTextureUnit(unit);
        }
        else if (strcmp(call->GetName(), "glTexImage2D") == 0 && encode_format != "UNCOMPRESSED" &&
                (mode == "COMPLETE" || (mode == "NOALPHA" && !pat::WithAlphaChannel(call->arg_to_uint(6)))))
        {
            const unsigned int target = call->arg_to_uint(0);

            pat::TextureObjectPtr boundTex = context->GetBoundTextureObject(context->GetActiveTextureUnit(), target);
            if (boundTex)
            {
                if (boundTex->UsedAsRenderTarget())
                {
                    printf("LOG [%d/%d]: Process call no.%d(%s) can't be compressed since the bound texture object is used as render target\n", ++finishedCall, totalCall, callNo, call->GetName());
                }
                else if (boundTex->HaveSetSubImage())
                {
                    printf("LOG [%d/%d]: Process call no.%d(%s) can't be compressed since the bound texture object is set with sub image\n", ++finishedCall, totalCall, callNo, call->GetName());
                }
                else if (boundTex->HaveGeneratedMipmap())
                {
                    printf("LOG [%d/%d]: Process call no.%d(%s) can't be compressed since the bound texture object generates mipmap\n", ++finishedCall, totalCall, callNo, call->GetName());
                }
                else
                {
                    pat::Image uncompressed, compressed;
                    if (CallToImage(call, uncompressed) == false)
                    {
                        printf("Error : Failed to convert call to image no.%d(%s)\n", callNo, call->GetName());
                        return -1;
                    }

                    if (pat::CanCompressAs(uncompressed.Format(), uncompressed.Type(), encode_format))
                    {
                        if (pat::Compress(uncompressed, compressed, encode_format))
                        {
                            if (ImageToCall(compressed, call) == false)
                            {
                                printf("Error : Failed to convert image to call no.%d(%s)\n", callNo, call->GetName());
                                return -1;
                            }
                            else
                            {
                                printf("LOG [%d/%d]: Processed call no.%d(%s)\n", ++finishedCall, totalCall, callNo, call->GetName());
                                ++compressCompleted;
                            }
                        }
                        else
                        {
                            printf("Error : Failed to compress call no.%d(%s)\n", callNo, call->GetName());
                            return -1;
                        }
                    }
                    else
                    {
                        const char *format_str = EnumString(uncompressed.Format());
                        const char *type_str = EnumString(uncompressed.Type());
                        printf("LOG [%d/%d]: For call no.%d(%s), compression as %s doesn't support input format(%s) and type(%s) combination\n", ++finishedCall, totalCall, callNo, call->GetName(), encode_format.c_str(), format_str, type_str);
                    }
                }
            }
            else
            {
                printf("No texture object is bound no.%d(%s)\n", callNo, call->GetName());
            }
        }
        else if (strcmp(call->GetName(), "glCompressedTexImage2D") == 0)
        {
            pat::Image oldCompressed, uncompressed, newCompressed;
            if (CallToImage(call, oldCompressed) == false)
            {
                printf("Error : Failed to convert call to image no.%d(%s)\n", callNo, call->GetName());
                return -1;
            }

            if (pat::Uncompress(oldCompressed, uncompressed))
            {
                if (encode_format == "UNCOMPRESSED")
                {
                    if (ImageToCall(uncompressed, call) == false)
                    {
                        printf("Error : Failed to convert image to call no.%d(%s)\n", callNo, call->GetName());
                        return -1;
                    }
                    printf("LOG [%d/%d]: Processed call no.%d(%s)\n", ++finishedCall, totalCall, callNo, call->GetName());
                    ++compressCompleted;
                }
                else if (pat::CanCompressAs(uncompressed.Format(), uncompressed.Type(), encode_format))
                {
                    if (pat::Compress(uncompressed, newCompressed, encode_format))
                    {
                        if (ImageToCall(newCompressed, call) == false)
                        {
                            printf("Error : Failed to convert image to call no.%d(%s)\n", callNo, call->GetName());
                            return -1;
                        }
                        printf("LOG [%d/%d]: Processed call no.%d(%s)\n", ++finishedCall, totalCall, callNo, call->GetName());
                        ++compressCompleted;
                    }
                    else
                    {
                        printf("Error : Failed to compress call no.%d(%s)\n", callNo, call->GetName());
                        return -1;
                    }
                }
            }
            else
            {
                printf("Error : Failed to uncompress call no.%d(%s) and keep the call as it was.\n", callNo, call->GetName());
            }
        }

        outputFile->write(call);

        delete call;
    }

    printf("Summary : In total, %d calls have been compressed.\n", compressCompleted);

    if (compressCompleted > 0)
    {
        Json::Value jsonHeader;
        Json::Reader reader;
        if (reader.parse(json_header, jsonHeader))
        {
            if (encode_format == "ETC1")
                jsonHeader["texCompress"] = GL_ETC1_RGB8_OES;
            else if (encode_format == "ASTC4x4")
                jsonHeader["texCompress"] = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            else if (encode_format == "ASTC6x6")
                jsonHeader["texCompress"] = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            else
                printf("Error : Unexpected compression format.\n");

            if (encode_format == "ASTC4x4" || encode_format == "ASTC6x6")
                jsonHeader["glesVersion"] = 3;
        }
        else
        {
            printf("Error : Failed to parse the json header.\n");
        }

        Json::FastWriter writer;
        json_header = writer.write(jsonHeader);
    }

    outputFile->write_json_header(json_header);
    inputFile->close();
    outputFile->close();

    return 0;
}
