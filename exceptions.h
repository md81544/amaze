#pragma once

#include <stdexcept>
#include <string>
#include <typeinfo>

namespace marengo {
namespace amaze {

#ifdef __linux
#define THROWUP(ex, message) throw ex(__FILE__, __LINE__, __func__, message)
#elif __APPLE__
#define THROWUP(ex, message) throw ex(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#else
#define THROWUP(ex, message) throw ex(__FILE__, __LINE__, __FUNCSIG__, message)
#endif

class AmazeBaseException : public std::exception {
public:
    AmazeBaseException(
        const char* exceptionFile,
        const unsigned int exceptionLine,
        const char* exceptionFunction,
        const std::string& message)
        : fileName(exceptionFile)
        , lineNumber(exceptionLine)
        , functionSignature(exceptionFunction)
        , details(message)
    {
    }
    virtual ~AmazeBaseException() {};

    virtual const char* what() const noexcept override
    {
        return details.c_str();
    }
    std::string file() const
    {
        return fileName;
    }
    std::string function() const
    {
        return functionSignature;
    }
    unsigned int line() const
    {
        return lineNumber;
    }

private:
    std::string fileName;
    unsigned int lineNumber;
    std::string functionSignature;
    std::string details;
};

// Exception thrown if the application cannot start up correctly.
class AmazeStartupException : public AmazeBaseException {
public:
    AmazeStartupException(
        const char* file,
        unsigned int line,
        const char* function,
        const std::string& message)
        : AmazeBaseException(file, line, function, message)
    {
    }
};

// Exception thrown if the application encounters a runtime error
class AmazeRuntimeException : public AmazeBaseException {
public:
    AmazeRuntimeException(
        const char* file,
        unsigned int line,
        const char* function,
        const std::string& message)
        : AmazeBaseException(file, line, function, message)
    {
    }
};

} // namespace amaze
} // namespace marengo
