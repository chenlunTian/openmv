// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestframework.h"

#include "qttestconstants.h"
#include "qttestparser.h"
#include "qttesttreeitem.h"
#include "../autotesttr.h"

namespace Autotest {
namespace Internal {

ITestParser *QtTestFramework::createTestParser()
{
    return new QtTestParser(this);
}

ITestTreeItem *QtTestFramework::createRootNode()
{
    return new QtTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

const char *QtTestFramework::name() const
{
    return QtTest::Constants::FRAMEWORK_NAME;
}

QString QtTestFramework::displayName() const
{
    return Tr::tr(QtTest::Constants::FRAMEWORK_SETTINGS_CATEGORY);
}

unsigned QtTestFramework::priority() const
{
    return QtTest::Constants::FRAMEWORK_PRIORITY;
}

QStringList QtTestFramework::testNameForSymbolName(const QString &symbolName) const
{
    int index = symbolName.lastIndexOf("::");
    if (index == -1)
        return {};
    return { symbolName.left(index), symbolName.mid(index + 2) };
}

} // namespace Internal
} // namespace Autotest
