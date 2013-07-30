# Include this file after defining the pkgconfig.files variable

!isEmpty(pkgconfig.files) {
    QMAKE_SUBSTITUTES += $${pkgconfig.files}.in
    pkgconfig.CONFIG = no_check_exist
    pkgconfig.path  = $${INSTALL_PREFIX}/lib/pkgconfig
    QMAKE_EXTRA_TARGETS += pkgconfig

    INSTALLS += pkgconfig

    QMAKE_CLEAN += $${pkgconfig.files}
}
