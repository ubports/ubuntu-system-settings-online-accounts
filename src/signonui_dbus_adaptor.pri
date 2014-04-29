# This script is a modified copy of Qt's mkspecs/features/dbusadaptors.prf,
# written to allow passing extra "-i" options to qdbusxml2cpp

qtPrepareTool(QMAKE_QDBUSXML2CPP, qdbusxml2cpp)

for(SIGNONUI_DBUS_ADAPTOR, $$list($$unique(SIGNONUI_DBUS_ADAPTORS))) {

    !contains(SIGNONUI_DBUS_ADAPTOR, .*\\w\\.xml$) {
        warning("Invalid D-BUS adaptor: '$${DBUS_ADAPTOR}', please use 'com.mydomain.myinterface.xml' instead.")
        next()
    }

    SIGNONUI_DBUS_ADAPTOR_LIST += $${SIGNONUI_DBUS_ADAPTOR}
}

for(SIGNONUI_DBUS_INCLUDE, $$list($$unique(SIGNONUI_DBUS_INCLUDES))) {

    QDBUS_EXTRA_OPTIONS += -i $${SIGNONUI_DBUS_INCLUDE}
}

signonui_dbus_adaptor_header.commands = $$QMAKE_QDBUSXML2CPP -a ${QMAKE_FILE_OUT}: ${QMAKE_FILE_IN}
signonui_dbus_adaptor_header.output_function = signonui_dbus_adaptor_header_output
signonui_dbus_adaptor_header.name = DBUSXML2CPP ADAPTOR HEADER ${QMAKE_FILE_IN}
signonui_dbus_adaptor_header.variable_out = SIGNONUI_DBUS_ADAPTOR_HEADERS
signonui_dbus_adaptor_header.input = SIGNONUI_DBUS_ADAPTOR_LIST

defineReplace(signonui_dbus_adaptor_header_output) {
    return("$$lower($$section($$list($$basename(1)),.,-2,-2))_adaptor.h")
}

signonui_dbus_adaptor_source.commands = $$QMAKE_QDBUSXML2CPP $$QDBUS_EXTRA_OPTIONS -i ${QMAKE_FILE_OUT_BASE}.h -a :${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
signonui_dbus_adaptor_source.output_function = signonui_dbus_adaptor_source_output
signonui_dbus_adaptor_source.name = DBUSXML2CPP ADAPTOR SOURCE ${QMAKE_FILE_IN}
signonui_dbus_adaptor_source.variable_out = SOURCES
signonui_dbus_adaptor_source.input = SIGNONUI_DBUS_ADAPTOR_LIST

load(moc)
signonui_dbus_adaptor_moc.commands = $$moc_header.commands
signonui_dbus_adaptor_moc.output = $$moc_header.output
signonui_dbus_adaptor_moc.depends = $$signonui_dbus_adaptor_header.output
signonui_dbus_adaptor_moc.input = SIGNONUI_DBUS_ADAPTOR_HEADERS
signonui_dbus_adaptor_moc.variable_out = GENERATED_SOURCES
signonui_dbus_adaptor_moc.name = $$moc_header.name

defineReplace(signonui_dbus_adaptor_source_output) {
    return("$$lower($$section($$list($$basename(1)),.,-2,-2))_adaptor.cpp")
}

QMAKE_EXTRA_COMPILERS += signonui_dbus_adaptor_header signonui_dbus_adaptor_source signonui_dbus_adaptor_moc

