#include <QTemporaryDir>
#include <QtQuickTest/quicktest.h>

int main(int argc, char **argv)
{
    QTemporaryDir accountsDir;
    if (Q_UNLIKELY(!accountsDir.isValid())) {
        qFatal("Could not create temporary directory!");
        return EXIT_FAILURE;
    }

    qputenv("ACCOUNTS", accountsDir.path().toUtf8());
    qputenv("AG_APPLICATIONS", TEST_DATA_DIR);
    qputenv("AG_SERVICES", TEST_DATA_DIR);
    qputenv("AG_SERVICE_TYPES", TEST_DATA_DIR);
    qputenv("AG_PROVIDERS", TEST_DATA_DIR);
    qputenv("XDG_DATA_HOME", TEST_DATA_DIR);

    return quick_test_main(argc, argv, "online_accounts_qml", QUICK_TEST_SOURCE_DIR);
}
