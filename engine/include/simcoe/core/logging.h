#pragma once

#include <unordered_set>
#include <format>

namespace simcoe::logging {
    struct Category;
    struct ISink;

    enum Level : unsigned {
#define LEVEL(id, name) id,
#include "simcoe/core/logging.inc"
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

    struct IFilterSink : ISink {
        IFilterSink(const char *pzName) : ISink(pzName) { }
        void send(Category &category, Level level, const char *pzMessage) override final;

        virtual void accept(Category &category, Level level, const char *pzMessage) = 0;
    };

    struct ConsoleSink final : IFilterSink {
        ConsoleSink(const char *pzName = "console");

        void accept(Category &category, Level level, const char *pzMessage) override;
    };

    struct FileSink final : IFilterSink {
        FileSink(const char *pzName, const char *pzPath);

        void accept(Category &category, Level level, const char *pzMessage) override;

    private:
        FILE *pFile;
    };

    struct DebugSink final : IFilterSink {
        DebugSink() : IFilterSink("debug") { }

        void accept(Category &category, Level level, const char *pzMessage) override;
    };
}
