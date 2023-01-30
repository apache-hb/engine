#pragma once

#include <unordered_set>
#include <format>

namespace simcoe::logging {
    struct Category;
    struct ISink;

    enum Level {
        eInfo,
        eWarn,
        eFatal,

        eTotal
    };

    struct Category final {
        using SinkSet = std::unordered_set<ISink*>;
        
        Category(Level level, const char *pzName)
            : logLevel(level)
            , pzName(pzName)
        { }

        void send(Level level, const char *pzMessage);

        void info(const char *pzMessage, auto&&... args) {
            send(eInfo, std::vformat(pzMessage, std::make_format_args(args...)).c_str());
        }

        void warn(const char *pzMessage, auto&&... args) {
            send(eWarn, std::vformat(pzMessage, std::make_format_args(args...)).c_str());
        }

        void fatal(const char *pzMessage, auto&&... args) {
            send(eFatal, std::vformat(pzMessage, std::make_format_args(args...)).c_str());
        }

        void addSink(ISink *pSink);
        void removeSink(ISink *pSink);

        constexpr Level getLevel() const { return logLevel; }
        constexpr const char *getName() const { return pzName; }

        const SinkSet& getSinks() const { return sinks; }

    private:
        Level logLevel;
        const char *pzName;

        SinkSet sinks;
    };

    struct ISink {
        virtual void send(Category &category, Level level, const char *pzMessage) = 0;

        constexpr const char *getName() const { return pzName; }

    protected:
        constexpr ISink(const char *pzName)
            : pzName(pzName)
        { }

        virtual ~ISink() = default;

    private:
        const char *pzName;
    };

    struct ConsoleSink final : ISink {
        ConsoleSink(const char *pzName = "console");
        
        void send(Category &category, Level level, const char *pzMessage) override;
    };

    struct FileSink final : ISink {
        FileSink(const char *pzName, const char *pzPath);

        void send(Category &category, Level level, const char *pzMessage) override;

    private:
        FILE *pFile;
    };

    struct DebugSink final : ISink {
        DebugSink() : ISink("debug") { }

        void send(Category &category, Level level, const char *pzMessage) override;
    };
}
