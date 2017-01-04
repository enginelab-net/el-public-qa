// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcview.h>

#include <string>
#include <sstream>

extern std::string _normal;
extern std::wstring _wide;
inline const std::string &to_api(const std::string &s) { return s; }
inline const std::string &to_api(const char *s) { _normal = s;  return _normal; }
inline const std::string &to_api(const std::wstring &ws) { _normal.assign(ws.begin(), ws.end()); return _normal; }
inline const std::string &to_api(const wchar_t *ws) { _wide = ws; return to_api(_wide); }

#if defined(_UNICODE)
inline const std::wstring &proper(const wchar_t *ws) { _wide = ws; return _wide; }
inline const std::wstring &proper(const std::wstring &ws) { return ws; }
inline const std::wstring &proper(const std::string &s) { _wide.assign(s.begin(), s.end()); return _wide; }
inline const std::wstring &proper(const char *n) { _normal = n; return proper(_normal); }
typedef std::wstring string;
typedef std::wostringstream ostringstream;
typedef std::wostream ostream;
#else
inline const std::string &proper(const char *n) { _normal = n; return _normal; }
inline const std::string &proper(const std::string &s) { return s; }
inline const std::string &proper(const std::wstring &ws) { _normal.assign(ws.begin(), ws.end()); return _normal; }
inline const std::string &proper(const wchar_t *ws) { _wide = ws; return proper(_wide); }
typedef std::string string;
typedef std::ostringstream ostringstream;
typedef std::ostream ostream;
#endif

