#pragma once

#include <comdef.h>
#include <spdlog/spdlog.h>

#include <iostream>

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