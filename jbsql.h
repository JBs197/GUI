#ifndef JBSQLPLUGIN_H
#define JBSQLPLUGIN_H

#include "jbsql_global.h"

#include <extensionsystem/iplugin.h>

namespace JBSQL {
namespace Internal {

class JBSQLPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "JBSQL.json")

public:
    JBSQLPlugin();
    ~JBSQLPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace Internal
} // namespace JBSQL

#endif // JBSQLPLUGIN_H
