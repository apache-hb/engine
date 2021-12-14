#include <string>
#include <string_view>
#include <source_location>

export module engine.util.error;

export namespace engine {
    struct Error {
        constexpr Error(std::string_view message = "unknown error", const std::source_location& loc = std::source_location::current())
            : message(message)
            , location(loc)
        { }

        constexpr std::string_view what() const { return message; }

        constexpr std::source_location where() const { return location; }

        constexpr std::uint_least32_t line() const { return location.line(); }
        constexpr std::uint_least32_t column() const { return location.column(); }
        constexpr std::string_view file() const { return location.file_name(); }
        constexpr std::string_view function() const { return location.function_name(); }
        
    protected:
        std::string_view message;
        std::source_location location;
    };
}
