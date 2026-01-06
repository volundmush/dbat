#include "dbat/Log.h"
#include <vector>
#include <filesystem>
#include <stdarg.h>

namespace mud::log {

    LogOutputter custom_log_outputter = nullptr;

    const char* log_levels[] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "CRITICAL",
    };

    void log(std::source_location loc, int lvl, const char* fmtstr, ...) {
        std::string output;
        va_list args;
        va_start(args, fmtstr);
        auto size = vsnprintf(nullptr, 0, fmtstr, args);
        va_end(args);
        output.resize(size + 1);
        va_start(args, fmtstr);
        vsnprintf(output.data(), size + 1, fmtstr, args);
        va_end(args);

        if(custom_log_outputter) {
            
            custom_log_outputter(loc.file_name(), loc.function_name(), loc.line(), loc.column(), lvl, output);
            return;
        }

        // Format: [LEVEL] file:line:function: message
        printf("[%s] %s:%d:%s: %s\n", log_levels[lvl], loc.file_name(), loc.line(), loc.function_name(), output.c_str());
    }


} // namespace mud::log
