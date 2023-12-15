// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyconstants.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblyrunconfigurationaspects.h"
#include "webassemblytr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

static FilePath pythonInterpreter(const Environment &env)
{
    const QString emsdkPythonEnvVarKey("EMSDK_PYTHON");
    if (env.hasKey(emsdkPythonEnvVarKey))
        return FilePath::fromUserInput(env.value(emsdkPythonEnvVarKey));

    // FIXME: Centralize addPythonsFromPath() from the Python plugin and use that
    for (const char *interpreterCandidate : {"python3", "python", "python2"}) {
        const FilePath interpereter = env.searchInPath(QLatin1String(interpreterCandidate));
        if (interpereter.isExecutableFile())
            return interpereter;
    }
    return {};
}

static CommandLine emrunCommand(const Target *target,
                                const QString &buildKey,
                                const QString &browser,
                                const QString &port)
{
    if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
        const Environment env = bc->environment();
        const FilePath emrun = env.searchInPath("emrun");
        const FilePath emrunPy = emrun.absolutePath().pathAppended(emrun.baseName() + ".py");
        const FilePath targetPath = bc->buildSystem()->buildTarget(buildKey).targetFilePath;
        const FilePath html = targetPath.absolutePath() / (targetPath.baseName() + ".html");

        QStringList args(emrunPy.path());
        if (!browser.isEmpty()) {
            args.append("--browser");
            args.append(browser);
        }
        args.append("--port");
        args.append(port);
        args.append("--no_emrun_detect");
        args.append("--serve_after_close");
        args.append(html.toString());

        return CommandLine(pythonInterpreter(env), args);
    }
    return {};
}

// Runs a webassembly application via emscripten's "emrun" tool
// https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html
class EmrunRunConfiguration : public ProjectExplorer::RunConfiguration
{
public:
    EmrunRunConfiguration(Target *target, Utils::Id id)
            : RunConfiguration(target, id)
    {
        auto webBrowserAspect = addAspect<WebBrowserSelectionAspect>(target);

        auto effectiveEmrunCall = addAspect<StringAspect>();
        effectiveEmrunCall->setLabelText(Tr::tr("Effective emrun call:"));
        effectiveEmrunCall->setDisplayStyle(StringAspect::TextEditDisplay);
        effectiveEmrunCall->setReadOnly(true);

        setUpdater([this, target, effectiveEmrunCall, webBrowserAspect] {
            effectiveEmrunCall->setValue(emrunCommand(target,
                                                      buildKey(),
                                                      webBrowserAspect->currentBrowser(),
                                                      "<port>").toUserOutput());
        });

        connect(webBrowserAspect, &BaseAspect::changed, this, &RunConfiguration::update);
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }
};

class EmrunRunWorker : public SimpleTargetRunner
{
public:
    EmrunRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        auto portsGatherer = new PortsGatherer(runControl);
        addStartDependency(portsGatherer);

        setStartModifier([this, runControl, portsGatherer] {
            const QString browserId =
                    runControl->aspect<WebBrowserSelectionAspect>()->currentBrowser;
            setCommandLine(emrunCommand(runControl->target(),
                                        runControl->buildKey(),
                                        browserId,
                                        QString::number(portsGatherer->findEndPoint().port())));
            setEnvironment(runControl->buildEnvironment());
        });
    }
};

// Factories

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

EmrunRunWorkerFactory::EmrunRunWorkerFactory()
{
    setProduct<EmrunRunWorker>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
}

} // Webassembly::Internal
