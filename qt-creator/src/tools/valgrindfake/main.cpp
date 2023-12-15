// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <QDir>
#include <QTcpSocket>
#include <QXmlStreamReader>
#include <QProcessEnvironment>

#include "outputgenerator.h"

using namespace Valgrind::Fake;

QTextStream qerr(stderr);
QTextStream qout(stdout);

void usage(QTextStream& stream)
{
    stream << "valgrind-fake OPTIONS" << '\n';
    stream << '\n';
    stream << " REQUIRED OPTIONS:" << '\n';
    stream << "  --xml-socket=ipaddr:port \tXML output to socket ipaddr:port" << '\n';
    stream << "  -i, --xml-input FILE     \tpath to a XML file as generated by valgrind" << '\n';
    stream << '\n';
    stream << " OPTIONAL OPTIONS:" << '\n';
    stream << "  -c, --crash              \tcrash randomly" << '\n';
    stream << "  -g, --garbage            \toutput invalid XML somewhere" << '\n';
    stream << "  -w, --wait SECONDS       \twait randomly for the given amount of seconds" << '\n';
    stream << "  -h, --help               \tprint help" << '\n';
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments();
    QString arg_port;
    QString arg_server;
    QString arg_xmlFile;
    bool arg_crash = false;
    bool arg_garbage = false;
    uint arg_wait = 0;

    const QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    arg_xmlFile = sysEnv.value(QLatin1String("QCIT_INPUT_FILE"));

    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (arg.startsWith(QLatin1String("--xml-socket="))) {
            arg_server = arg.mid(13, arg.indexOf(QLatin1Char(':')) - 13);
            arg_port = arg.mid(13 + arg_server.length() + 1);
        } else if (args.size() > i + 1
                    && (args.at(i) == QLatin1String("-i")
                        || args.at(i) == QLatin1String("--xml-input"))) {
            arg_xmlFile = args.at(i+1);
            ++i;
        } else if (arg == QLatin1String("-c") || arg == QLatin1String("--crash")) {
            arg_crash = true;
        } else if (arg == QLatin1String("-g") || arg == QLatin1String("--garbage")) {
            arg_garbage = true;
        } else if (args.size() > i + 1 && (arg == QLatin1String("-w") || arg == QLatin1String("--wait"))) {
            bool ok;
            arg_wait = args.at(i+1).toUInt(&ok);
            if (!ok) {
                qerr << "ERROR: invalid wait time given" << args.at(i+1) << '\n';
                usage(qerr);
                return 4;
            }
        } else if (args.at(i) == QLatin1String("--help") || args.at(i) == QLatin1String("-h")) {
            usage(qout);
            return 0;
        }
    }

    if (arg_xmlFile.isEmpty()) {
        qerr << "ERROR: no XML input file given" << '\n';
        usage(qerr);
        return 1;
    }
    if (arg_server.isEmpty()) {
        qerr << "ERROR: no server given" << '\n';
        usage(qerr);
        return 2;
    }
    if (arg_port.isEmpty()) {
        qerr << "ERROR: no port given" << '\n';
        usage(qerr);
        return 3;
    }

    QFile xmlFile(arg_xmlFile);
    if (!xmlFile.exists() || !xmlFile.open(QIODevice::ReadOnly)) {
        qerr << "ERROR: invalid XML file" << '\n';
        usage(qerr);
        return 10;
    }
    bool ok = false;
    quint16 port = arg_port.toUInt(&ok);
    if (!ok) {
        qerr << "ERROR: invalid port" << '\n';
        usage(qerr);
        return 30;
    }

    QTcpSocket socket;
    socket.connectToHost(arg_server, port, QIODevice::WriteOnly);
    if (!socket.isOpen()) {
        qerr << "ERROR: could not open socket to server:" << arg_server << ":" << port << '\n';
        usage(qerr);
        return 20;
    }
    if (!socket.waitForConnected()) {
        qerr << "ERROR: could not connect to socket: " << socket.errorString() << '\n';
        return 21;
    }

    OutputGenerator generator(&socket, &xmlFile);
    QObject::connect(&generator, &OutputGenerator::finished, &app, &QCoreApplication::quit);
    generator.setCrashRandomly(arg_crash);
    generator.setOutputGarbage(arg_garbage);
    generator.setWait(arg_wait);

    return app.exec();
}
