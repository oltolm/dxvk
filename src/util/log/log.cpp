#include "log.h"

#include "../util_env.h"

namespace dxvk {
  
  Logger::Logger(const std::wstring& file_name)
  : m_minLevel(getMinLogLevel()) {
    if (m_minLevel != LogLevel::None) {
      auto path = getFileName(file_name);

      if (!path.empty())
        m_fileStream = std::ofstream(path);
    }
  }
  
  
  Logger::~Logger() { }
  
  
  void Logger::trace(const std::string& message) {
    s_instance.emitMsg(LogLevel::Trace, message);
  }
  
  
  void Logger::debug(const std::string& message) {
    s_instance.emitMsg(LogLevel::Debug, message);
  }
  
  
  void Logger::info(const std::string& message) {
    s_instance.emitMsg(LogLevel::Info, message);
  }
  
  
  void Logger::warn(const std::string& message) {
    s_instance.emitMsg(LogLevel::Warn, message);
  }
  
  
  void Logger::err(const std::string& message) {
    s_instance.emitMsg(LogLevel::Error, message);
  }
  
  
  void Logger::log(LogLevel level, const std::string& message) {
    s_instance.emitMsg(level, message);
  }
  
  
  void Logger::emitMsg(LogLevel level, const std::string& message) {
    if (level >= m_minLevel) {
      std::lock_guard<dxvk::mutex> lock(m_mutex);
      
      static std::array<const char*, 5> s_prefixes
        = {{ "trace: ", "debug: ", "info:  ", "warn:  ", "err:   " }};
      
      const char* prefix = s_prefixes.at(static_cast<uint32_t>(level));

      std::stringstream stream(message);
      std::string       line;

      while (std::getline(stream, line, '\n')) {
        std::cerr << prefix << line << std::endl;

        if (m_fileStream)
          m_fileStream << prefix << line << std::endl;
      }
    }
  }
  
  
  LogLevel Logger::getMinLogLevel() {
    const std::array<std::pair<const char*, LogLevel>, 6> logLevels = {{
      { "trace", LogLevel::Trace },
      { "debug", LogLevel::Debug },
      { "info",  LogLevel::Info  },
      { "warn",  LogLevel::Warn  },
      { "error", LogLevel::Error },
      { "none",  LogLevel::None  },
    }};
    
    const std::string logLevelStr = env::getEnvVar("DXVK_LOG_LEVEL");
    
    for (const auto& pair : logLevels) {
      if (logLevelStr == pair.first)
        return pair.second;
    }
    
    return LogLevel::Info;
  }
  
  
  std::filesystem::path Logger::getFileName(const std::wstring& base) {
    std::filesystem::path path = env::getEnvVar(L"DXVK_LOG_PATH");
    
    if (path == L"none")
      return L"";

    return path / (env::getExeName().stem().wstring() + L"_" + base);
  }
  
}
