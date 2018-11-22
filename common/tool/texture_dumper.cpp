#include "eglstate/context.hpp"
#include "tool/trace_interface.hpp"
#include "tool/config.hpp"
using namespace pat;

extern "C"

class bufferSaver
{
public:
    bufferSaver() {}
    ~bufferSaver()
    {
        for (auto it : _bufferObjects)
            if (it.second) delete []it.second;
    }

    void bindBuffer(CallInterface *call)
    {
        const unsigned int target = call->arg_to_uint(0);
        const unsigned int buffer = call->arg_to_uint(1);
        if (target == 0x88EC) //GL_PIXEL_UNPACK_BUFFER
        {
            setActiveBuffer(buffer);
        }
    }

    void bufferData(CallInterface *call)
    {
        const unsigned int target = call->arg_to_uint(0);
        const unsigned int size = call->arg_to_uint(1);
        bool bufferObject = false;
        unsigned char *pixels = call->arg_to_pointer(2, &bufferObject);
        if (target == 0x88EC && pixels) //GL_PIXEL_UNPACK_BUFFER
        {
            unsigned int buffer = getActiveBuffer();
            BufferObjectMap::iterator it = _bufferObjects.find(buffer);
            if (it != _bufferObjects.end())
            {
                if (it->second) delete []it->second;
                _bufferObjects.erase(it);
            }

            char *p = new char[size];
            memcpy(p, pixels, size);
            _bufferObjects[buffer] = p;
        }
    }

    unsigned char* queryBufferData(const unsigned int buffer)
    {
        BufferObjectMap::iterator it = _bufferObjects.find(buffer);
        if (it != _bufferObjects.end())
        {
            return (unsigned char*)it->second;
        }

        return NULL;
    }

    void setActiveBuffer(unsigned int buffer) {activeBufferObject = buffer;}
    unsigned int getActiveBuffer() { return activeBufferObject;}

private:
    typedef std::map<int, char*> BufferObjectMap;
    BufferObjectMap _bufferObjects;
    unsigned int activeBufferObject;
};

class BufferMangerForThread
{
public:
    BufferMangerForThread() {}
    ~BufferMangerForThread()
    {
        for (BufferThreadMap::iterator it = gContexts.begin(); it != gContexts.end() && it->second; ++it)
            delete it->second;
    }

    bufferSaver* GetBufferMangerForThread(UInt32 threadID)
    {
        BufferThreadMap::iterator found = gContexts.find(threadID);
        if (found == gContexts.end())
        {
            bufferSaver  *ptr = new bufferSaver();
            gContexts[threadID] = ptr;
            return ptr;
        }
        else
        {
            return found->second;
        }
    }

private:
    typedef std::map<UInt32, bufferSaver*> BufferThreadMap;
    BufferThreadMap gContexts;
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Please specify a trace file or -h for help. Exit.\n");
        exit(1);
    }

    if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        fprintf(stderr, "Usage: %s <trace file>\n\n", argv[0]);
        fprintf(stderr, " -h output this help text\n");
        fprintf(stderr, " -v output the version\n");
        fprintf(stderr, "\nDumps the textures in a trace file.\n");
        exit(0);
    }

    if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
    {
        fprintf(stderr, "Version: " PATRACE_VERSION);
        exit(0);
    }

    const char *trace_filename = argv[1];

    std::shared_ptr<InputFileInterface> inputFile(GenerateInputFile());
    if (!inputFile->open(trace_filename))
    {
        PAT_DEBUG_LOG("Error : failed to open %s\n", trace_filename);
        return 1;
    }

    BufferMangerForThread bufferManagerForThread;
    CallInterface *call = NULL;
    while ((call = inputFile->next_call()))
    {
        const unsigned int thread = call->GetThreadID();
        const unsigned int callNum = call->GetNumber();
        pat::ContextPtr context = pat::GetStateMangerForThread(thread);
        context->SetCurrentCallNumber(callNum);
        bufferSaver *_bufferSaver = bufferManagerForThread.GetBufferMangerForThread(thread);

        if (strcmp(call->GetName(), "glGenTextures") == 0)
        {
            const unsigned int n = call->arg_to_uint(0);
            for (unsigned int i = 0; i < n; i++)
            {
                const unsigned int tex = call->array_arg_to_uint(1, i);
                context->CreateTextureObject(tex);
            }
        }
        else if (strcmp(call->GetName(), "glActiveTexture") == 0)
        {
            const unsigned int unit = call->arg_to_uint(0);
            context->SetActiveTextureUnit(unit);
        }
        else if (strcmp(call->GetName(), "glBindTexture") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int handle = call->arg_to_uint(1);
            if (handle == 0) continue;
            context->BindTextureObject(target, handle);
        }
        else if (strcmp(call->GetName(), "glTexImage2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int level = call->arg_to_uint(1);
            const unsigned int width = call->arg_to_uint(3);
            const unsigned int height = call->arg_to_uint(4);
            const unsigned int format = call->arg_to_uint(6);
            const unsigned int type = call->arg_to_uint(7);
            bool bufferObject = false;
            unsigned char *pixels = call->arg_to_pointer(8, &bufferObject);
            context->SetTexImage(target, level, format, type, width, height, pixels);
        }
        else if (strcmp(call->GetName(), "glTexSubImage2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int level = call->arg_to_uint(1);
            const unsigned int xoffset = call->arg_to_uint(2);
            const unsigned int yoffset = call->arg_to_uint(3);
            const unsigned int width = call->arg_to_uint(4);
            const unsigned int height = call->arg_to_uint(5);
            const unsigned int format = call->arg_to_uint(6);
            const unsigned int type = call->arg_to_uint(7);
            bool bufferObject = false;
            unsigned char *pixels = call->arg_to_pointer(8, &bufferObject);
            if (bufferObject)
            {
                pixels = _bufferSaver->queryBufferData(_bufferSaver->getActiveBuffer());
            }
            context->SetTexSubImage(target, level, format, type, xoffset, yoffset, width, height, pixels);
        }
        else if (strcmp(call->GetName(), "glCompressedTexImage2D") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            const unsigned int level = call->arg_to_uint(1);
            const unsigned int format = call->arg_to_uint(2);
            const unsigned int width = call->arg_to_uint(3);
            const unsigned int height = call->arg_to_uint(4);
            const unsigned int dataSize = call->arg_to_uint(6);
            bool bufferObject = false;
            unsigned char *pixels = call->arg_to_pointer(7, &bufferObject);
            context->SetCompressedTexImage(target, level, format, width, height, dataSize, pixels);
        }
        else if (strcmp(call->GetName(), "glBindBuffer") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            if (target == 0x88EC) //GL_PIXEL_UNPACK_BUFFER
            {
                _bufferSaver->bindBuffer(call);
            }
        }
        else if (strcmp(call->GetName(), "glBufferData") == 0)
        {
            const unsigned int target = call->arg_to_uint(0);
            if (target == 0x88EC) //GL_PIXEL_UNPACK_BUFFER
            {
                _bufferSaver->bufferData(call);
            }
        }
        delete call;
    }
    inputFile->close();

    const pat::ContextPtrList contexts = pat::GetAllContexts();
    pat::ContextPtrList::const_iterator citer = contexts.begin();
    for (; citer != contexts.end(); ++citer)
    {
        (*citer)->DumpTextureObjects();
    }
}
