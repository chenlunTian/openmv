// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidslideindexexception.h"

/*!
\class QmlDesigner::InvalidSlideIndexException
\ingroup CoreExceptions
\brief The InvalidSlideIndexException class provides an exception for an invalid
index for a slide.

\see ModelNode
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidSlideIndexException::InvalidSlideIndexException(int line,
                                                       const QByteArray &function,
                                                       const QByteArray &file)
  : Exception(line, function, file)
{
    createWarning();
}

/*!
    Returns the type of the exception as a string.
*/
QString InvalidSlideIndexException::type() const
{
    return QLatin1String("InvalidSlideIndexException");
}

} // namespace QmlDesigner
