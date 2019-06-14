#include <utility>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>

#include "tool/parse_interface.h"
#include "tool/glsl_parser.h"
#include "tool/glsl_utils.h"

static void printHelp()
{
    std::cout <<
        "Usage : shader_analyzer [OPTIONS] shader.ext\n"
        "Options:\n"
        "  --compress    Remove comments, preprocess and remove unnecessary whitespace\n"
        "  --preprocess  Preprocess shader\n"
        "  --strip       Strip comments\n"
        "  --analyze     Print analysis\n"
        "  --debug       Enable debug output\n"
        "  --inline      Inline data from any (non-standard) #include directives\n"
        "  --selftest    Run internal self-tests\n"
        "  --test        Test parser on a shader, printing only file name and shader type\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

void printshader(const GLSLRepresentation::Variable& i, int pad = 1)
{
    printf("%s(%s)[%s]%s %s%s",
           std::string(pad, '\t').c_str(),
           lookup_get_string(i.precision).c_str(),
           lookup_get_string(i.storage).c_str(),
           lookup_get_string(i.type).c_str(),
           i.name.c_str(),
           i.dimensions.c_str()
          );
    for (const auto& v : i.qualifiers)
    {
        printf(" %s", lookup_get_string(v).c_str());
    }
    printf(" %s", i.layout.c_str());
    printf("\n");
    pad++;
    for (const GLSLRepresentation::Variable& v : i.members)
    {
        assert(&v != &i);
        printshader(v, pad);
    }
}

enum Mode { NONE, COMPRESS, PREPROCESS, STRIP, ANALYZE, TEST };

int main(int argc, char **argv)
{
    int argIndex = 1;
    Mode mode = NONE;
    bool debug = false;
    bool inline_includes = false;

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
        else if (arg == "--debug")
        {
            debug = true;
        }
        else if (arg == "--inline")
        {
            inline_includes = true;
        }
        else if (arg == "--test")
        {
            if (mode != NONE) { fprintf(stderr, "Incompatible parameters"); return -1; }
            mode = TEST;
        }
        else if (arg == "--compress")
        {
            if (mode != NONE) { fprintf(stderr, "Incompatible parameters"); return -1; }
            mode = COMPRESS;
        }
        else if (arg == "--analyze")
        {
            if (mode != NONE) { fprintf(stderr, "Incompatible parameters"); return -1; }
            mode = ANALYZE;
        }
        else if (arg == "--preprocess")
        {
            if (mode != NONE) { fprintf(stderr, "Incompatible parameters"); return -1; }
            mode = PREPROCESS;
        }
        else if (arg == "--strip")
        {
            if (mode != NONE) { fprintf(stderr, "Incompatible parameters"); return -1; }
            mode = STRIP;
        }
        else if (arg == "--selftest")
        {
            GLSLParser parser;
            parser.self_test();
            return 0;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 1;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if (argc < argIndex + 1)
    {
        printHelp();
        return 1;
    }

    std::string filename = argv[argIndex++];
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp)
    {
        fprintf(stderr, "Failed to open \"%s\": %s\n", filename.c_str(), strerror(errno));
        return -1;
    }
    std::string data;
    fseek(fp, 0, SEEK_END);
    data.resize(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    size_t r = fread(&data[0], data.size(), 1, fp);
    if (r != 1)
    {
        fprintf(stderr, "Failed to read \"%s\"\n", filename.c_str());
        return -2;
    }
    fclose(fp);
    GLSLParser parser(filename, debug);
    int shaderType = parser.shaderType(strrchr(filename.c_str(), '.'));
    if (mode == TEST)
    {
        printf("%s [type=0x%04x]\n", filename.c_str(), (unsigned)shaderType);
    }
    if (inline_includes)
    {
        data = parser.inline_includes(data);
    }
    std::string pass1 = parser.strip_comments(data);

    if (mode == STRIP)
    {
        printf("%s\n", pass1.c_str());
        return 0;
    }

    GLSLShader s = parser.preprocessor(pass1, shaderType);

    if (mode == COMPRESS || mode == PREPROCESS)
    {
        printf("#version %d%s\n", s.version, s.version > 100 ? " es" : "");
        for (const std::string& ext : s.extensions) printf("#extension %s : enable\n", ext.c_str());
    }

    if (mode == COMPRESS)
    {
       std::string c = parser.compressed(s);
       printf("%s\n", c.c_str());
    }
    else if (mode == PREPROCESS)
    {
        printf("%s\n", s.code.c_str());
    }
    else if (mode == ANALYZE || mode == TEST)
    {
        GLSLRepresentation r = parser.parse(s);
        if (mode == TEST)
        {
            return 0;
        }
        printf("Interface:\n");
        for (const auto& i : r.global.members)
        {
            printshader(i);
        }
        if (shaderType == GL_FRAGMENT_SHADER)
        {
            printf("Counts:\n");
            printf("\tVarying locations: %d\n", count_varying_locations_used(r));
            printf("\tLowp varyings: %d\n", count_varyings_by_precision(r, Keyword::Lowp));
            printf("\tMediump varyings: %d\n", count_varyings_by_precision(r, Keyword::Mediump));
            printf("\tHighp varyings: %d\n", count_varyings_by_precision(r, Keyword::Highp));
        }
    }

    return 0;
}
