#ifndef PAEXCEPTION_H
#define PAEXCEPTION_H

#include <iostream>
#include <string>
#include <exception>
#include <sstream>

#include "common/os.hpp" // DBG_LOG

class PAException : public std::exception
{
public:
    PAException(const std::string& what, const std::string& file=std::string(), int line=-1)
        : std::exception()
        , m_what(what)
        , m_file(file)
        , m_line(line)
        , m_message()
    {
        std::stringstream ss;
        ss << m_file << "," << m_line << ": " << m_what;
        m_message = ss.str();
    }

    ~PAException() throw() {}

    virtual const char* what() const throw()
    {
        os::log("*********************************\n");
        os::log("EXCEPTION at %s\n", m_message.c_str());
        os::log("*********************************\n");
        return m_message.c_str();
    }

private:
    std::string m_what;
    std::string m_file;
    int m_line;
    std::string m_message;
};

#define PA_EXCEPTION(what) PAException(what, SHORT_FILE, __LINE__)

#endif  /* PAEXCEPTION_H */
