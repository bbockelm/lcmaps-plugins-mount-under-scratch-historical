
dnl ============================================================================
dnl  http://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_0x.html
dnl ============================================================================
dnl
dnl SYNOPSIS
dnl
dnl   AX_CXX_COMPILE_STDCXX_0X
dnl
dnl DESCRIPTION
dnl
dnl   Check for baseline language coverage in the compiler for the C++0x
dnl   standard.
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
dnl
dnl   Copying and distribution of this file, with or without modification, are
dnl   permitted in any medium without royalty provided the copyright notice
dnl   and this notice are preserved. This file is offered as-is, without any
dnl   warranty.

dnl serial 7

AU_ALIAS([AC_CXX_COMPILE_STDCXX_0X], [AX_CXX_COMPILE_STDCXX_0X])
AC_DEFUN([AX_CXX_COMPILE_STDCXX_0X], [
  AC_CACHE_CHECK(if g++ supports C++0x features without additional flags,
  ax_cv_cxx_compile_cxx0x_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx0x_native=yes, ax_cv_cxx_compile_cxx0x_native=no)
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++0x features with -std=c++0x,
  ax_cv_cxx_compile_cxx0x_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++0x"
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx0x_cxx=yes, ax_cv_cxx_compile_cxx0x_cxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++0x features with -std=gnu++0x,
  ax_cv_cxx_compile_cxx0x_gxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=gnu++0x"
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx0x_gxx=yes, ax_cv_cxx_compile_cxx0x_gxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  if test "$ax_cv_cxx_compile_cxx0x_native" = yes ||
     test "$ax_cv_cxx_compile_cxx0x_cxx" = yes ||
     test "$ax_cv_cxx_compile_cxx0x_gxx" = yes; then
    AC_DEFINE(HAVE_STDCXX_0X,,[Define if g++ supports C++0x features. ])
  fi

  if test "$ax_cv_cxx_compile_cxx0x_native" != yes; then
  	CXX0X_CFLAGS=""
  	GNUXX0X_CFLAGS=""
  	if test "$ax_cv_cxx_compile_cxx0x_cxx" = yes; then
  		CXX0X_CFLAGS="-std=c++0x"
  	fi
  	if test "$ax_cv_cxx_compile_cxx0x_gxx" = yes; then
  		GNUXX0X_CFLAGS="-std=gnu++0x"
  	fi
  fi
  AC_SUBST(CXX0X_CFLAGS)
  AC_SUBST(GNUXX0X_CFLAGS)

])

dnl ===============================================================================
dnl  http://www.gnu.org/software/autoconf-archive/ax_cxx_header_unordered_set.html
dnl ===============================================================================
dnl
dnl SYNOPSIS
dnl
dnl   AX_CXX_HEADER_UNORDERED_MAP
dnl
dnl DESCRIPTION
dnl
dnl   Check whether the C++ include <unordered_set> exists and define
dnl   HAVE_UNORDERED_MAP if it does.
dnl
dnl LICENSE
dnl
dnl   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
dnl
dnl   Copying and distribution of this file, with or without modification, are
dnl   permitted in any medium without royalty provided the copyright notice
dnl   and this notice are preserved. This file is offered as-is, without any
dnl   warranty.

dnl serial 6

AU_ALIAS([AC_CXX_HEADER_UNORDERED_MAP], [AX_CXX_HEADER_UNORDERED_MAP])
AC_DEFUN([AX_CXX_HEADER_UNORDERED_MAP], [
  AC_CACHE_CHECK(for unordered_set,
  ax_cv_cxx_unordered_set,
  [AC_REQUIRE([AC_CXX_COMPILE_STDCXX_0X])
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=gnu++0x"
  AC_TRY_COMPILE([#include <unordered_set>], [using std::unordered_set;],
  ax_cv_cxx_unordered_set=yes, ax_cv_cxx_unordered_set=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])
  if test "$ax_cv_cxx_unordered_set" = yes; then
    AC_DEFINE(HAVE_UNORDERED_MAP,,[Define if unordered_set is present. ])
  fi
])

