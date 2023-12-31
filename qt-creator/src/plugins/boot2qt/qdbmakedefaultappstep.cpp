// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbmakedefaultappstep.h"

#include "qdbconstants.h"
#include "qdbtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

#include <utils/commandline.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace Qdb::Internal {

// QdbMakeDefaultAppService

class QdbMakeDefaultAppService : public RemoteLinux::AbstractRemoteLinuxDeployService
{
public:
    void setMakeDefault(bool makeDefault) { m_makeDefault = makeDefault; }

private:
    bool isDeploymentNecessary() const final { return true; }

    Group deployRecipe() final
    {
        const auto setupHandler = [this](QtcProcess &process) {
            QString remoteExe;
            if (RunConfiguration *rc = target()->activeRunConfiguration()) {
                if (auto exeAspect = rc->aspect<ExecutableAspect>())
                    remoteExe = exeAspect->executable().nativePath();
            }
            CommandLine cmd{deviceConfiguration()->filePath(Constants::AppcontrollerFilepath)};
            if (m_makeDefault && !remoteExe.isEmpty())
                cmd.addArgs({"--make-default", remoteExe});
            else
                cmd.addArg("--remove-default");
            process.setCommand(cmd);
            QtcProcess *proc = &process;
            connect(proc, &QtcProcess::readyReadStandardError, this, [this, proc] {
                emit stdErrData(proc->readAllStandardError());
            });
        };
        const auto doneHandler = [this](const QtcProcess &) {
            if (m_makeDefault)
                emit progressMessage(Tr::tr("Application set as the default one."));
            else
                emit progressMessage(Tr::tr("Reset the default application."));
        };
        const auto errorHandler = [this](const QtcProcess &process) {
            emit errorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
        };
        return Group { Process(setupHandler, doneHandler, errorHandler) };
    }

    bool m_makeDefault = true;
};

// QdbMakeDefaultAppStep

class QdbMakeDefaultAppStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
public:
    QdbMakeDefaultAppStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new QdbMakeDefaultAppService;
        setDeployService(service);

        auto selection = addAspect<SelectionAspect>();
        selection->setSettingsKey("QdbMakeDefaultDeployStep.MakeDefault");
        selection->addOption(Tr::tr("Set this application to start by default"));
        selection->addOption(Tr::tr("Reset default application"));

        setInternalInitializer([service, selection] {
            service->setMakeDefault(selection->value() == 0);
            return service->isDeploymentPossible();
        });
    }
};


// QdbMakeDefaultAppStepFactory

QdbMakeDefaultAppStepFactory::QdbMakeDefaultAppStepFactory()
{
    registerStep<QdbMakeDefaultAppStep>(Constants::QdbMakeDefaultAppStepId);
    setDisplayName(Tr::tr("Change default application"));
    setSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // Qdb::Internal
