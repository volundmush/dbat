#include "dbat/Log.h"
#include <vector>
#include <filesystem>
#include <stdarg.h>

namespace mud::log {

    const char* log_levels[] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "CRITICAL",
    };

    void log(std::source_location loc, int lvl, const char* fmtstr, ...) {
        constexpr size_t MAX_LOG_BUFFER = 8192;
        char buffer[MAX_LOG_BUFFER];

        va_list args;
        va_start(args, fmtstr);
        vsnprintf(buffer, MAX_LOG_BUFFER, fmtstr, args);
        va_end(args);

        // Format: [LEVEL] file:line:function: message
        printf("[%s] %s:%d:%s: %s\n", log_levels[lvl], loc.file_name(), loc.line(), loc.function_name(), buffer);
    }


} // namespace mud::log
