#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "glsl_lookup.h"

/// Contains preprocessed shader (output after GLSL spec step 9)
struct GLSLShader
{
    std::string code;
    std::vector<std::string> extensions;
    int version = 100; // default
    bool contains_optimize_off_pragma = false;
    bool contains_debug_on_pragma = false;
    bool contains_invariant_all_pragma = false;
    int shaderType = 0;

    /// Convert shader to 310-compatible version
    void upgrade_to_glsl310();
};

/// Contains a semantic understanding of the contents of the shader
struct GLSLRepresentation
{
    struct Variable
    {
        Keyword precision = Keyword::None;
        Keyword type = Keyword::None;
        Keyword storage = Keyword::None;
        std::string name;
        int size = 0; // array size, if any
        std::string dimensions;
        std::vector<Keyword> qualifiers;
        std::string layout;
        std::vector<Variable> members;
        int binding = -1; // TBD
    };

    bool contains_invariants = false;
    Variable global;
};

struct Define
{
    std::string value;
    std::vector<std::string> params;
};

/// GLSL parser toolkit
class GLSLParser
{
public:
    GLSLParser(const std::string& name = std::string(), bool debug = false) : mShaderName(name), mDebug(debug) {}
    GLSLRepresentation parse(const GLSLShader& shader);
    GLSLShader preprocessor(std::string shader, int shaderType);
    std::string strip_comments(const std::string& s);
    std::string compressed(GLSLShader shader);
    std::string inline_includes(const std::string& s);

    void self_test();
    int shaderType(const std::string& extension);

private:
    bool resolve_conditionals(const std::unordered_map<std::string, Define>& defines, const std::string& expr, int shaderType);
    int mLineNo = 0;
    std::string mShaderName;
    bool mDebug = false;
};
