#pragma once

#include <unordered_map>
#include <string>
#include "dispatch/eglimports.hpp"

// Bad X11, bad!!!
#ifdef None
#undef None
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif

// Workaround to fix bug in c++11 standard. Fixed in c++14 but can't wait.
struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

enum class KeywordType
{
    Qualifier,
    Type,
    Storage,
    Flow,
    None
};

enum class Keyword
{
    Const,
    Uniform,
    Buffer,
    Shared,
    Coherent,
    Volatile,
    Restrict,
    Readonly,
    Writeonly,
    Atomic_uint,
    Layout,
    Centroid,
    Flat,
    Smooth,
    Patch,
    Sample,
    Precise,
    Break,
    Continue,
    Do,
    For,
    While,
    Switch,
    Case,
    Default,
    If,
    Else,
    In,
    Out,
    Inout,
    Float,
    Int,
    Void,
    Bool,
    True,
    False,
    Invariant,
    Discard,
    Return,
    Mat2,
    Mat3,
    Mat4,
    Mat2x2,
    Mat2x3,
    Mat2x4,
    Mat3x2,
    Mat3x3,
    Mat3x4,
    Mat4x2,
    Mat4x3,
    Mat4x4,
    Vec2,
    Vec3,
    Vec4,
    Ivec2,
    Ivec3,
    Ivec4,
    Bvec2,
    Bvec3,
    Bvec4,
    Uint,
    Uvec2,
    Uvec3,
    Uvec4,
    Lowp,
    Mediump,
    Highp,
    Precision,
    Sampler2D,
    Sampler3D,
    SamplerCube,
    Sampler2DShadow,
    SamplerCubeShadow,
    Sampler2DArray,
    Sampler2DArrayShadow,
    Isampler2D,
    Isampler3D,
    IsamplerCube,
    Isampler2DArray,
    Usampler2D,
    Usampler3D,
    UsamplerCube,
    Usampler2DArray,
    Sampler2DMS,
    Isampler2DMS,
    Usampler2DMS,
    SamplerBuffer,
    IsamplerBuffer,
    UsamplerBuffer,
    ImageBuffer,
    IimageBuffer,
    UimageBuffer,
    ImageCubeArray,
    IimageCubeArray,
    UimageCubeArray,
    SamplerCubeArray,
    IsamplerCubeArray,
    UsamplerCubeArray,
    SamplerCubeArrayShadow,
    Sampler2DMSArray,
    Isampler2DMSArray,
    Usampler2DMSArray,
    Image2DArray,
    Iimage2DArray,
    Uimage2DArray,
    Image2D,
    Iimage2D,
    Uimage2D,
    Image3D,
    Iimage3D,
    Uimage3D,
    ImageCube,
    IimageCube,
    UimageCube,
    Struct,
    Attribute,
    Varying,
    Resource,
    Noperspective,
    Subroutine,
    Common,
    Partition,
    Active,
    Asm,
    Class,
    Union,
    Enum,
    Typedef,
    Template,
    This,
    Goto,
    Inline,
    Noinline,
    Public,
    Static,
    Extern,
    External,
    Interface,
    Long,
    Short,
    Double,
    Half,
    Fixed,
    Unsigned,
    Superp,
    Input,
    Output,
    Hvec2,
    Hvec3,
    Hvec4,
    Dvec2,
    Dvec3,
    Dvec4,
    Fvec2,
    Fvec3,
    Fvec4,
    Sampler3DRect,
    Filter,
    Image1D,
    Iimage1D,
    Uimage1D,
    Iimage1DArray,
    Uimage1DArray,
    Sampler1D,
    Sampler1DShadow,
    Sampler1DArray,
    Sampler1DArrayShadow,
    Isampler1D,
    Isampler1DArray,
    Usampler1D,
    Usampler1DArray,
    Sampler2DRect,
    Sampler2DRectShadow,
    Isampler2DRect,
    Usampler2DRect,
    Sizeof,
    Cast,
    Namespace,
    Using,
    Dmat2,
    Dmat3,
    Dmat4,
    Dmat2x2,
    Dmat2x3,
    Dmat2x4,
    Dmat3x2,
    Dmat3x3,
    Dmat3x4,
    Dmat4x2,
    Dmat4x3,
    Dmat4x4,
    Image2DRect,
    Iimage2DRect,
    Uimage2DRect,
    Image2DMS,
    Iimage2DMS,
    Uimage2DMS,
    Image2DMSArray,
    Iimage2DMSArray,
    Uimage2DMSArray,
    // the below are not real keywords
    None
};

struct KeywordDefinition
{
    Keyword keyword;
    KeywordType type;
    GLenum glesType;
};

// Slow
const std::string& lookup_get_string(Keyword);

// Fast
KeywordDefinition lookup_get(const std::string& key);

// Slow first time, fast after
GLenum lookup_get_gles_type(Keyword key);

// Fast
bool lookup_is_sampler(Keyword key);
