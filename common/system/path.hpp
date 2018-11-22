#ifndef _INCLUDE_PATH_HPP_
#define _INCLUDE_PATH_HPP_

#include <vector>
#include <string>
#ifdef WIN32
#else
    #include <dirent.h>
#endif

namespace pat
{

class Path
{
public:
    static const char Sep;
    static bool Exists(const std::string &p);
    static bool IsDirectory(const std::string &p);

    Path();
    Path(const std::string &p);
    operator const char *() const;
    bool Exists() const;
    bool IsDirectory() const;
    const std::string GetExtension() const;

    Path& operator+=(const Path &p);
    bool operator==(const Path &p) const;

private:
    std::string _path;
};

const Path operator+(Path p0, const Path &p1);

class DirectoryIterator
{
public:
    DirectoryIterator(const std::string &p);
    ~DirectoryIterator();

    void Reset();
    bool Next(Path &p);

private:
#ifdef WIN32
#else
    DIR * _handle;
#endif
};

// non-recursive version
bool SearchUnderPath(const std::string &dir, const std::string &file);

} // namespace pat

#endif // _INCLUDE_PATH_HPP_

