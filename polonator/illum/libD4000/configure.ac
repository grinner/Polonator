AC_INIT([libD4000], [1.0])
AM_INIT_AUTOMAKE

AC_PREFIX_DEFAULT([/])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# verify that we have libusb and libusb-devel installed
AC_CHECK_HEADERS([usb.h], [], [AC_MSG_ERROR([libusb0.1 header (usb.h) not found!])])


# setup firmware target directory (/lib/firmware)
firmwaredir='${prefix}/lib/firmware'
AC_SUBST([firmwaredir])


# Linux hack to switch libdir to lib64, modifed from http://rpm5.org/community/rpm-devel/1120.html
MARK64=
case "$host_os" in
    *linux* )
        if ! echo "${libdir}" | grep -q '64$'; then
            case "${host_cpu}" in
                    x86_64*|ppc64*|powerpc64*|sparc64*|s390x*) MARK64=64 ;;
            esac
        fi
        ;;
esac
AC_ARG_ENABLE(
    [build-lib64],
    AC_HELP_STRING([--disable-build-lib64], [disable automatic use of /usr/lib64 (system dependent)]),
    [ if test ".$enableval" = .no; then MARK64=""; fi ])
AC_SUBST(MARK64)
# end Linux hack


# correct lib-related directories if they were not explicitly specified
if test ${libdir} == '${exec_prefix}/lib'; then
	libdir='${exec_prefix}/usr/lib'${MARK64}
	includedir='${prefix}/usr/include'
fi
echo "libdir = ${libdir}"



AC_CONFIG_FILES([Makefile src/Makefile firmware/Makefile etc/Makefile])
AC_OUTPUT

