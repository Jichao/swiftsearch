#pragma once

#include "targetver.h"
#include "WinDDKFixes.hpp"

#include <assert.h>
#include <math.h>
#include <process.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include <boost/range/algorithm/equal.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <Windows.h>
#include <WinUser.h>
#include <ComDef.h>
#include <ShellApi.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <ShlGuid.h>
#include <CommCtrl.h>
#include <ntddscsi.h>
#include <WinIoCtl.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlmisc.h>
extern ATL::CComModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atltheme.h>

#include "NtObjectMini.hpp"
