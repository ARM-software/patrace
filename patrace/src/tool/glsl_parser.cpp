#include <algorithm>
#include <cassert>
#include <string>
#include <sstream>
#include <stddef.h>
#include <unordered_map>
#include <climits>
#include <string.h>

// TBD:
// * support UTF-8
// * could be _a lot_ more efficient if I used c++14 string views, but probably don't care about performance here
// * parse any binding values of samplers (and other inputs)

#include "glsl_parser.h"

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_TESS_CONTROL_SHADER 0x8E88
#define GL_TESS_EVALUATION_SHADER 0x8E87
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_COMPUTE_SHADER 0x91B9

#define ASSERT(_stmt, _line, ...) do { if (!(_stmt)) { fprintf(stderr, "input=\"%s\" lineno=%d line=\"%s\"\n", mShaderName.c_str(), mLineNo, _line); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); assert(false); } } while(0)
#define ELOG(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, " (filename=%s, line=%d)\n", mShaderName.c_str(), mLineNo); } while(0)

static inline int is_special(char curr, char next, char next2)
{
    static const char* start_special = ";|&()!=+-*/%<>&^|<>|&!*/%<><>^~:,{}[]??++--.\n";
    static const char* second_special= " |&  =======<>=====      <>          : + -   "; // valid combinations, space means valid alone
    static const char* third_special = "            ==                               ";
    for (unsigned i = 0; start_special[i] != '\0'; i++)
    {
        if (start_special[i] == curr && second_special[i] == next && next != ' ' && third_special[i] == next2 && next2 != ' ') return 3;
        else if (start_special[i] == curr && second_special[i] == next && next != ' ') return 2;
        else if (start_special[i] == curr && second_special[i] == ' ') return 1;
    }
    return 0;
}

static void special_test()
{
    assert(is_special('<','<', '=') == 3);
    assert(is_special('>','>', '=') == 3);
    assert(is_special('/','=', ' ') == 2);
    assert(is_special('!','=', ' ') == 2);
    assert(is_special(';','\0', ' ') == 1);
    assert(is_special(';',' ', ' ') == 1);
    assert(is_special('|','|', ' ') == 2);
    assert(is_special('|',' ', ' ') == 1);
    assert(is_special('!','!', ' ') == 1);
    assert(is_special('/','2', ' ') == 1);
    assert(is_special('(','t', ' ') == 1);
    assert(is_special('>','=', ' ') == 2);
}

struct Token
{
    std::string str;
    enum tokentype
    {
        OPERATOR, IDENTIFIER, KEYWORD, EMPTY
    } type = EMPTY;
    Keyword keyword;
    KeywordType keywordType;
    std::string whitespace;
    Token(const std::string& s, tokentype t, KeywordDefinition k, const std::string& w) : str(s), type(t), keyword(k.keyword), keywordType(k.type), whitespace(w) {}
    Token() {}
};

static Token scan_token(std::string& line, unsigned start = 0)
{
    std::string whitespace;
    Token::tokentype type = Token::IDENTIFIER;
    const size_t length = line.size();
    if (length == 0)
    {
        line = "";
        return { "", Token::EMPTY, { Keyword::None, KeywordType::None }, "" };
    }
    while (start < length && line[start] != '\0' && (line[start] == '\t' || line[start] == ' '))
    {
        whitespace += line[start]; // store whitespace
        start++; // skip whitespace
    }
    size_t end = start;
    while (end < length && line[end] != '\0' && line[end] != '\t' && line[end] != ' ')
    {
        const int special = is_special(line[end], (length - end > 0) ? line[end + 1] : ' ', (length - end > 1) ? line[end + 2] : ' ');
        if (special && end == start) {
            end += special;    // got something
            type = Token::OPERATOR;
            break;
        }
        else if (special && end != start) {
            break;    // hit something
        }
        end++;
    }
    std::string ret = line.substr(start, end - start); // non-inclusive for end
    KeywordDefinition k = lookup_get(ret);
    if (k.keyword != Keyword::None)
    {
        type = Token::KEYWORD;
    }
    line = line.substr(end);
    return { ret, type, k, whitespace };
}

static std::string scan_rest(std::string& line, unsigned start = 0)
{
    if (start >= line.size()) return std::string();
    unsigned end = start + 1;
    while (start == end || line[end] != '\0') end++; // skip non-whitespace
    std::string ret = line.substr(start, end - start);
    if (end > line.size()) line = std::string();
    else line = line.substr(end);
    return ret;
}

static std::string macro_expansion(const std::string& orig, const std::unordered_map<std::string, Define>& defines);

static std::string replace_macro(std::string& s, const Define& define, const std::unordered_map<std::string, Define>& defines)
{
    if (define.params.size() == 0) // easy! (simple macro)
    {
        return define.value;
    }
    else if (s.size() == 0 || s[0] == '\n' || s[0] == '\0')
    {
        return s;
    }
    else // harder (macro function)
    {
        std::vector<std::string> replacements;
        // cannot use scan_token() below, since input does not follow token rules
        const char* start = strchr(s.c_str(), '(');
        assert(start);
        unsigned i = 0;
        const char* next = start;
        do
        {
            int recursive = 0;
            next++;
            while (*next == ' ' || *next == '\t') next++; // skip spaces
            const char* probe = next;
            const char* lim = nullptr;
repeat:
            lim = probe = strpbrk(probe, "(,)");
            if (*probe == '(') { recursive++; probe++; goto repeat; }
            else if (*probe == ')' && recursive > 0) { recursive--; probe++; goto repeat; }
            else if (*probe == ',' && recursive > 0) { probe++; goto repeat; }
            else if (*probe == ')' || *probe == ',') lim--;
            while (*lim == ' ' || *lim == '\t') lim--;
            const std::string s = std::string(next, (lim + 1) - next);
            replacements.push_back(macro_expansion(s, defines));
            next = probe;
            i++;
        }
        while (next && *next != ')');
        s = std::string(++next); // consume parameters in input, let callee keep remains
        assert(i == define.params.size()); // we found same number of parameters
        // Ok, now replace
        std::string ret;
        std::string repl = define.value;
        Token curr = scan_token(repl);
        ret += curr.whitespace;
        while (curr.str.size())
        {
            bool found = false;
            int j = 0;
            for (const std::string& r : define.params)
            {
                if (curr.str == r)
                {
                    ret += macro_expansion(replacements.at(j), defines);// + " ";
                    found = true;
                    break;
                }
                j++;
            }
            if (!found)
            {
                ret += macro_expansion(curr.str, defines);// + " ";
            }
            curr = scan_token(repl);
            ret += curr.whitespace;
        }
        return ret;
    }
}

/// As a side-effect, this strips most whitespace. Takes a line or line fragment as input.
static std::string macro_expansion(const std::string& orig, const std::unordered_map<std::string, Define>& defines)
{
    std::string r;
    std::string s = orig;
    Token prev;
    while (s.size())
    {
        Token curr = scan_token(s);
        if (defines.count(curr.str) > 0)
        {
            const Define& d = defines.at(curr.str);
            curr.str = replace_macro(s, d, defines);
        }
        r += curr.whitespace + curr.str;
        prev = curr;
    }
    return r;
}

// All non-unary, non-parenthetical operators, all of which are left
// to right associativity. This leaves out (), unary +-~! and defined.
struct opdef
{
    std::string token;
    int precedence; // lower is higher precedence
    long (*eval)(long t1, long t2);
};
static opdef operators[] =
{
    { "*", 3, [](long t1, long t2)->long{ return t1 * t2; } },
    { "/", 3, [](long t1, long t2)->long{ return t1 / t2; } },
    { "%", 3, [](long t1, long t2)->long{ return t1 % t2; } },
    { "+", 4, [](long t1, long t2)->long{ return t1 + t2; } },
    { "-", 4, [](long t1, long t2)->long{ return t1 - t2; } },
    { "<<", 5, [](long t1, long t2)->long{ return t1 << t2; } },
    { ">>", 5, [](long t1, long t2)->long{ return t1 >> t2; } },
    { "<", 6, [](long t1, long t2)->long{ return t1 < t2; } },
    { ">", 6, [](long t1, long t2)->long{ return t1 > t2; } },
    { "<=", 6, [](long t1, long t2)->long{ return t1 <= t2; } },
    { ">=", 6, [](long t1, long t2)->long{ return t1 >= t2; } },
    { "==", 7, [](long t1, long t2)->long{ return t1 == t2; } },
    { "!=", 7, [](long t1, long t2)->long{ return t1 != t2; } },
    { "&", 8, [](long t1, long t2)->long{ return t1 & t2; } },
    { "^", 9, [](long t1, long t2)->long{ return t1 ^ t2; } },
    { "|", 10, [](long t1, long t2)->long{ return t1 | t2; } },
    { "&&", 11, [](long t1, long t2)->long{ return t1 && t2; } },
    { "||", 12, [](long t1, long t2)->long{ return t1 || t2; } },
};

static const opdef* get_operator(const std::string& token)
{
    for (const auto& op : operators)
    {
        if (token == op.token)
        {
            return &op;
        }
    }
    return nullptr;
}

static int match_operator(const std::string& token)
{
    if (token == "(")
    {
        return 1;
    }
    else if (token == "defined" || token == "~" || token == "!" || token == "_")
    {
        return 2; // unary operator
    }
    for (const auto& op : operators)
    {
        if (token == op.token)
        {
            return op.precedence;
        }
    }
    return 0;
}

bool GLSLParser::resolve_conditionals(const std::unordered_map<std::string, Define>& defines, const std::string& expr, int shader_type)
{
    ///
    // Implement shunting-yard algorithm for turning into reverse polish notation,
    // while resolving define replacements and special keywords.
    // See https://en.wikipedia.org/wiki/Shunting-yard_algorithm
    ///
    std::vector<std::string> ops;
    std::vector<std::string> vals;
    std::string tmpstr = expr;
    bool was_defined = false; // was previous directive a 'defined'? if so, do not macro expand!
    int was_unary = 0;
    bool may_be_unary = true;
    do {
        Token token = scan_token(tmpstr);

        char *end = nullptr;
        long value = strtol(token.str.c_str(), &end, 10);
        int op = match_operator(token.str);

        if (end != nullptr && *end == '\0' && token.str.size() > 0) // valid integer value
        {
            vals.push_back(std::to_string(value));
            may_be_unary = false;
        }
        else if (token.str.empty())
        {
            break;
        }
        else if (token.str == "__VERSION__")
        {
            vals.push_back("320");
        }
        else if (token.str == "__LINE__")
        {
            vals.push_back(std::to_string(mLineNo));
        }
        else if (token.str == "__FILE__")
        {
            vals.push_back("1"); // hah
        }
        else if (may_be_unary && (token.str == "-" || token.str == "+"))
        {
            if (token.str == "-")
            {
                ops.push_back("_");
                was_unary++;
            }
            else
            {
                // ignored!
            }
        }
        else if (token.str == "defined" || token.str == "!" || token.str == "~")
        {
            may_be_unary = true;
            ops.push_back(token.str);
            was_defined = (token.str == "defined");
            was_unary++;
        }
        else if (op > 0)
        {
            may_be_unary = true; // any operator except ')'
            int top_op = 0; // what is the top (previous) operator
            if (ops.size() > 0) top_op = match_operator(ops.back()); // if any...
            // Push out operator if we can... and as many as we can. Cannot push
            // bracket end or any unary operator.
            while (top_op <= op && top_op > 0 && top_op != 1)
            {
                vals.push_back(ops.back());
                ops.pop_back();
                top_op = 0;
                if (ops.size() > 0) top_op = match_operator(ops.back());
            }
            ops.push_back(token.str);
        }
        else if (token.str == ")")
        {
            while (ops.size() > 0 && ops.back() != "(")
            {
                vals.push_back(ops.back());
                ops.pop_back();
            }
            ASSERT(ops.size() > 0, expr.c_str(), "No left bracket in operator stack!");
            ops.pop_back(); // pop left bracket, ie '('
        }
        else
        {
            // macro expansion
            may_be_unary = false;
            if (!was_defined && defines.count(token.str) > 0)
            {
                // can only pray they don't nest resolving somehow
                const Define& d = defines.at(token.str);
                if (d.params.size() > 0) // macro function
                {
                    std::string t = token.str;
                    token = scan_token(tmpstr, 0);
                    ASSERT(token.str == "(", expr.c_str(), "Function without parenthesis begin");
                    t += token.str;
                    while (token.str.size() && token.str != ")")
                    {
                        token = scan_token(tmpstr, 0);
                        t += token.str;
                    }
                    ASSERT(token.str == ")", expr.c_str(), "Function without parenthesis end");
                    token.str = macro_expansion(t, defines);
                }
                else
                {
                    token.str = d.value;
                }
            }
            vals.push_back(token.str);
            was_defined = false;
            while (was_unary && ops.size() > 0 && ops.back() != "(")
            {
                ASSERT(ops.back() != ")" && ops.back() != "(", expr.c_str(), "Tried to push parenthesis onto value stack!");
                vals.push_back(ops.back());
                ops.pop_back();
                was_unary--;
            }
        }
    } while (tmpstr.size());
    while (ops.size() > 0)
    {
        vals.push_back(ops.back());
        ASSERT(ops.back() != ")" && ops.back() != "(", expr.c_str(), "Mismatched parentheses");
        ops.pop_back();
    }

    ///
    // Now calculate the result.
    // See https://en.wikipedia.org/wiki/Reverse_Polish_notation
    ///
    std::vector<std::string> stack;
    for (const std::string& s : vals)
    {
        const opdef* op = get_operator(s);
        if (op)
        {
            ASSERT(stack.size() > 1, expr.c_str(), "Operator with insufficient operands");
            const long p2 = strtol(stack.back().c_str(), nullptr, 10);
            stack.pop_back(); // order reversed in stack
            const long p1 = strtol(stack.back().c_str(), nullptr, 10);
            stack.pop_back();
            const long result = op->eval(p1, p2);
            stack.push_back(std::to_string(result));
        }
        else if (s == "defined")
        {
            ASSERT(stack.size() > 0, expr.c_str(), "Defined operator with no operand");
            const std::string str = stack.back();
            stack.pop_back();
            stack.push_back(std::to_string(int(defines.count(str) > 0)));
        }
        else if (s == "!")
        {
            ASSERT(stack.size() > 0, expr.c_str(), "Negation operator with no operand");
            const long p1 = strtol(stack.back().c_str(), nullptr, 10);
            stack.pop_back();
            stack.push_back(std::to_string(!p1));
        }
        else if (s == "~")
        {
            ASSERT(stack.size() > 0, expr.c_str(), "Bitwise negation operator with no operand");
            const long p1 = strtol(stack.back().c_str(), nullptr, 10);
            stack.pop_back();
            stack.push_back(std::to_string(~p1));
        }
        else if (s == "_") // unary minus
        {
            ASSERT(stack.size() > 0, expr.c_str(), "Unary minus operator with no operand");
            const long p1 = strtol(stack.back().c_str(), nullptr, 10);
            stack.pop_back();
            stack.push_back(std::to_string(-p1));
        }
        else
        {
            stack.push_back(s); // must be an operand
        }
    }
    if (stack.size() != 1) // we are going to crash, print out what we got
    {
        fprintf(stderr, "Results:");
        for (const std::string& s : stack)
        {
            fprintf(stderr, "\t%s\n", s.c_str());
        }
        fprintf(stderr, "Values:");
        for (const std::string& s : vals)
        {
            fprintf(stderr, "\t%s\n", s.c_str());
        }
    }
    ASSERT(stack.size() == 1, expr.c_str(), "No singular result left after calculating the result! Got %u results", (unsigned)stack.size());
    return strtol(stack.back().c_str(), nullptr, 10);
}

// Implements GLSL spec 3.10 step 9. Steps 4, 6 and 7 must have
// been done previously. Steps 5 and 8 are to be skipped.
GLSLShader GLSLParser::preprocessor(std::string shader, int shaderType)
{
    GLSLShader ret;
    ret.shaderType = shaderType;
    std::unordered_map<std::string, Define> defines;
    defines["GL_ES"] = { "1" };
    defines["GL_FRAGMENT_PRECISION_HIGH"] = { "1" };
    defines["GL_ARM_shader_framebuffer_fetch"] = { "1" };
    defines["GL_ARM_shader_framebuffer_fetch_depth_stencil"] = { "1" };
    defines["GL_EXT_shader_pixel_local_storage"] = { "1" };
    defines["GL_EXT_gpu_shader5"] = { "1" };
    defines["GL_EXT_primitive_bounding_box"] = { "1" };
    defines["GL_OES_shader_io_blocks"] = { "1" };
    defines["GL_EXT_shader_non_constant_global_initializers"] = { "1" };
    std::istringstream lines(shader);
    std::string orig_line;
    std::vector<int> nested; // 1 - true, 0 - false, -1 - false but has been true
    mLineNo = 1;
    while (std::getline(lines, orig_line))
    {
        int i = 0;
        std::string line = orig_line;
        while (line[i] != '\0' && (line[i] == '\t' || line[i] == ' ')) i++; // skip whitespace
        if (line[i] == '#') // found a preprocessor directive
        {
            i++; // skip the #
            std::string directive = scan_token(line, i).str;
            if (directive == "endif")
            {
                ASSERT(nested.size(), orig_line.c_str(), "Mismatched if...endif");
                nested.pop_back();
                if (mDebug) ret.code += "//## removed endif nesting=" + std::to_string(nested.size());
            }
            else if (directive == "else")
            {
                ASSERT(nested.size(), orig_line.c_str(), "Mismatched if...else");
                nested.back() = (nested.back() == 0);
                if (mDebug) ret.code += std::string("//## removed else next=") + std::to_string(nested.back());
            }
            else if (directive == "ifdef" || directive == "ifndef")
            {
                std::string macro = scan_token(line).str;
                bool success = (defines.count(macro) > 0 || macro == "GL_ES");
                if (directive == "ifndef")
                {
                    success = !success;
                }
                nested.push_back((int)success);
                if (mDebug) ret.code += "//## removed if(n)def macro=" + macro + std::string(" success=") + (success ? "true" : "false");
            }
            else if (directive == "if" || directive == "elif")
            {
                if (directive == "elif" && nested.size() > 0 && nested.back() != 0) // has been true, no need to evaluate
                {
                    nested.back() = -1;
                    if (mDebug) ret.code += std::string("//## removed elif success=") + std::to_string(nested.back()) + " [no need to eval]";
                }
                else
                {
                    std::string conditionals = scan_rest(line);
                    bool success = resolve_conditionals(defines, conditionals, shaderType);
                    if (directive != "elif")
                    {
                        nested.push_back((int)success);
                    }
                    else if (success)
                    {
                        nested.back() = (int)success;
                    }
                    if (mDebug) ret.code += std::string("//## removed (el)if success=") + std::to_string(nested.back()) + " cond=" + conditionals;
                }
            }
            else if (nested.size() > 0
                     && (std::find(std::begin(nested), std::end(nested), 0) != std::end(nested)
                         || std::find(std::begin(nested), std::end(nested), -1) != std::end(nested))) // inside false section of #if ... #endif
            {
                // skip!
                if (mDebug) ret.code += "//## skipped preprocessor directive";
            }
            else if (directive == "define")
            {
                std::string key = scan_token(line).str;
                std::string value;
                // it is only a function-like macro if the parenthesis follows immediately after WITHOUT any whitespace
                Define d = { key };
                if (line[0] == '(') // is a function-like macro
                {
                    value = scan_token(line).str;
                    while (value != ")")
                    {
                        value = scan_token(line).str;
                        if (value != ")" && value != ",")
                        {
                            d.params.push_back(value);
                        }
                    }
                }
                value = scan_rest(line);
                d.value = macro_expansion(value, defines);
                ASSERT(key.size() > 0, orig_line.c_str(), "Bad define key!");
                defines[key] = d;
                if (mDebug) ret.code += "//## gobbled up define key=" + key + " value=\"" + d.value + "\"";
            }
            else if (directive == "undef")
            {
                std::string key = scan_token(line).str;
                defines.erase(key);
                if (mDebug) ret.code += "//## gobbled up undef key=" + key;
            }
            else if (directive == "extension")
            {
                std::string ext = scan_token(line).str;
                std::string colon = scan_token(line).str;
                std::string cond = scan_token(line).str;
                assert(colon == ":");
                assert(ext.size() > 0);
                assert(cond.size() > 0);
                if (colon == ":" && ext.size() > 0 && cond.size() > 0 && (cond == "require" || cond == "enable" || cond == "warn"))
                {
                    ret.extensions.push_back(ext);
                }
                else if (cond != "disable")
                {
                    ELOG("Bad extension directive: %s", orig_line.c_str());
                    assert(false);
                }
                if (mDebug) ret.code += "//## gobbled up extension ext=" + ext;
            }
            else if (directive == "version")
            {
                std::string version = scan_token(line).str;
                assert(version.size() > 0);
                ret.version = strtol(version.c_str(), nullptr, 10);
                assert(ret.version != INT_MAX && ret.version != INT_MIN); // over-/underflow
                assert(ret.version != 0); // generic error
            }
            else if (directive == "error")
            {
                std::string err_msg = scan_rest(line);
                ELOG("ERROR directive: %s", err_msg.c_str());
                assert(false);
                if (mDebug) ret.code += "//## triggered error";
            }
            else if (directive == "line")
            {
                std::string newlineno = scan_token(line).str;
                long lineno2 = strtol(newlineno.c_str(), nullptr, 10);
                assert(lineno2 != INT_MAX && lineno2 != INT_MIN); // over-/underflow
                if (mDebug) ret.code += "//## gobbled up (ignored) line " + newlineno;
            }
            else if (directive == "pragma")
            {
                std::string token = scan_token(line).str;
                if (token.compare(0, 5, "debug") == 0)
                {
                    std::string first = scan_token(line).str;
                    ASSERT(first == "(", orig_line.c_str(), "Pragma missing start parenthesis");
                    std::string value = scan_token(line).str;
                    if (value == "on")
                    {
                        ret.contains_debug_on_pragma = true;
                    }
                    std::string last = scan_token(line).str;
                    ASSERT(last == ")", orig_line.c_str(), "Pragma missing close parenthesis");
                }
                else if (token.compare(0, 8, "optimize") == 0)
                {
                    std::string first = scan_token(line).str;
                    ASSERT(first == "(", orig_line.c_str(), "Pragma missing start parenthesis");
                    std::string value = scan_token(line).str;
                    if (value == "off")
                    {
                        ret.contains_optimize_off_pragma = true;
                    }
                    std::string last = scan_token(line).str;
                    ASSERT(last == ")", orig_line.c_str(), "Pragma missing close parenthesis");
                }
                else if (token.compare(0, 5, "STDGL") == 0)
                {
                    // #pragma STDGL invariant(all)
                    std::string type = scan_token(line).str;
                    if (type == "invariant")
                    {
                        std::string first = scan_token(line).str;
                        ASSERT(first == "(", orig_line.c_str(), "Pragma missing start parenthesis");
                        std::string value = scan_token(line).str;
                        if (value == "all")
                        {
                            ret.contains_invariant_all_pragma = true;
                        }
                        std::string last = scan_token(line).str;
                        ASSERT(last == ")", orig_line.c_str(), "Pragma missing close parenthesis");
                    }
                }
                if (mDebug) ret.code += "//## gobbled up pragma " + token;
            }
            else if (directive.size() == 0)
            {
                // nothing
                if (mDebug) ret.code += "//## removed empty # line";
            }
            else
            {
                ELOG("UNKNOWN directive: \"%s\"", directive.c_str());
                assert(false);
                if (mDebug) ret.code += "//## removed unknown directive " + directive;
            }
            ret.code += "\n";
        }
        else if (line.size() > 0 && line[0] != '\n' && line[0] != '\0')
        {
            if (nested.size() > 0
                    && (std::find(std::begin(nested), std::end(nested), 0) != std::end(nested)
                        || std::find(std::begin(nested), std::end(nested), -1) != std::end(nested))) // inside #if ... #endif
            {
                // ignore
                if (mDebug) ret.code += "//## skipped code nesting=" + std::to_string(nested.size());
            }
            else
            {
                ret.code += macro_expansion(orig_line, defines);
            }
            ret.code += "\n";
        }
        mLineNo++;
    }
    return ret;
}

std::string GLSLParser::inline_includes(const std::string& s)
{
    std::string r;
    std::istringstream lines(s);
    std::string orig_line;
    mLineNo = 1;
    while (std::getline(lines, orig_line))
    {
        int i = 0;
        std::string line = orig_line;
        while (line[i] != '\0' && (line[i] == '\t' || line[i] == ' ')) i++; // skip whitespace
        if (line[i] == '#') // found a preprocessor directive
        {
            i++; // skip the #
            std::string directive = scan_token(line, i).str;
            if (directive == "include")
            {
                while (line[i] != '\0' && (line[i] == '\t' || line[i] == ' ' || line[i] == '"' || line[i] == '<')) i++; // skip noise
                std::string filename = scan_rest(line, i); // filename
                filename = filename.substr(0, filename.size() - 1);
                FILE* fp = fopen(filename.c_str(), "r");
                if (!fp)
                {
                    fprintf(stderr, "Include file not found \"%s\" on line %d\n", filename.c_str(), mLineNo);
                    return "";
                }
                std::string data;
                fseek(fp, 0, SEEK_END);
                data.resize(ftell(fp));
                fseek(fp, 0, SEEK_SET);
                size_t result = fread(&data[0], data.size(), 1, fp);
                if (result != 1)
                {
                    fprintf(stderr, "Failed to read \"%s\": %s\n", filename.c_str(), strerror(ferror(fp)));
                    return "";
                }
                fclose(fp);
                r += inline_includes(data); // recursive call
                r += '\n';
            }
            else
            {
                r += orig_line;
            }
        }
        else
        {
            r += orig_line;
        }
        mLineNo++;
    }
    return r;
}

// Implements GLSL spec 3.10 steps 4, 6 and 7. Step 5 can be ignored.
// GLSL does not allow nested comments, and has no trigraphs
std::string GLSLParser::strip_comments(const std::string& s)
{
    std::string r;
    char prev = '\0';
    bool inside = false;
    bool prev_star = false;
    bool prev_backslash = false;
    bool eolcomment = false;
    for (size_t i = 0; i < s.size() + 1; i++)
    {
        char curr = s[i];

        if (curr == '\0' && !prev_star && prev != '\0')
        {
            r += prev;
        }
        else if (curr == '\r')
        {
            continue; // ignore it
        }
        else if (curr == '\\')
        {
            prev_backslash = true;
            continue; // ignore it
        }
        else if (curr == '\n' && prev_backslash)
        {
            prev_backslash = false;
            continue; // ignore it
        }
        else if (curr == '\n' && eolcomment)
        {
            eolcomment = false;
            r += ' '; // as per the standard, comment replaced by space
            r += '\n';
        }
        else if (!eolcomment && prev == '/' && curr == '/' && !inside)
        {
            eolcomment = true;
        }
        else if (!inside && prev == '/' && curr == '*')
        {
            inside = true;
        }
        else if (inside && prev == '*' && curr == '/')
        {
            r += ' '; // as per the standard, comment replaced by space
            inside = false;
        }
        else if (prev == '/' && !eolcomment && !inside && !prev_star)
        {
            r += prev;
            r += curr;
            curr = '\0';
        }
        else if (curr != '/' && !eolcomment && !inside && curr != '\0')
        {
            r += curr;
            curr = '\0';
        }

        prev_backslash = false;
        prev_star = (prev == '*');
        prev = curr;
    }
    return r;
}

static void test_scan(const std::string& k, Token::tokentype ty, const std::string& r)
{
    std::string kc = k;
    Token t = scan_token(kc);
    assert(t.type == ty);
    assert(t.str == r);
}

void GLSLParser::self_test()
{
    special_test();

    {
        const std::unordered_map<std::string, Define> om{{"mix2", {" mix((a),(b),(t))", {"a", "b", "t"}}}};
        std::string s = "mix2(c1.rgb,(c*upper) + ((1.0-c)*lower),opacity)";
        std::string r = replace_macro(s, om.at("mix2"), om);
        assert(r == " mix((c1.rgb),((c*upper) + ((1.0-c)*lower)),(opacity))");
        std::string s2 = "mix2(texcolor.rgb,overlayBlend3(texcolor.rgb, texoverlay.rgb*vec3(0.5,0.5,0.5), overlayweights.r),overlaymask.r)";
        r = replace_macro(s2, om.at("mix2"), om);
        assert(r == " mix((texcolor.rgb),(overlayBlend3(texcolor.rgb, texoverlay.rgb*vec3(0.5,0.5,0.5), overlayweights.r)),(overlaymask.r))");
    }

    const std::unordered_map<std::string, Define> om1{{"float2", {"vec2"}}};
    const char* v1 = " Square(float2 A)";
    std::string i1 = v1;
    Define d1{"vec2"};
    std::string r1 = replace_macro(i1, d1, om1);
    assert(i1 == v1);
    assert(r1 == "vec2");

    v1 = "float2 Square(float2 A)";
    std::string r2 = macro_expansion(v1, om1);
    assert(r2 == "vec2 Square(vec2 A)");

    std::string trick = "/= not a comment =/";
    std::string trick2 = strip_comments(trick);
    assert(trick == trick2);
    std::string blah = "blklj\r\nsldkfj\r\n";
    std::string blah2 = strip_comments(blah);
    std::string closing = "}";
    std::string closing2 = strip_comments(closing);
    assert(closing == closing2);
    assert(blah2.find('\r') == std::string::npos);
    assert(blah2.find("blklj") == 0);
    assert(blah2.find("nsldkfj"));
    test_scan("/=", Token::OPERATOR, "/=");
    test_scan("", Token::EMPTY, "");
    test_scan("TOKEN!TOKEN", Token::IDENTIFIER, "TOKEN");
    test_scan("#", Token::IDENTIFIER, "#"); // well...
    test_scan("e", Token::IDENTIFIER, "e");
    //test_scan("###", token::OPERATOR, "#"); // illegal?
    test_scan("<<=", Token::OPERATOR, "<<=");
    test_scan("<<=!1", Token::OPERATOR, "<<=");
    std::string s = "TOKEN TOKEN";
    Token t = scan_token(s);
    assert(t.str == "TOKEN" && t.type == Token::IDENTIFIER);
    t = scan_token(s);
    assert(t.str == "TOKEN" && t.type == Token::IDENTIFIER);
    s = "!TOKEN&&TOKEN";
    t = scan_token(s);
    assert(t.str == "!" && t.type == Token::OPERATOR);
    t = scan_token(s);
    assert(t.str == "TOKEN" && t.type == Token::IDENTIFIER);
    t = scan_token(s);
    assert(t.str == "&&" && t.type == Token::OPERATOR);
    t = scan_token(s);
    assert(t.str == "TOKEN" && t.type == Token::IDENTIFIER);
    s = "\tresult /= 255.0;";
    t = scan_token(s);
    assert(t.str == "result" && t.type == Token::IDENTIFIER);
    t = scan_token(s);
    assert(t.str == "/=" && t.type == Token::OPERATOR);
    t = scan_token(s);
    assert(t.str == "255" && t.type == Token::IDENTIFIER);
    t = scan_token(s);
    assert(t.str == "." && t.type == Token::OPERATOR);
    t = scan_token(s);
    assert(t.str == "0" && t.type == Token::IDENTIFIER);

    const std::unordered_map<std::string, Define> oop{{"GL_ES", {"1"}},{"GL_FRAGMENT_PRECISION_HIGH", {"1"}},{"TRUEDEF",{"1"}},{"FALSEDEF",{"0"}}, {"LIGHTING", {"1"}}, {"MAX_BONE_WEIGHTS", {"2"}}};
    assert(resolve_conditionals(oop, "(COLOR_MULTIPLY_SOURCE >= COLOR_MULTIPLY_SOURCE_MASK_TEXTURE_RED) && (COLOR_MULTIPLY_SOURCE <= COLOR_MULTIPLY_SOURCE_MASK_TEXTURE_ALPHA)", GL_FRAGMENT_SHADER) == true);
    assert(resolve_conditionals(oop, "defined LIGHTING || defined REFLECTION || defined DEP_TEXTURING || defined TRANSITION_EFFECT", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "defined REFLECTION || defined DEP_TEXTURING || defined TRANSITION_EFFECT", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "1 || defined DEP_TEXTURING || defined TRANSITION_EFFECT", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "defined DEP_TEXTURING || defined TRANSITION_EFFECT || 1 == 1", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "(3 - 3)", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "!1", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "!0", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "!(3 - 3)", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "!!(3 - 3)", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "-1 == 1", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "-1 == -1", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "+(-1) == -1", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "-(-1) == 1", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "defined(LOD2) || (MAX_BONE_WEIGHTS < 1)", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "-(1-1)-(1-1) == 0", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "MAX_BONE_WEIGHTS > 0", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "MAX_BONE_WEIGHTS > 1", GL_VERTEX_SHADER) == true);
    assert(resolve_conditionals(oop, "MAX_BONE_WEIGHTS > 2", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "MAX_BONE_WEIGHTS > 3", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "defined(EVA_UV_ANIMATION)", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "defined(EVA_UV_ANIMATION)  ", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "EVA_SHADER_PROFILE < 50 && defined(EVA_ALPHA_TEST) && defined(EVA_MULTISAMPLE_ENABLED)  ", GL_VERTEX_SHADER) == false);
    assert(resolve_conditionals(oop, "defined DEP_TEXTURING || defined TRANSITION_EFFECT || -1 == 1", GL_VERTEX_SHADER) == false);
    std::unordered_map<std::string, Define> oop2{{"GL_ES", {"1"}},{"GL_FRAGMENT_PRECISION_HIGH", {"1"}}};
    oop2["UNIFORM"] = { "uniform Type Name", { "Type", "Name" } };
    assert(macro_expansion("UNIFORM(sampler2D,Name)", oop2) == "uniform sampler2D Name");

    bool was_debug = mDebug;
    mDebug = false; // otherwise below test will fail

    std::string ppin = "#define float2 vec2\nfloat2 Square(float2 A) { return A * A; }";
    GLSLShader shader = preprocessor(ppin, GL_FRAGMENT_SHADER);
    assert(shader.code.find("Square") != std::string::npos);
    assert(shader.code.find("vec2") != std::string::npos);
    assert(shader.code.find("float2") == std::string::npos);

    std::string ppin2 = "#define PIXEL_MAIN void main()\nPIXEL_MAIN\n{\n}";
    shader = preprocessor(ppin2, GL_FRAGMENT_SHADER);
    assert(shader.code.find("main") != std::string::npos);
    assert(shader.code.find("PIXEL_MAIN") == std::string::npos);

    std::string ppin3 = "#if NOPE || NOPE2\nNOT_THIS\n#elif NOPE3\nNOT_THIS2\n#else\nTHIS_WE_WANT\n#endif";
    shader = preprocessor(ppin3, GL_FRAGMENT_SHADER);
    assert(shader.code.find("THIS_WE_WANT") != std::string::npos);

    std::string ppin4 = "#define REC 0\n#define OBSCURE(p1, p2, p3) texture(p1, p2, p3)\nOBSCURE(REC, REC, REC)\n";
    shader = preprocessor(ppin4, GL_FRAGMENT_SHADER);
    assert(shader.code.find("REC") == std::string::npos);
    assert(shader.code.find("OBSCURE") == std::string::npos);
    assert(shader.code.find("texture") != std::string::npos);

    std::string ppin5 = "#define DEFAULT_LOD_BIAS -0.50\n#define texture2DBias(Texture,UV,Bias) texture2D(Texture, UV, Bias)\nlowp vec4 BaseColor = texture2DBias( TextureBase, BaseTextureCoord, DEFAULT_LOD_BIAS );\n";
    shader = preprocessor(ppin5, GL_FRAGMENT_SHADER);
    assert(shader.code.find("DEFAULT_LOD_BIAS") == std::string::npos);

    std::string ppin6 = "#if 0\nNOPE\n#elif 1\nDUMBO\n#endif\n";
    shader = preprocessor(ppin6, GL_FRAGMENT_SHADER);
    assert(shader.code.find("DUMBO") != std::string::npos);
    assert(shader.code.find("NOPE") == std::string::npos);

    std::string ppin7 = "#define mulmat(p1,p2) (p1*p2)\nmulmat(tex,vec3(blah, 1.0))\n";
    shader = preprocessor(ppin7, GL_FRAGMENT_SHADER);

    std::string ppin8 = "#define A(x) x*x\n#define B(x) A(A(x))\nB(blah)";
    shader = preprocessor(ppin8, GL_FRAGMENT_SHADER);

    std::string ppin9 = "#define CONV(v) v.blah\nCONV(result)\n";
    shader = preprocessor(ppin9, GL_FRAGMENT_SHADER);
    assert(shader.code.find("result") != std::string::npos);

    std::string ppin10 = "struct InstanceData{mat4 model;};layout(std140, binding = 0) uniform UBlockInstance{InstanceData instance_data[4];};";
    shader = preprocessor(ppin10, GL_FRAGMENT_SHADER);
    (void)parse(shader); // just checking the asserts

    mDebug = was_debug;
}

std::string GLSLParser::compressed(GLSLShader shader)
{
    std::string r;
    std::string feed = shader.code;
    Token prev;
    int line = 0;
    while (feed.size())
    {
        Token curr = scan_token(feed);
        if ((curr.type == Token::IDENTIFIER || curr.type == Token::KEYWORD)
                && curr.str != "=" && prev.str != "=" && curr.str != "."
                && (prev.type == Token::IDENTIFIER || prev.type == Token::KEYWORD)) r += " "; // mandatory space
        if (curr.str == "\n")
        {
            if (line++ > 0)
            {
                continue;
            }
        }
        prev = curr;
        r += curr.str;
    }
    return r;
}

GLSLRepresentation GLSLParser::parse(const GLSLShader& shader)
{
    GLSLRepresentation ret;
    mLineNo = 1;
    std::string feed = shader.code;
    int block_depth = 0;
    /// Keep track of structure type definitions. Structures without a type name are immediately parsed
    std::unordered_map<std::string, GLSLRepresentation::Variable> structs; // type name : struct definitions

    // Setting default layouts on global scope, see 4.4.4 in the GLSL spec
    std::unordered_map<Keyword, std::string, EnumClassHash> default_packing_layout; // at global scope
    default_packing_layout[Keyword::Uniform] = "shared";
    default_packing_layout[Keyword::Buffer] = "shared";
    std::unordered_map<Keyword, Keyword, EnumClassHash> default_major_layout; // at global scope
    default_packing_layout[Keyword::Uniform] = "column_major";
    default_packing_layout[Keyword::Buffer] = "column_major";

    // Setting default precisions on global scope, see 4.7.4 in the GLSL spec
    std::unordered_map<Keyword, Keyword, EnumClassHash> default_precision; // at global scope
    default_precision[Keyword::Sampler2D] = Keyword::Lowp;
    default_precision[Keyword::SamplerCube] = Keyword::Lowp;
    default_precision[Keyword::Atomic_uint] = Keyword::Highp;
    if (shader.shaderType == GL_FRAGMENT_SHADER)
    {
        default_precision[Keyword::Int] = Keyword::Mediump;
    }
    else
    {
        default_precision[Keyword::Float] = Keyword::Highp;
        default_precision[Keyword::Int] = Keyword::Highp;
    }

    Keyword state = Keyword::None;
    std::vector<GLSLRepresentation::Variable> nesting; // we're only ever one level deep, but this is the correct design
    GLSLRepresentation::Variable var;
    bool in_array = false;
    bool in_struct = false;
    bool in_interface = false;
    enum class Termination { None, StructScope, InterfaceScope, StructMember };
    Termination do_terminate = Termination::None;
    std::string struct_name; // definitions never nested
    while (feed.size())
    {
        Token curr = scan_token(feed);
        if (curr.str == "\n")
        {
            mLineNo++;
            continue;
        }
        ASSERT(curr.str.find("\n") == std::string::npos, curr.str.c_str(), "EOL mixed into token");
        ASSERT(nesting.size() <= 2, curr.str.c_str(), "Too many nested levels");

        // Outside of global scope, we only interested in counting { and }
        if (curr.str == "{")
        {
            if (in_struct || state == Keyword::Uniform) // structure or layout interface
            {
                in_interface = !in_struct;
                nesting.push_back(var);
                var = GLSLRepresentation::Variable();
            }
            else
            {
                block_depth++;
            }
        }
        else if (curr.str == ";" && do_terminate != Termination::None)
        {
            // Anything setting do_terminate will also have pushed something on the nesting stack
            nesting.back().dimensions = var.dimensions;
            ret.global.members.push_back(nesting.back());
            struct_name = std::string();
            in_struct = (do_terminate == Termination::StructScope) ? false : in_struct;
            in_interface = (do_terminate == Termination::InterfaceScope) ? false : in_interface;
            state = Keyword::None;
            var = GLSLRepresentation::Variable();
            ASSERT(nesting.size() > 0, curr.str.c_str(), "Bad nesting");
            nesting.pop_back();
            do_terminate = Termination::None;
        }
        else if (curr.str == "}")
        {
            if (in_struct || in_interface) // we are ending a struct or layout definition (that we care about)
            {
                if (!struct_name.empty() && in_struct) // is a definition (possibly among other things)
                {
                    structs[struct_name] = nesting.back();
                }
                curr = scan_token(feed); // we will get either a name or a ';'

                if (curr.str != ";") // delay termination (could be an array eg)
                {
                    do_terminate = in_struct ? Termination::StructScope : Termination::InterfaceScope;
                    nesting.back().name = curr.str;
                }
                else
                {
                    if (in_interface)
                    {
                        ret.global.members.push_back(nesting.back());
                    }
                    struct_name = std::string();
                    in_struct = false;
                    state = Keyword::None;
                    in_interface = false;
                    var = GLSLRepresentation::Variable();
                    ASSERT(nesting.size() > 0, curr.str.c_str(), "Bad nesting");
                    nesting.pop_back();
                }
            }
            else
            {
                ASSERT(block_depth > 0, feed.c_str(), "Too many scopes ending!");
                block_depth--;
            }
        }
        else if (block_depth > 0)
        {
            continue; // don't care about the actual meat of the shader
        }
        // all the below are now only checked if in the global scope
        else if (curr.str == "[")
        {
            in_array = true;
            var.dimensions += curr.str;
        }
        else if (curr.str == "]")
        {
            in_array = false;
            var.dimensions += curr.str;
        }
        else if (in_array && curr.str.size() > 0)
        {
            var.dimensions += curr.str;
            char *end = nullptr;
            long value = strtol(curr.str.c_str(), &end, 10);
            if (end != nullptr && *end == '\0') // valid integer value
            {
                var.size += value;
            }
        }
        // Are we ending a variable declaration?
        else if (curr.str == ";" && state != Keyword::None && !var.name.empty())
        {
            GLSLRepresentation::Variable& target = (in_struct || in_interface) ? nesting.back() : ret.global;

            if (var.precision == Keyword::None && default_precision.count(var.type) > 0)
            {
                var.precision = default_precision.at(var.type);
            }
            else if (var.precision == Keyword::None) // default precision is highp
            {
                var.precision = Keyword::Highp;
            }

            if (var.dimensions.size() == 0)
            {
                var.size = 1; // default size is 1
            }

            if (state == Keyword::Varying && shader.shaderType != GL_FRAGMENT_SHADER && shader.contains_invariant_all_pragma)
            {
                var.qualifiers.push_back(Keyword::Invariant);
            }

            target.members.push_back(var);
            var = GLSLRepresentation::Variable();
            if (!in_struct && !in_interface)
            {
                state = Keyword::None;
            }
        }
        else if (curr.str == ";") // but not in a recognized keyword state
        {
            // could be setting default layout
            if (state != Keyword::None && var.name.empty())
            {
                state = Keyword::None;
                var = GLSLRepresentation::Variable();
                // TBD do something here
            }
        }
        else if (state != Keyword::None && var.type != Keyword::None && !var.name.empty() && curr.str == "(") // function!
        {
            state = Keyword::None; // ignore it
            var = GLSLRepresentation::Variable();
        }
        else if (state != Keyword::None && curr.keywordType != KeywordType::Qualifier) // pick up name and type
        {
            if (curr.keywordType == KeywordType::Type)
            {
                switch (state)
                {
                case Keyword::Uniform:
                case Keyword::Varying:
                case Keyword::Attribute:
                case Keyword::In:
                case Keyword::Out:
                case Keyword::Buffer:
                    var.storage = state;
                    break;
                default: break;
                }
                var.type = curr.keyword;
            }
            else if (structs.count(curr.str) > 0)
            {
                do_terminate = Termination::StructMember;
                nesting.push_back(structs.at(curr.str));
                nesting.back().name = curr.str;
                ASSERT(nesting.size() <= 2, curr.str.c_str(), "Bad nesting");
            }
            else if (curr.type == Token::IDENTIFIER)
            {
                var.name = curr.str;
            }
        }
        else if (curr.type == Token::KEYWORD) // we only care about globals
        {
            switch (curr.keyword)
            {
            case Keyword::Struct:
            {
                Token type_name = scan_token(feed); // either name or {
                in_struct = true;
                ASSERT(type_name.str != "{", feed.c_str(), "Anonymous structs are forbidden");
                if (type_name.str != "{") // named struct
                {
                    struct_name = type_name.str;
                }
                state = curr.keyword;
                break;
            }
            case Keyword::Highp:
            case Keyword::Mediump:
            case Keyword::Lowp:
                var.storage = state;
                var.precision = curr.keyword;
                break;
            case Keyword::Uniform:
            case Keyword::Varying:
            case Keyword::Attribute:
            case Keyword::In:
            case Keyword::Out:
            case Keyword::Buffer:
                var.storage = state;
                state = curr.keyword;
                break;
            case Keyword::Invariant:
                ret.contains_invariants = true; // fall-through
            case Keyword::Centroid:
            case Keyword::Sample:
            case Keyword::Patch:
            case Keyword::Flat:
            case Keyword::Precise:
            case Keyword::Coherent:
            case Keyword::Volatile:
            case Keyword::Restrict:
            case Keyword::Readonly:
            case Keyword::Writeonly:
                var.qualifiers.push_back(curr.keyword);
                break;
            case Keyword::Precision: // changing default precision on global scope
            {
                Token precision = scan_token(feed);
                Token type = scan_token(feed);
                Token semicolon = scan_token(feed);
                ASSERT(semicolon.str == ";", semicolon.str.c_str(), "Semicolon expected in default precision");
                default_precision[type.keyword] = precision.keyword;
                break;
            }
            // TBD: "layout(early_fragment_tests) in;" special case, see 4.4.1.3 in spec
            case Keyword::Layout:
            {
                var.layout = curr.str;
                while (curr.str != ")")
                {
                    curr = scan_token(feed);
                    var.layout += curr.str;
                    ASSERT(curr.type != Token::EMPTY, feed.c_str(), "Layout qualifier not terminated with right parenthesis!");
                }
                break;
            }
            default:
                break;
            }
        }
    }
    return ret;
}

int GLSLParser::shaderType(const std::string& extension)
{
    if (extension == ".vert")
    {
        return GL_VERTEX_SHADER;
    }
    else if (extension == ".frag")
    {
        return GL_FRAGMENT_SHADER;
    }
    else if (extension == ".tess" || extension == ".tese")
    {
        return GL_TESS_EVALUATION_SHADER;
    }
    else if (extension == ".tscc" || extension == ".tesc")
    {
        return GL_TESS_CONTROL_SHADER;
    }
    else if (extension == ".geom")
    {
        return GL_GEOMETRY_SHADER;
    }
    else if (extension == ".comp")
    {
        return GL_COMPUTE_SHADER;
    }
    else
    {
        return 0;
    }
}
