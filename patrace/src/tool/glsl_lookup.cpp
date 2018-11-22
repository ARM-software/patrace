#include "glsl_lookup.h"

static std::unordered_map<std::string, KeywordDefinition> keywords =
{
    { "const", { Keyword::Const, KeywordType::Qualifier, GL_NONE } },
    { "uniform", { Keyword::Uniform, KeywordType::Storage, GL_NONE } },
    { "buffer", { Keyword::Buffer, KeywordType::Storage, GL_NONE } },
    { "shared", { Keyword::Shared, KeywordType::Qualifier, GL_NONE } },
    { "coherent", { Keyword::Coherent, KeywordType::Qualifier, GL_NONE } },
    { "volatile", { Keyword::Volatile, KeywordType::Qualifier, GL_NONE } },
    { "restrict", { Keyword::Restrict, KeywordType::Qualifier, GL_NONE } },
    { "readonly", { Keyword::Readonly, KeywordType::Qualifier, GL_NONE } },
    { "writeonly", { Keyword::Writeonly, KeywordType::Qualifier, GL_NONE } },
    { "atomic_uint", { Keyword::Atomic_uint, KeywordType::Storage, GL_NONE } },
    { "layout", { Keyword::Layout, KeywordType::Qualifier, GL_NONE } },
    { "centroid", { Keyword::Centroid, KeywordType::Qualifier, GL_NONE } },
    { "flat", { Keyword::Flat, KeywordType::Qualifier, GL_NONE } },
    { "smooth", { Keyword::Smooth, KeywordType::Qualifier, GL_NONE } },
    { "patch", { Keyword::Patch, KeywordType::Qualifier, GL_NONE } },
    { "sample", { Keyword::Sample, KeywordType::Qualifier, GL_NONE } },
    { "precise", { Keyword::Precise, KeywordType::Qualifier, GL_NONE } },
    { "break", { Keyword::Break, KeywordType::Flow, GL_NONE } },
    { "continue", { Keyword::Continue, KeywordType::Flow, GL_NONE } },
    { "do", { Keyword::Do, KeywordType::Flow, GL_NONE } },
    { "for", { Keyword::For, KeywordType::Flow, GL_NONE } },
    { "while", { Keyword::While, KeywordType::Flow, GL_NONE } },
    { "switch", { Keyword::Switch, KeywordType::Flow, GL_NONE } },
    { "case", { Keyword::Case, KeywordType::Flow, GL_NONE } },
    { "default", { Keyword::Default, KeywordType::Flow, GL_NONE } },
    { "if", { Keyword::If, KeywordType::Flow, GL_NONE } },
    { "else", { Keyword::Else, KeywordType::Flow, GL_NONE } },
    { "in", { Keyword::In, KeywordType::Storage, GL_NONE } },
    { "out", { Keyword::Out, KeywordType::Storage, GL_NONE } },
    { "inout", { Keyword::Inout, KeywordType::Storage, GL_NONE } },
    { "float", { Keyword::Float, KeywordType::Type, GL_FLOAT } },
    { "int", { Keyword::Int, KeywordType::Type, GL_INT } },
    { "void", { Keyword::Void, KeywordType::Type, GL_NONE } },
    { "bool", { Keyword::Bool, KeywordType::Type, GL_BOOL } },
    { "true", { Keyword::True, KeywordType::None, GL_TRUE } },
    { "false", { Keyword::False, KeywordType::None, GL_FALSE } },
    { "invariant", { Keyword::Invariant, KeywordType::Qualifier, GL_NONE } },
    { "discard", { Keyword::Discard, KeywordType::Flow, GL_NONE } },
    { "return", { Keyword::Return, KeywordType::Flow, GL_NONE } },
    { "mat2", { Keyword::Mat2, KeywordType::Type, GL_FLOAT_MAT2 } },
    { "mat3", { Keyword::Mat3, KeywordType::Type, GL_FLOAT_MAT3 } },
    { "mat4", { Keyword::Mat4, KeywordType::Type, GL_FLOAT_MAT4 } },
    { "mat2x2", { Keyword::Mat2x2, KeywordType::Type, GL_FLOAT_MAT2 } }, // alias
    { "mat2x3", { Keyword::Mat2x3, KeywordType::Type, GL_FLOAT_MAT2x3 } },
    { "mat2x4", { Keyword::Mat2x4, KeywordType::Type, GL_FLOAT_MAT2x4 } },
    { "mat3x2", { Keyword::Mat3x2, KeywordType::Type, GL_FLOAT_MAT3x2 } },
    { "mat3x3", { Keyword::Mat3x3, KeywordType::Type, GL_FLOAT_MAT3 } }, // alias
    { "mat3x4", { Keyword::Mat3x4, KeywordType::Type, GL_FLOAT_MAT3x4 } },
    { "mat4x2", { Keyword::Mat4x2, KeywordType::Type, GL_FLOAT_MAT4x2 } },
    { "mat4x3", { Keyword::Mat4x3, KeywordType::Type, GL_FLOAT_MAT4x3 } },
    { "mat4x4", { Keyword::Mat4x4, KeywordType::Type, GL_FLOAT_MAT4 } }, // alias
    { "vec2", { Keyword::Vec2, KeywordType::Type, GL_FLOAT_VEC2 } },
    { "vec3", { Keyword::Vec3, KeywordType::Type, GL_FLOAT_VEC3 } },
    { "vec4", { Keyword::Vec4, KeywordType::Type, GL_FLOAT_VEC4 } },
    { "ivec2", { Keyword::Ivec2, KeywordType::Type, GL_INT_VEC2 } },
    { "ivec3", { Keyword::Ivec3, KeywordType::Type, GL_INT_VEC3 } },
    { "ivec4", { Keyword::Ivec4, KeywordType::Type, GL_INT_VEC4 } },
    { "bvec2", { Keyword::Bvec2, KeywordType::Type, GL_BOOL_VEC2 } },
    { "bvec3", { Keyword::Bvec3, KeywordType::Type, GL_BOOL_VEC3 } },
    { "bvec4", { Keyword::Bvec4, KeywordType::Type, GL_BOOL_VEC4 } },
    { "uint", { Keyword::Uint, KeywordType::Type, GL_UNSIGNED_INT } },
    { "uvec2", { Keyword::Uvec2, KeywordType::Type, GL_UNSIGNED_INT_VEC2 } },
    { "uvec3", { Keyword::Uvec3, KeywordType::Type, GL_UNSIGNED_INT_VEC3 } },
    { "uvec4", { Keyword::Uvec4, KeywordType::Type, GL_UNSIGNED_INT_VEC4 } },
    { "lowp", { Keyword::Lowp, KeywordType::Qualifier, GL_NONE } },
    { "mediump", { Keyword::Mediump, KeywordType::Qualifier, GL_NONE } },
    { "highp", { Keyword::Highp, KeywordType::Qualifier, GL_NONE } },
    { "precision", { Keyword::Precision, KeywordType::Qualifier, GL_NONE } },
    { "sampler2D", { Keyword::Sampler2D, KeywordType::Type, GL_SAMPLER_2D } },
    { "sampler3D", { Keyword::Sampler3D, KeywordType::Type, GL_SAMPLER_3D } },
    { "samplerCube", { Keyword::SamplerCube, KeywordType::Type, GL_SAMPLER_CUBE } },
    { "sampler2DShadow", { Keyword::Sampler2DShadow, KeywordType::Type, GL_SAMPLER_2D_SHADOW } },
    { "samplerCubeShadow", { Keyword::SamplerCubeShadow, KeywordType::Type, GL_SAMPLER_CUBE_SHADOW } },
    { "sampler2DArray", { Keyword::Sampler2DArray, KeywordType::Type, GL_SAMPLER_2D_ARRAY } },
    { "sampler2DArrayShadow", { Keyword::Sampler2DArrayShadow, KeywordType::Type, GL_SAMPLER_2D_ARRAY_SHADOW } },
    { "isampler2D", { Keyword::Isampler2D, KeywordType::Type, GL_INT_SAMPLER_2D } },
    { "isampler3D", { Keyword::Isampler3D, KeywordType::Type, GL_INT_SAMPLER_3D } },
    { "isamplerCube", { Keyword::IsamplerCube, KeywordType::Type, GL_INT_SAMPLER_CUBE } },
    { "isampler2DArray", { Keyword::Isampler2DArray, KeywordType::Type, GL_INT_SAMPLER_2D_ARRAY } },
    { "usampler2D", { Keyword::Usampler2D, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_2D } },
    { "usampler3D", { Keyword::Usampler3D, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_3D } },
    { "usamplerCube", { Keyword::UsamplerCube, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_CUBE } },
    { "usampler2DArray", { Keyword::Usampler2DArray, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_2D_ARRAY } },
    { "sampler2DMS", { Keyword::Sampler2DMS, KeywordType::Type, GL_SAMPLER_2D_MULTISAMPLE } },
    { "isampler2DMS", { Keyword::Isampler2DMS, KeywordType::Type, GL_INT_SAMPLER_2D_MULTISAMPLE } },
    { "usampler2DMS", { Keyword::Usampler2DMS, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE } },
    { "samplerBuffer", { Keyword::SamplerBuffer, KeywordType::Type, GL_SAMPLER_BUFFER } },
    { "isamplerBuffer", { Keyword::IsamplerBuffer, KeywordType::Type, GL_INT_SAMPLER_BUFFER } },
    { "usamplerBuffer", { Keyword::UsamplerBuffer, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_BUFFER } },
    { "imageBuffer", { Keyword::ImageBuffer, KeywordType::Type, GL_IMAGE_BUFFER } },
    { "iimageBuffer", { Keyword::IimageBuffer, KeywordType::Type, GL_INT_IMAGE_BUFFER } },
    { "uimageBuffer", { Keyword::UimageBuffer, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_BUFFER } },
    { "imageCubeArray", { Keyword::ImageCubeArray, KeywordType::Type, GL_IMAGE_CUBE_MAP_ARRAY } },
    { "iimageCubeArray", { Keyword::IimageCubeArray, KeywordType::Type, GL_INT_IMAGE_CUBE_MAP_ARRAY } },
    { "uimageCubeArray", { Keyword::UimageCubeArray, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY } },
    { "samplerCubeArray", { Keyword::SamplerCubeArray, KeywordType::Type, GL_SAMPLER_CUBE_MAP_ARRAY_EXT } },
    { "isamplerCubeArray", { Keyword::IsamplerCubeArray, KeywordType::Type, GL_INT_SAMPLER_CUBE_MAP_ARRAY_EXT } },
    { "usamplerCubeArray", { Keyword::UsamplerCubeArray, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_EXT } },
    { "samplerCubeArrayShadow", { Keyword::SamplerCubeArrayShadow, KeywordType::Type, GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_EXT } },
    { "sampler2DMSArray", { Keyword::Sampler2DMSArray, KeywordType::Type, GL_SAMPLER_2D_MULTISAMPLE_ARRAY_OES } },
    { "isampler2DMSArray", { Keyword::Isampler2DMSArray, KeywordType::Type, GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES } },
    { "usampler2DMSArray", { Keyword::Usampler2DMSArray, KeywordType::Type, GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES } },
    { "image2DArray", { Keyword::Image2DArray, KeywordType::Type, GL_IMAGE_2D_ARRAY } },
    { "iimage2DArray", { Keyword::Iimage2DArray, KeywordType::Type, GL_INT_IMAGE_2D_ARRAY } },
    { "uimage2DArray", { Keyword::Uimage2DArray, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_2D_ARRAY } },
    { "image2D", { Keyword::Image2D, KeywordType::Type, GL_IMAGE_2D } },
    { "iimage2D", { Keyword::Iimage2D, KeywordType::Type, GL_INT_IMAGE_2D } },
    { "uimage2D", { Keyword::Uimage2D, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_2D } },
    { "image3D", { Keyword::Image3D, KeywordType::Type, GL_IMAGE_3D } },
    { "iimage3D", { Keyword::Iimage3D, KeywordType::Type, GL_INT_IMAGE_3D } },
    { "uimage3D", { Keyword::Uimage3D, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_3D } },
    { "imageCube", { Keyword::ImageCube, KeywordType::Type, GL_IMAGE_CUBE } },
    { "iimageCube", { Keyword::IimageCube, KeywordType::Type, GL_IMAGE_CUBE } },
    { "uimageCube", { Keyword::UimageCube, KeywordType::Type, GL_UNSIGNED_INT_IMAGE_CUBE } },
    { "struct", { Keyword::Struct, KeywordType::Qualifier, GL_NONE } },
    { "attribute", { Keyword::Attribute, KeywordType::Storage, GL_NONE } },
    { "varying", { Keyword::Varying, KeywordType::Storage, GL_NONE } },

    // ... and there's a bunch of other keywords reserved for future use that we'll ignore for now
    // their keyword type is a "best guess" at best in most cases
    { "resource", { Keyword::Resource, KeywordType::Storage, GL_NONE } },
    { "noperspective", { Keyword::Noperspective, KeywordType::Qualifier, GL_NONE } },
    { "subroutine", { Keyword::Subroutine, KeywordType::Flow, GL_NONE } },
    { "common", { Keyword::Common, KeywordType::Qualifier, GL_NONE } },
    { "partition", { Keyword::Partition, KeywordType::Qualifier, GL_NONE } },
    { "active", { Keyword::Active, KeywordType::Qualifier, GL_NONE } },
    { "asm", { Keyword::Asm, KeywordType::Flow, GL_NONE } },

    { "class", { Keyword::Class, KeywordType::Storage, GL_NONE } },
    { "union", { Keyword::Union, KeywordType::Storage, GL_NONE } },
    { "enum", { Keyword::Enum, KeywordType::Storage, GL_NONE } },
    { "typedef", { Keyword::Typedef, KeywordType::Storage, GL_NONE } },
    { "template", { Keyword::Template, KeywordType::Storage, GL_NONE } },
    { "this", { Keyword::This, KeywordType::None, GL_NONE } },
    { "goto", { Keyword::Goto, KeywordType::Flow, GL_NONE } },
    { "inline", { Keyword::Inline, KeywordType::Qualifier, GL_NONE } },
    { "noinline", { Keyword::Noinline, KeywordType::Qualifier, GL_NONE } },
    { "public", { Keyword::Public, KeywordType::Qualifier, GL_NONE } },
    { "static", { Keyword::Static, KeywordType::Qualifier, GL_NONE } },
    { "extern", { Keyword::Extern, KeywordType::Qualifier, GL_NONE } },
    { "external", { Keyword::External, KeywordType::Qualifier, GL_NONE } },
    { "interface", { Keyword::Interface, KeywordType::Qualifier, GL_NONE } },
    { "long", { Keyword::Long, KeywordType::Type, GL_NONE } },
    { "short", { Keyword::Short, KeywordType::Type, GL_SHORT } },
    { "double", { Keyword::Double, KeywordType::Type, GL_NONE } },
    { "half", { Keyword::Half, KeywordType::Type, GL_HALF_FLOAT } },
    { "fixed", { Keyword::Fixed, KeywordType::Qualifier, GL_FIXED } },
    { "unsigned", { Keyword::Unsigned, KeywordType::Type, GL_NONE } },
    { "superp", { Keyword::Superp, KeywordType::Qualifier, GL_NONE } },
    { "input", { Keyword::Input, KeywordType::Qualifier, GL_NONE } },
    { "output", { Keyword::Output, KeywordType::Qualifier, GL_NONE } },
    { "hvec2", { Keyword::Hvec2, KeywordType::Type, GL_NONE } },
    { "hvec3", { Keyword::Hvec3, KeywordType::Type, GL_NONE } },
    { "hvec4", { Keyword::Hvec4, KeywordType::Type, GL_NONE } },
    { "dvec2", { Keyword::Dvec2, KeywordType::Type, GL_NONE } },
    { "dvec3", { Keyword::Dvec3, KeywordType::Type, GL_NONE } },
    { "dvec4", { Keyword::Dvec4, KeywordType::Type, GL_NONE } },
    { "fvec2", { Keyword::Fvec2, KeywordType::Type, GL_NONE } },
    { "fvec3", { Keyword::Fvec3, KeywordType::Type, GL_NONE } },
    { "fvec4", { Keyword::Fvec4, KeywordType::Type, GL_NONE } },
    { "sampler3DRect", { Keyword::Sampler3DRect, KeywordType::Type, GL_NONE } },
    { "filter", { Keyword::Filter, KeywordType::Qualifier, GL_NONE } },
    { "image1D", { Keyword::Image1D, KeywordType::Type, GL_NONE } },
    { "iimage1D", { Keyword::Iimage1D, KeywordType::Type, GL_NONE } },
    { "uimage1D", { Keyword::Uimage1D, KeywordType::Type, GL_NONE } },
    { "iimage1DArray", { Keyword::Iimage1DArray, KeywordType::Type, GL_NONE } },
    { "uimage1DArray", { Keyword::Uimage1DArray, KeywordType::Type, GL_NONE } },
    { "sampler1D", { Keyword::Sampler1D, KeywordType::Type, GL_NONE } },
    { "sampler1DShadow", { Keyword::Sampler1DShadow, KeywordType::Type, GL_NONE } },
    { "sampler1DArray", { Keyword::Sampler1DArray, KeywordType::Type, GL_NONE } },
    { "sampler1DArrayShadow", { Keyword::Sampler1DArrayShadow, KeywordType::Type, GL_NONE } },
    { "isampler1D", { Keyword::Isampler1D, KeywordType::Type, GL_NONE } },
    { "isampler1DArray", { Keyword::Isampler1DArray, KeywordType::Type, GL_NONE } },
    { "usampler1D", { Keyword::Usampler1D, KeywordType::Type, GL_NONE } },
    { "usampler1DArray", { Keyword::Usampler1DArray, KeywordType::Type, GL_NONE } },
    { "sampler2DRect", { Keyword::Sampler2DRect, KeywordType::Type, GL_NONE } },
    { "sampler2DRectShadow", { Keyword::Sampler2DRectShadow, KeywordType::Type, GL_NONE } },
    { "isampler2DRect", { Keyword::Isampler2DRect, KeywordType::Type, GL_NONE } },
    { "usampler2DRect", { Keyword::Usampler2DRect, KeywordType::Type, GL_NONE } },
    { "sizeof", { Keyword::Sizeof, KeywordType::None, GL_NONE } },
    { "cast", { Keyword::Cast, KeywordType::Qualifier, GL_NONE } },
    { "namespace", { Keyword::Namespace, KeywordType::None, GL_NONE } },
    { "using", { Keyword::Using, KeywordType::None, GL_NONE } },
    { "dmat2", { Keyword::Dmat2, KeywordType::Type, GL_NONE } },
    { "dmat3", { Keyword::Dmat3, KeywordType::Type, GL_NONE } },
    { "dmat4", { Keyword::Dmat4, KeywordType::Type, GL_NONE } },
    { "dmat2x2", { Keyword::Dmat2x2, KeywordType::Type, GL_NONE } },
    { "dmat2x3", { Keyword::Dmat2x3, KeywordType::Type, GL_NONE } },
    { "dmat2x4", { Keyword::Dmat2x4, KeywordType::Type, GL_NONE } },
    { "dmat3x2", { Keyword::Dmat3x2, KeywordType::Type, GL_NONE } },
    { "dmat3x3", { Keyword::Dmat3x3, KeywordType::Type, GL_NONE } },
    { "dmat3x4", { Keyword::Dmat3x4, KeywordType::Type, GL_NONE } },
    { "dmat4x2", { Keyword::Dmat4x2, KeywordType::Type, GL_NONE } },
    { "dmat4x3", { Keyword::Dmat4x3, KeywordType::Type, GL_NONE } },
    { "dmat4x4", { Keyword::Dmat4x4, KeywordType::Type, GL_NONE } },
    { "image2DRect", { Keyword::Image2DRect, KeywordType::Type, GL_NONE } },
    { "iimage2DRect", { Keyword::Iimage2DRect, KeywordType::Type, GL_NONE } },
    { "uimage2DRect", { Keyword::Uimage2DRect, KeywordType::Type, GL_NONE } },
    { "image2DMS", { Keyword::Image2DMS, KeywordType::Type, GL_NONE } },
    { "iimage2DMS", { Keyword::Iimage2DMS, KeywordType::Type, GL_NONE } },
    { "uimage2DMS", { Keyword::Uimage2DMS, KeywordType::Type, GL_NONE } },
    { "image2DMSArray", { Keyword::Image2DMSArray, KeywordType::Type, GL_NONE } },
    { "iimage2DMSArray", { Keyword::Iimage2DMSArray, KeywordType::Type, GL_NONE } },
    { "uimage2DMSArray", { Keyword::Uimage2DMSArray, KeywordType::Type, GL_NONE } },
};

static std::unordered_map<Keyword, GLenum, EnumClassHash> enum_mapping;

GLenum lookup_get_gles_type(Keyword key)
{
    if (enum_mapping.size() == 0) // only ever do this once
    {
        for (const auto& pair : keywords)
        {
            enum_mapping[pair.second.keyword] = pair.second.glesType;
        }
    }
    return enum_mapping.at(key);
}

const std::string& lookup_get_string(Keyword key)
{
    for (const auto& pair : keywords)
    {
        if (pair.second.keyword == key)
        {
            return pair.first;
        }
    }
    static std::string empty;
    return empty;
}

KeywordDefinition lookup_get(const std::string& key)
{
    if (keywords.count(key)) return keywords.at(key);
    else return { Keyword::None, KeywordType::None };
}

bool lookup_is_sampler(Keyword key)
{
    switch (key)
    {
    case Keyword::Sampler2D:
    case Keyword::Sampler3D:
    case Keyword::SamplerCube:
    case Keyword::Sampler2DShadow:
    case Keyword::SamplerCubeShadow:
    case Keyword::Sampler2DArray:
    case Keyword::Sampler2DArrayShadow:
    case Keyword::Isampler2D:
    case Keyword::Isampler3D:
    case Keyword::IsamplerCube:
    case Keyword::Isampler2DArray:
    case Keyword::Usampler2D:
    case Keyword::Usampler3D:
    case Keyword::UsamplerCube:
    case Keyword::Usampler2DArray:
    case Keyword::Sampler2DMS:
    case Keyword::Isampler2DMS:
    case Keyword::Usampler2DMS:
    case Keyword::SamplerBuffer:
    case Keyword::IsamplerBuffer:
    case Keyword::UsamplerBuffer:
    case Keyword::SamplerCubeArray:
    case Keyword::IsamplerCubeArray:
    case Keyword::UsamplerCubeArray:
    case Keyword::SamplerCubeArrayShadow:
    case Keyword::Sampler2DMSArray:
    case Keyword::Isampler2DMSArray:
    case Keyword::Usampler2DMSArray:
    case Keyword::Sampler3DRect:
    case Keyword::Sampler1D:
    case Keyword::Sampler1DShadow:
    case Keyword::Sampler1DArray:
    case Keyword::Sampler1DArrayShadow:
    case Keyword::Isampler1D:
    case Keyword::Isampler1DArray:
    case Keyword::Usampler1D:
    case Keyword::Usampler1DArray:
    case Keyword::Sampler2DRect:
    case Keyword::Sampler2DRectShadow:
    case Keyword::Isampler2DRect:
    case Keyword::Usampler2DRect:
        return true;
    default:
        return false;
    }
    return false; // make compiler happy
}
