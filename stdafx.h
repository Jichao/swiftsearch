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
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <boost/bind/bind.hpp>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/equal_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/stable_sort.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/join.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>

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
