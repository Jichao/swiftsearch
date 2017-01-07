#pragma once

#include "basetypes.h"
#include "nttypes.h"
#include "fstypes.h"

template<class T>
inline T const &use_facet(std::locale const &loc)
{
	return std::
#if defined(_USEFAC)
		_USE(loc, T)
#else
		use_facet<T>(loc)
#endif
		;
}


template<typename Str, typename S1, typename S2>
void replace_all(Str &str, S1 from, S2 to)
{
    size_t const nFrom = Str(
                             from).size();  // SLOW but OK because this function is O(n^2) anyway
    size_t const nTo = Str(to).size();  // SLOW but OK because this function is O(n^2) anyway
    for (size_t i = 0; (i = str.find_first_of(from, i)) != Str::npos; i += nTo) {
        str.erase(i, nFrom);
        str.insert(i, to);
    }
}

LONGLONG RtlSystemTimeToLocalTime(LONGLONG systemTime);
LPCTSTR SystemTimeToString(LONGLONG systemTime, LPTSTR buffer, size_t cchBuffer,
	LPCTSTR dateFormat = NULL, LPCTSTR timeFormat = NULL, LCID lcid = GetThreadLocale());

std::tstring NormalizePath(std::tstring const &path);
std::tstring GetDisplayName(HWND hWnd, const std::tstring &path, DWORD shgdn);
void CheckAndThrow(int const success);
LPTSTR GetAnyErrorText(DWORD errorCode, va_list* pArgList = NULL);

static void append(std::tstring &str, TCHAR const sz[], size_t const cch)
{
    if (str.size() + cch > str.capacity()) {
        str.reserve(str.size() + str.size() / 2 + cch * 2);
    }
    str.append(sz, cch);
}

static void append(std::tstring &str, TCHAR const sz[])
{
    return append(str, sz, std::char_traits<TCHAR>::length(sz));
}


void read(void *const file, unsigned long long const offset, void *const buffer,
          size_t const size, HANDLE const event_handle = NULL);

unsigned int get_cluster_size(void *const volume);

std::vector<std::pair<unsigned long long, long long> > get_mft_retrieval_pointers(
	void *const volume, TCHAR const path[], long long *const size,
	long long const mft_start_lcn, unsigned int const file_record_size);

class iless {
    mutable std::basic_string<TCHAR> s1, s2;
    std::ctype<TCHAR> const &ctype;
    bool logical;
    struct iless_ch {
        iless const *p;
        iless_ch(iless const &l) : p(&l) { }
        bool operator()(TCHAR a, TCHAR b) const
        {
            return _totupper(a) < _totupper(b);
            //return p->ctype.toupper(a) < p->ctype.toupper(b);
        }
    };

    template<class T>
    static T const &use_facet(std::locale const &loc)
    {
        return std::
#if defined(_USEFAC)
               _USE(loc, T)
#else
               use_facet<T>(loc)
#endif
               ;
    }

public:
    iless(std::locale const &loc,
          bool const logical) : ctype(static_cast<std::ctype<TCHAR> const &>
                                          (use_facet<std::ctype<TCHAR> >(loc))), logical(logical) {}
    bool operator()(boost::iterator_range<TCHAR const *> const a,
                    boost::iterator_range<TCHAR const *> const b) const
    {
        s1.assign(a.begin(), a.end());
        s2.assign(b.begin(), b.end());
        return logical ? StrCmpLogicalW(s1.c_str(),
                                        s2.c_str()) < 0 : std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                                iless_ch(*this));
    }
};

template<class It, class ItBuf, class Pred>
void mergesort_level(It const begin, ptrdiff_t const n, ItBuf const buf, Pred comp,
                     bool const in_buf, ptrdiff_t const m, ptrdiff_t const j)
{
    using std::merge;
    using std::min;
    if (in_buf) {
        merge(
            buf + min(n, j), buf + min(n, j + m),
            buf + min(n, j + m), buf + min(n, j + m + m),
            begin + min(n, j),
            comp);
    } else {
        merge(
            begin + min(n, j), begin + min(n, j + m),
            begin + min(n, j + m), begin + min(n, j + m + m),
            buf + min(n, j),
            comp);
    }
}

template<class It, class ItBuf, class Pred>
bool mergesort(It const begin, It const end, ItBuf const buf, Pred comp,
               bool const parallel)  // MUST check the return value!
{
    bool in_buf = false;
    for (ptrdiff_t m = 1, n = end - begin; m < n; in_buf = !in_buf, m += m) {
        ptrdiff_t const k = n + n - m;
#define X() for (ptrdiff_t j = 0; j < k; j += m + m) { mergesort_level<It, ItBuf, Pred>(begin, n, buf, comp, in_buf, m, j); }
#ifdef _OPENMP
        if (parallel) {
            #pragma omp parallel for
            X();
        } else
#endif
        {
            (void) parallel;
            X();
        }
#undef X
    }
    // if (in_buf) { using std::swap_ranges; swap_ranges(begin, end, buf), in_buf = !in_buf; }
    return in_buf;
}

template<class It, class Pred>
void inplace_mergesort(It const begin, It const end, Pred const &pred,
                       bool const parallel)
{
    class buffer_ptr {
        buffer_ptr(buffer_ptr const &) { }
        void operator =(buffer_ptr const &) { }
        typedef typename std::iterator_traits<It>::value_type value_type;
        typedef value_type *pointer;
        typedef size_t size_type;
        pointer p;
    public:
        typedef pointer iterator;
        ~buffer_ptr()
        {
            delete [] this->p;
        }
        explicit buffer_ptr(size_type const n) : p(new value_type[n]) { }
        iterator begin()
        {
            return this->p;
        }
    } buf(static_cast<size_t>(std::distance(begin, end)));
    if (mergesort<It, typename buffer_ptr::iterator, Pred>(begin, end, buf.begin(), pred,
            parallel)) {
        using std::swap_ranges;
        swap_ranges(begin, end, buf.begin());
    }
}

unsigned long get_num_threads();
DWORD WINAPI SHOpenFolderAndSelectItemsThread(IN LPVOID lpParameter);

void remove_path_stream_and_trailing_sep(std::tstring &path);
