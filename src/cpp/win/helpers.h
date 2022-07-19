#pragma once

#include <comdef.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <codecvt>
#include <string>

#define LOG_ERROR(hr_err, expr_string)                                    \
  _com_error err##__LINE__(hr_err);                                       \
  LPCTSTR errMsg##__LINE__ = err##__LINE__.ErrorMessage();                \
  spdlog::error("FAILED {}:{} {} :: {}", __FILE__, __LINE__, expr_string, \
                errMsg##__LINE__);

#define LOG_IF_FAILED(expr)           \
  {                                   \
    auto _hr##__LINE__ = expr;        \
    if (FAILED(_hr##__LINE__)) {      \
      LOG_ERROR(_hr##__LINE__, #expr) \
    }                                 \
  }

#define IF_SUCCEEDED(expr)        \
  auto _hr##__LINE__ = expr;        \
  if (FAILED(_hr##__LINE__)) {      \
    LOG_ERROR(_hr##__LINE__, #expr) \
  } else                                 \

#define RETURN_IF_FAILED(expr)        \
  {                                   \
    auto _hr##__LINE__ = expr;        \
    if (FAILED(_hr##__LINE__)) {      \
      LOG_ERROR(_hr##__LINE__, #expr) \
      return _hr##__LINE__;           \
    }                                 \
  }

#define DEFAULT_ADDREF_RELEASE()                 \
  ULONG STDMETHODCALLTYPE AddRef() { return 1; } \
  ULONG STDMETHODCALLTYPE Release() { return 1; }

#define QUERYINTERFACE_HELPER() \
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object)

namespace Gloo::Internal::MicDetector::Windows {
  inline std::string to_utf8(const std::wstring& wstr) {
    if( wstr.empty() ) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
  }
}