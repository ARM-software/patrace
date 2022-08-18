// Swiss army knife tool for patrace - for all kinds of misc stuff

#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>
#include <set>
#include <utility>

#include "tool/parse_interface.h"

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"
#include "tool/utils.hpp"

#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

static bool debug = false;
static bool onlycount = false;
static bool verbose = false;
static int endframe = -1;
static bool waitsync = false;
static std::set<int> unused_mipmaps; // call numbers
static std::set<std::pair<int, int>> unused_textures; // context index + texture index
static std::set<std::pair<int, int>> uninit_textures; // context index + texture index
static int lastframe = -1;

static void printHelp()
{
    std::cout <<
        "Usage : converter [OPTIONS] trace_file.pat new_file.pat\n"
        "Options:\n"
        "  --waitsync    Add a non-zero timeout to glClientWaitSync and glWaitSync calls\n"
        "  --mipmap FILE Remove unused glGenerateMipmap calls. Need a usage CSV file from analyze_trace as input\n"
        "  --tex FILE    Remove unused texture calls. Need an uninitialized CSV file from analyze_trace as input\n"
        "  --utex FILE   Fix uninitialized texture storage calls. Need a usage CSV file as input\n"
        "  --end FRAME   End frame (terminates trace here)\n"
        "  --last FRAME  Stop doing changes at this frame (copies the remaining trace without changes)\n"
        "  --verbose     Print more information while running\n"
        "  -c            Only count and report instances, do no changes\n"
        "  -d            Print debug messages\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    if (onlycount) return;
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];
    char *dest = buffer;
    dest = call->Serialize(dest);
    outputFile.Write(buffer, dest-buffer);
}

int converter(ParseInterface& input, common::OutFile& outputFile)
{
    common::CallTM *call = nullptr;
    int count = 0;

    // Go through entire trace file
    while ((call = input.next_call()))
    {
        if (lastframe != -1 && input.frames >= lastframe)
        {
            writeout(outputFile, call);
            continue;
        }

        if (call->mCallName == "glClientWaitSync")
        {
            const uint64_t timeout = call->mArgs[2]->GetAsUInt64();
            if (timeout == 0)
            {
                call->mArgs[2]->mUint64 = UINT64_MAX;
                count++;
            }
            writeout(outputFile, call);
        }
        else if (call->mCallName == "glGenerateMipmap" && unused_mipmaps.count(call->mCallNo) > 0)
        {
            count++;
        }
        else if (call->mCallName == "glTexStorage3D" || call->mCallName == "glTexStorage2D" || call->mCallName == "glTexStorage1D"
             || call->mCallName == "glTexStorage3DEXT" || call->mCallName == "glTexStorage2DEXT" || call->mCallName == "glTexStorage1DEXT"
             || call->mCallName == "glTexImage3D" || call->mCallName == "glTexImage2D" || call->mCallName == "glTexImage1D" || call->mCallName == "glTexImage3DOES"
             || call->mCallName == "glCompressedTexImage3D" || call->mCallName == "glCompressedTexImage2D"
             || call->mCallName == "glCompressedTexImage1D" || call->mCallName == "glTexSubImage1D" || call->mCallName == "glTexSubImage2D"
             || call->mCallName == "glTexSubImage3D" || call->mCallName == "glCompressedTexSubImage2D" || call->mCallName == "glCompressedTexSubImage3D")
        {
            const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
            const GLuint unit = input.contexts[input.context_index].activeTextureUnit;
            const GLuint tex_id = input.contexts[input.context_index].textureUnits[unit][target];
            assert(tex_id != 0);
            const int target_texture_index = input.contexts[input.context_index].textures.remap(tex_id);
            if (input.contexts[input.context_index].textures.contains(tex_id) && tex_id != 0)
            {
                if (unused_textures.count(std::make_pair(input.context_index, target_texture_index)) > 0)
                {
                    count++;
                    continue;
                }
            }
            writeout(outputFile, call);
            if ((call->mCallName == "glTexImage3D" || call->mCallName == "glTexImage2D") && uninit_textures.count(std::make_pair(input.context_index, target_texture_index)) > 0)
            {
                const GLint level = call->mArgs[1]->GetAsUInt();
                //const GLint internalFormat = call->mArgs[2]->GetAsUInt();
                const GLsizei width = call->mArgs[3]->GetAsUInt();
                const GLsizei height = call->mArgs[4]->GetAsUInt();
                //GLint border = call->mArgs[5]->GetAsUInt();
                const GLenum format = call->mArgs[6]->GetAsUInt();
                const GLenum type = call->mArgs[7]->GetAsUInt();
                common::CallTM c("glTexSubImage2D");
                c.mArgs.push_back(new common::ValueTM(target));
                c.mArgs.push_back(new common::ValueTM(level)); // level
                c.mArgs.push_back(new common::ValueTM(0)); // xoffset
                c.mArgs.push_back(new common::ValueTM(0)); // yoffset
                c.mArgs.push_back(new common::ValueTM(width));
                c.mArgs.push_back(new common::ValueTM(height));
                c.mArgs.push_back(new common::ValueTM(format));
                c.mArgs.push_back(new common::ValueTM(type));
                const unsigned tsize = width * height * 4 * 4; // max size
                std::vector<char> zeroes(tsize);
                c.mArgs.push_back(common::CreateBlobOpaqueValue(tsize, zeroes.data()));
                c.mTid = call->mTid;
                writeout(outputFile, &c);
                count++;
            }
            else if ((call->mCallName == "glTexStorage3D" || call->mCallName == "glTexStorage2D") && uninit_textures.count(std::make_pair(input.context_index, target_texture_index)) > 0)
            {
                const GLsizei levels = call->mArgs[1]->GetAsUInt();
                const GLenum format = call->mArgs[2]->GetAsUInt();
                const GLsizei width = call->mArgs[3]->GetAsUInt();
                const GLsizei height = call->mArgs[4]->GetAsUInt();
                if (isCompressedFormat(format))
                {
                    for (int i = 0; i < levels; i++)
                    {
                        const GLsizei w = width / (i + 1);
                        const GLsizei h = height / (i + 1);
                        common::CallTM c("glCompressedTexSubImage2D");
                        c.mArgs.push_back(new common::ValueTM(target));
                        c.mArgs.push_back(new common::ValueTM(i)); // level
                        c.mArgs.push_back(new common::ValueTM(0)); // xoffset
                        c.mArgs.push_back(new common::ValueTM(0)); // yoffset
                        c.mArgs.push_back(new common::ValueTM(w));
                        c.mArgs.push_back(new common::ValueTM(h));
                        c.mArgs.push_back(new common::ValueTM(format));
                        const unsigned tsize = w * h * 4 * 4; // max size
                        c.mArgs.push_back(new common::ValueTM(tsize)); // image size
                        std::vector<char> zeroes(tsize);
                        c.mArgs.push_back(common::CreateBlobOpaqueValue(tsize, zeroes.data()));
                        c.mTid = call->mTid;
                        writeout(outputFile, &c);
                    }
                }
                else // uncompressed
                {
                    for (int i = 0; i < levels; i++)
                    {
                        const GLsizei w = width / (i + 1);
                        const GLsizei h = height / (i + 1);
                        common::CallTM c("glTexSubImage2D");
                        c.mArgs.push_back(new common::ValueTM(target));
                        c.mArgs.push_back(new common::ValueTM(i)); // level
                        c.mArgs.push_back(new common::ValueTM(0)); // xoffset
                        c.mArgs.push_back(new common::ValueTM(0)); // yoffset
                        c.mArgs.push_back(new common::ValueTM(w));
                        c.mArgs.push_back(new common::ValueTM(h));
                        c.mArgs.push_back(new common::ValueTM(sized_to_unsized_format(format)));
                        c.mArgs.push_back(new common::ValueTM(sized_to_unsized_type(format)));
                        const unsigned tsize = w * h * 4 * 4; // max size
                        std::vector<char> zeroes(tsize);
                        c.mArgs.push_back(common::CreateBlobOpaqueValue(tsize, zeroes.data()));
                        c.mTid = call->mTid;
                        writeout(outputFile, &c);
                    }
                }
                count++;
            }
        }
        else
        {
            writeout(outputFile, call);
        }
    }
    printf("Calls changed: %d\n", count);
    return count;
}

int main(int argc, char **argv)
{
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        std::string arg = argv[argIndex];

        if (arg[0] != '-')
        {
            break;
        }
        else if (arg == "-h")
        {
            printHelp();
            return 1;
        }
        else if (arg == "--end")
        {
            argIndex++;
            endframe = atoi(argv[argIndex]);
        }
        else if (arg == "--last")
        {
            argIndex++;
            lastframe = atoi(argv[argIndex]);
        }
        else if (arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg == "--waitsync")
        {
            waitsync = true;
        }
        else if (arg == "-c")
        {
            onlycount = true;
        }
        else if (arg == "-d")
        {
            debug = true;
        }
        else if (arg == "--mipmap")
        {
            argIndex++;
            FILE* fp = fopen(argv[argIndex], "r");
            if (!fp) { printf("Error: Unable to open %s: %s\n", argv[argIndex], strerror(errno)); return -11; }
            int call = -1;
            int ignore = fscanf(fp, "%*[^\n]\n");
            (void)ignore;
            while (fscanf(fp, "%d,%*d,%*d,%*d\n", &call) == 1) unused_mipmaps.insert(call);
            fclose(fp);
        }
        else if (arg == "--utex")
        {
            argIndex++;
            FILE* fp = fopen(argv[argIndex], "r");
            if (!fp) { printf("Error: Unable to open %s: %s\n", argv[argIndex], strerror(errno)); return -11; }
            int ctxidx = 0;
            int txidx = 0;
            int ignore = fscanf(fp, "%*[^\n]\n"); // skip header line
            (void)ignore;
            // Call,Frame,TxIndex,TxId,ContextIndex,ContextId
            while (fscanf(fp, "%*d,%*d,%d,%*d,%d,%*d\n", &txidx, &ctxidx) == 2)
            {
                uninit_textures.insert(std::make_pair(ctxidx, txidx));
            }
            fclose(fp);
        }
        else if (arg == "--tex")
        {
            argIndex++;
            FILE* fp = fopen(argv[argIndex], "r");
            if (!fp) { printf("Error: Unable to open %s: %s\n", argv[argIndex], strerror(errno)); return -11; }
            int ctxidx = 0;
            int txidx = 0;
            int ignore = fscanf(fp, "%*[^\n]\n");
            (void)ignore;
            while (fscanf(fp, "%*d,%*d,%d,%*d,%d,%*d\n", &txidx, &ctxidx) == 2) unused_textures.insert(std::make_pair(ctxidx, txidx));
            fclose(fp);
        }
        else if (arg == "-v")
        {
            printVersion();
            return 0;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if ((argIndex + 2 > argc && !onlycount) || (onlycount && argIndex + 1 > argc))
    {
        printHelp();
        return 1;
    }
    std::string source_trace_filename = argv[argIndex++];
    ParseInterface inputFile(true);
    inputFile.setQuickMode(true);
    inputFile.setScreenshots(false);
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }

    Json::Value header = inputFile.header;
    common::OutFile outputFile;
    if (!onlycount)
    {
        std::string target_trace_filename = argv[argIndex++];
        if (!outputFile.Open(target_trace_filename.c_str()))
        {
            std::cerr << "Failed to open for writing: " << target_trace_filename << std::endl;
            return 1;
        }
    }
    int count = converter(inputFile, outputFile);
    if (!onlycount)
    {
        Json::Value info;
        if (waitsync) info["waitsync"] = count;
        addConversionEntry(header, "converter", source_trace_filename, info);
        Json::FastWriter writer;
        const std::string json_header = writer.write(header);
        outputFile.mHeader.jsonLength = json_header.size();
        outputFile.WriteHeader(json_header.c_str(), json_header.size());
    }
    inputFile.close();
    outputFile.Close();
    return 0;
}
