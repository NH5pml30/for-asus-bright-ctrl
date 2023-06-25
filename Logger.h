#pragma once

#define _WIN32_WINNT NTDDI_WIN10

#include <functional>
#include <ostream>
#include <sstream>
#include <string_view>

class Logger {
public:
  enum class LogLevel { Info, Warning, Error };

  static inline constexpr LogLevel CurLevel =
#ifdef NDEBUG
      LogLevel::Error
#else
      LogLevel::Info
#endif
      ;

private:
  static consteval std::string_view LogLevelToString(LogLevel Lvl) {
    switch (Lvl) {
    case LogLevel::Info:
      return "Info";
    case LogLevel::Warning:
      return "Warn";
    case LogLevel::Error:
      return "Error";
    }
  }

public:
  static void initLog() {
    logger.printer = [](std::string_view) {};
  }

  template <typename T> static void initLog(T &&O) {
    logger.printer = [MovedO = std::move(O)](std::string_view sv) mutable {
      MovedO << sv << std::flush;
    };
  }

  static void initLog(std::ostream &O) {
    logger.printer = [&](std::string_view sv) { O << sv << std::flush; };
  }

  template <LogLevel Lvl = LogLevel::Info, typename... ArgsT>
  static Logger &print(ArgsT &&...Args) {
    if constexpr (Lvl >= CurLevel)
      return logger.printImpl("[", LogLevelToString(Lvl), "] ",
                              std::forward<ArgsT>(Args)...);
    else
      return logger;
  }

private:
  static Logger logger;

  Logger() = default;

  template <typename... ArgsT> Logger &printImpl(ArgsT &&...Args) {
    thread_local static std::string buf;
    buf.clear();
    std::stringstream ss(std::move(buf));
    (ss << ... << std::forward<ArgsT>(Args));
    printer(ss.view());
    buf = std::move(ss).str();
    return *this;
  }

  std::move_only_function<void(std::string_view)> printer =
      [](std::string_view) {};
};

static inline constexpr auto LOG_MODULE = "";

#define LOG_NV(LVL, ...) Logger::print<Logger::LogLevel::LVL>(__VA_ARGS__)
#define LOG_NV_LN(LVL, ...) LOG_NV(LVL, __VA_ARGS__, '\n')

#define LOG_V(LVL, ...)                                                        \
  LOG_NV(LVL, "[", LOG_MODULE, "] ", __func__, " at ", __FILE__, ':',          \
         __LINE__, ": \n> ", __VA_ARGS__)
#define LOG_V_LN(LVL, ...) LOG_V(LVL, __VA_ARGS__, '\n')

#define LOGI_V_LN(...) LOG_V_LN(Info, __VA_ARGS__)
#define LOGW_V_LN(...) LOG_V_LN(Warning, __VA_ARGS__)
#define LOGE_V_LN(...) LOG_V_LN(Error, __VA_ARGS__)

#define LOGI_V(...) LOG_V(Info, __VA_ARGS__)
#define LOGW_V(...) LOG_V(Warning, __VA_ARGS__)
#define LOGE_V(...) LOG_V(Error, __VA_ARGS__)

#define LOGI_NV_LN(...) LOG_NV_LN(Info, __VA_ARGS__)
#define LOGW_NV_LN(...) LOG_NV_LN(Warning, __VA_ARGS__)
#define LOGE_NV_LN(...) LOG_NV_LN(Error, __VA_ARGS__)

#define LOGI_NV(...) LOG_NV(Info, __VA_ARGS__)
#define LOGW_NV(...) LOG_NV(Warning, __VA_ARGS__)
#define LOGE_NV(...) LOG_NV(Error, __VA_ARGS__)
