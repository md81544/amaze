#pragma once

#include <string>
#include <stdexcept>
#include <typeinfo>

namespace amaze
{
namespace exceptions
{

#ifdef __linux
#define THROWUP( exception, message ) \
throw exception( __FILE__, __LINE__, __func__, message )
#else
#define THROWUP( exception, message ) \
throw exception( __FILE__, __LINE__, __FUNCSIG__, message )
#endif


class AmazeBaseException : public std::exception
{
public:
    AmazeBaseException(
        const char *exceptionFile,
        const unsigned int exceptionLine,
        const char *exceptionFunction,
        const std::string &message
    ) :
        fileName( exceptionFile ),
        lineNumber( exceptionLine ),
        functionSignature( exceptionFunction ),
        details( message )
    {}
    virtual ~AmazeBaseException() {};

    virtual const char *what() const noexcept override
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
    std::string  fileName;
    unsigned int lineNumber;
    std::string  functionSignature;
    std::string  details;
};

// Exception thrown if the application cannot start up correctly.
class AmazeStartupException : public AmazeBaseException
{
public:
    AmazeStartupException(
        const char *file,
        unsigned int line,
        const char *function,
        const std::string &message
    ) :
        AmazeBaseException( file, line, function, message ) {}
};

// Exception thrown if the application encounters a runtime error
class AmazeRuntimeException : public AmazeBaseException
{
public:
    AmazeRuntimeException(
        const char *file,
        unsigned int line,
        const char *function,
        const std::string &message
    ) :
        AmazeBaseException( file, line, function, message ) {}
};

} // namespace exceptions
} // namespace amaze
