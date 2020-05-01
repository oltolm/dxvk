#include <numeric>

#include "util_env.h"

#include "./com/com_include.h"

namespace dxvk::env {

  std::wstring getEnvVar(const wchar_t* name) {
    std::wstring result;
    result.resize(MAX_PATH);

    DWORD len = ::GetEnvironmentVariableW(name, result.data(), MAX_PATH);
    result.resize(len);

    return result;
  }

  std::string getEnvVar(const char* name) {
    return str::fromws(getEnvVar(str::tows(name).c_str()).c_str());
  }

  size_t matchFileExtension(const std::string& name, const char* ext) {
    auto pos = name.find_last_of('.');

    if (pos == std::string::npos)
      return pos;

    bool matches = std::accumulate(name.begin() + pos + 1, name.end(), true,
      [&ext] (bool current, char a) {
        if (a >= 'A' && a <= 'Z')
          a += 'a' - 'A';
        return current && *ext && a == *(ext++);
      });

    return matches ? pos : std::string::npos;
  }


  
  std::filesystem::path getExeName() {
    return getExePath().filename();
  }


  std::string getExeBaseName() {
    auto exeName = getExeName().string();
    auto extp = matchFileExtension(exeName, "exe");

    if (extp != std::string::npos)
      exeName.erase(extp);

    return exeName;
  }


  std::filesystem::path getExePath() {
    std::wstring exePath;
    exePath.resize(MAX_PATH);

    DWORD len = ::GetModuleFileNameW(NULL, exePath.data(), MAX_PATH);
    exePath.resize(len);

    return exePath;
  }
  
  
  void setThreadName(const std::string& name) {
    using SetThreadDescriptionProc = HRESULT (WINAPI *) (HANDLE, PCWSTR);

    static auto proc = reinterpret_cast<SetThreadDescriptionProc>(
      ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription"));

    if (proc != nullptr) {
      auto wideName = str::tows(name.c_str());
      (*proc)(::GetCurrentThread(), wideName.c_str());
    }
  }


  bool createDirectory(const std::filesystem::path& path) {
    return std::filesystem::create_directory(path);
  }
  
}
