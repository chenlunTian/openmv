// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace WebAssembly {
namespace Internal {

class WebAssemblyOptionsPage final : public Core::IOptionsPage
{
public:
    WebAssemblyOptionsPage();
};

} // namespace Internal
} // namespace WebAssmbly
