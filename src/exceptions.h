#pragma once

#include <source_location>
#include <stdexcept>
#include <string>

namespace marengo {
namespace amaze {

// The source location defaults to where the exception is constructed, i.e.
// the throw site. These are logged where they are caught, in main().
class AmazeBaseException : public std::runtime_error {
public:
    explicit AmazeBaseException(
        const std::string& message,
        const std::source_location& loc = std::source_location::current())
        : std::runtime_error(message)
        , m_location(loc)
    {
    }

    const std::source_location& location() const
    {
        return m_location;
    }

private:
    std::source_location m_location;
};

// Exception thrown if the application cannot start up correctly.
class AmazeStartupException : public AmazeBaseException {
public:
    using AmazeBaseException::AmazeBaseException;
};

// Exception thrown if the application encounters a runtime error
class AmazeRuntimeException : public AmazeBaseException {
public:
    using AmazeBaseException::AmazeBaseException;
};

} // namespace amaze
} // namespace marengo
