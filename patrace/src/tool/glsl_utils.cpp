#include "glsl_utils.h"

static void replace_all(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());
    std::string::size_type lastPos = 0;
    std::string::size_type findPos;
    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }
    newString += source.substr(lastPos);
    source.swap(newString);
}

std::string convert_to_tf(const std::string& s)
{
    std::string new_vs = s;
    replace_all(new_vs, "gl_Position", "az_outValue");
    replace_all(new_vs, "void main", "out vec4 az_outValue;\nvoid main");
    return new_vs;
}

/// Locations are counted as full vec4 slots. Not sure if we are doing
/// doubles correctly here, but they are hypothetical in GLES anyway.
static int count_locations(Keyword type)
{
    switch (type)
    {
    case Keyword::Float:
    case Keyword::Int:
    case Keyword::Bool:
    case Keyword::Atomic_uint:
    case Keyword::Vec2:
    case Keyword::Vec3:
    case Keyword::Vec4:
    case Keyword::Ivec2:
    case Keyword::Ivec3:
    case Keyword::Ivec4:
    case Keyword::Bvec2:
    case Keyword::Bvec3:
    case Keyword::Bvec4:
    case Keyword::Uint:
    case Keyword::Uvec2:
    case Keyword::Uvec3:
    case Keyword::Uvec4:
    case Keyword::Long:
    case Keyword::Short:
    case Keyword::Double:
    case Keyword::Half:
    case Keyword::Unsigned:
        return 1;
    case Keyword::Mat2:
    case Keyword::Mat2x2:
    case Keyword::Mat2x3:
    case Keyword::Mat2x4:
    case Keyword::Hvec2:
    case Keyword::Dvec2:
    case Keyword::Dmat2:
    case Keyword::Fvec2:
    case Keyword::Dmat2x2:
    case Keyword::Dmat2x3:
    case Keyword::Dmat2x4:
        return 2;
    case Keyword::Mat3:
    case Keyword::Mat3x2:
    case Keyword::Mat3x3:
    case Keyword::Mat3x4:
    case Keyword::Hvec3:
    case Keyword::Dvec3:
    case Keyword::Fvec3:
    case Keyword::Dmat3:
    case Keyword::Dmat3x2:
    case Keyword::Dmat3x3:
    case Keyword::Dmat3x4:
        return 3;
    case Keyword::Mat4:
    case Keyword::Mat4x2:
    case Keyword::Mat4x3:
    case Keyword::Mat4x4:
    case Keyword::Hvec4:
    case Keyword::Dvec4:
    case Keyword::Fvec4:
    case Keyword::Dmat4:
    case Keyword::Dmat4x2:
    case Keyword::Dmat4x3:
    case Keyword::Dmat4x4:
        return 4;
    default: break;
    }
    return 0;
}

static int count_items(Keyword type)
{
    switch (type)
    {
    case Keyword::Float:
    case Keyword::Int:
    case Keyword::Bool:
    case Keyword::Atomic_uint:
    case Keyword::Uint:
    case Keyword::Long:
    case Keyword::Short:
    case Keyword::Double:
    case Keyword::Half:
    case Keyword::Unsigned:
        return 1;
    case Keyword::Vec2:
    case Keyword::Ivec2:
    case Keyword::Bvec2:
    case Keyword::Uvec2:
    case Keyword::Hvec2:
    case Keyword::Dvec2:
    case Keyword::Dmat2:
    case Keyword::Fvec2:
        return 2;
    case Keyword::Vec3:
    case Keyword::Ivec3:
    case Keyword::Bvec3:
    case Keyword::Uvec3:
    case Keyword::Hvec3:
    case Keyword::Dvec3:
    case Keyword::Fvec3:
    case Keyword::Dmat3:
        return 3;
    case Keyword::Dmat2x2:
    case Keyword::Mat2x2:
    case Keyword::Uvec4:
    case Keyword::Bvec4:
    case Keyword::Vec4:
    case Keyword::Ivec4:
    case Keyword::Mat2:
    case Keyword::Hvec4:
    case Keyword::Dvec4:
    case Keyword::Fvec4:
    case Keyword::Dmat4:
        return 4;
    case Keyword::Dmat2x3:
    case Keyword::Mat2x3:
    case Keyword::Mat3x2:
    case Keyword::Dmat3x2:
        return 6;
    case Keyword::Dmat2x4:
    case Keyword::Mat2x4:
    case Keyword::Mat4x2:
    case Keyword::Dmat4x2:
        return 8;
    case Keyword::Mat3:
    case Keyword::Mat3x3:
    case Keyword::Dmat3x3:
    case Keyword::Dmat3x4:
        return 9;
    case Keyword::Mat3x4:
    case Keyword::Mat4x3:
    case Keyword::Dmat4x3:
        return 12;
    case Keyword::Mat4:
    case Keyword::Mat4x4:
    case Keyword::Dmat4x4:
        return 16;
    default: break;
    }
    return 0;
}

// TBD - should also count used built-ins, as they count toward the max limit.
int count_varying_locations_used(GLSLRepresentation r)
{
    int locs = 0;
    for (const GLSLRepresentation::Variable& v : r.global.members)
    {
        if (v.storage != Keyword::In && v.storage != Keyword::Varying) continue;
        locs += count_locations(v.type) * std::max(1, v.size);
        for (const GLSLRepresentation::Variable& vv : v.members)
        {
            // Struct of struct and array of struct not allowed in varyings
            locs += count_locations(vv.type) * std::max(1, v.size);
        }
    }
    return locs;
}

/// Count actually used items
int count_varyings_by_precision(GLSLRepresentation r, Keyword p)
{
    int locs = 0;
    for (const GLSLRepresentation::Variable& v : r.global.members)
    {
        if (v.storage != Keyword::In && v.storage != Keyword::Varying) continue;
        if (v.precision == p) locs += count_items(v.type) * std::max(1, v.size);
        for (const GLSLRepresentation::Variable& vv : v.members)
        {
            // Struct of struct and array of struct not allowed in varyings
            if (v.precision == p) locs += count_items(vv.type) * std::max(1, v.size);
        }
    }
    return locs;
}
