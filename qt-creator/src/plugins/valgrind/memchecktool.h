// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Valgrind::Internal {

class MemcheckToolRunner;

class MemcheckTool final
{
public:
    MemcheckTool();
    ~MemcheckTool();
};

} // Valgrind::Internal