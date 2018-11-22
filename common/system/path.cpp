#include <cassert>
#ifdef WIN32
#else
    #include <sys/stat.h>
#endif

#include "path.hpp"
#include "base/base.hpp"
#include "common/os.hpp"

namespace pat
{

#ifdef WIN32
    const char Path::Sep = '/'; // both / and \ work on win32
#else
    const char Path::Sep = '/';
#endif

Path::Path()
{
}

Path::Path(const std::string &p)
: _path(p)
{
}

Path::operator const char *() const
{
    return _path.c_str();
}

bool Path::Exists() const
{
    return Path::Exists(_path);
}

bool Path::IsDirectory() const
{
    return Path::IsDirectory(_path);
}

const std::string Path::GetExtension() const
{
    const size_t found = _path.find_last_of('.');
    if (found == std::string::npos)
    {
        return std::string();
    }
    else
    {
        return _path.substr(found + 1);
    }
}

bool Path::Exists(const std::string &p)
{
    if (p.empty())
        return false;

#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
    return false;
#else
    struct stat buf;
    return stat(p.c_str(), &buf) >= 0;
#endif
}

bool Path::IsDirectory(const std::string &p)
{
    PAT_DEBUG_ASSERT(Exists(p), "File not exists : %s\n", p.c_str());

#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
    return false;

#else
    struct stat buf;
    if (stat(p.c_str(), &buf) >= 0)
    {
        return S_ISDIR(buf.st_mode);
    }
#endif

    return false;
}

Path & Path::operator+=(const Path &p)
{
    if (_path[_path.size() - 1] != Path::Sep)
        _path.append(1, Path::Sep);
    _path.append(p);
    return *this;
}

bool Path::operator==(const Path &p) const
{
    return _path == p._path;
}

const Path operator+(Path p0, const Path &p1)
{
    return p0 += p1;
}

DirectoryIterator::DirectoryIterator(const std::string &p)
{
    PAT_DEBUG_ASSERT(Path::Exists(p), "Non-existing path %s\n", p.c_str());
#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
#else
    _handle = ::opendir(p.c_str());
#endif
}

DirectoryIterator::~DirectoryIterator()
{
#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
#else
    ::closedir(_handle);
#endif
}

void DirectoryIterator::Reset()
{
#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
#else
    ::rewinddir(_handle);
#endif
}

bool DirectoryIterator::Next(Path &p)
{
#ifdef WIN32
    DBG_LOG("%s not impl", __FUNCTION__);
    os::abort();
    return false;
#else
    struct dirent *entry = ::readdir(_handle);
    if (entry == NULL)
        return false;

    const std::string name(entry->d_name);

    // Skip "." and ".."
    if (entry->d_type == DT_DIR && (name == "." || name == ".."))
    {
        return Next(p);
    }

    p = Path(name);
    return true;
#endif
}

bool SearchUnderPath(const std::string &dir, const std::string &file)
{
    if (Path::Exists(dir) == false || Path::IsDirectory(dir) == false)
    {
        return false;
    }

    DirectoryIterator iter(dir);
    Path item;
    while (iter.Next(item))
    {
        if (item == file)
        {
            return true;
        }
    }
    return false;
}

} // namespace pat
