// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "portlist.h"

#include <QPair>
#include <QString>

#include <cctype>

namespace Utils {
namespace Internal {
namespace {

using Range = QPair<Port, Port>;

class PortsSpecParser
{
    struct ParseException {
        ParseException(const char *error) : error(error) {}
        const char * const error;
    };

public:
    PortsSpecParser(const QString &portsSpec)
        : m_pos(0), m_portsSpec(portsSpec) { }

    /*
     * Grammar: Spec -> [ ElemList ]
     *          ElemList -> Elem [ ',' ElemList ]
     *          Elem -> Port [ '-' Port ]
     */
    PortList parse()
    {
        try {
            if (!atEnd())
                parseElemList();
        } catch (const ParseException &e) {
            qWarning("Malformed ports specification: %s", e.error);
        }
        return m_portList;
    }

private:
    void parseElemList()
    {
        if (atEnd())
            throw ParseException("Element list empty.");
        parseElem();
        if (atEnd())
            return;
        if (nextChar() != ',') {
            throw ParseException("Element followed by something else "
                "than a comma.");
        }
        ++m_pos;
        parseElemList();
    }

    void parseElem()
    {
        const Port startPort = parsePort();
        if (atEnd() || nextChar() != '-') {
            m_portList.addPort(startPort);
            return;
        }
        ++m_pos;
        const Port endPort = parsePort();
        if (endPort < startPort)
            throw ParseException("Invalid range (end < start).");
        m_portList.addRange(startPort, endPort);
    }

    Port parsePort()
    {
        if (atEnd())
            throw ParseException("Empty port string.");
        int port = 0;
        do {
            const char next = nextChar();
            if (!std::isdigit(next))
                break;
            port = 10*port + next - '0';
            ++m_pos;
        } while (!atEnd());
        if (port == 0 || port >= 2 << 16)
            throw ParseException("Invalid port value.");
        return Port(port);
    }

    bool atEnd() const { return m_pos == m_portsSpec.length(); }
    char nextChar() const { return m_portsSpec.at(m_pos).toLatin1(); }

    PortList m_portList;
    int m_pos;
    const QString &m_portsSpec;
};

} // anonymous namespace

class PortListPrivate
{
public:
    QList<Range> ranges;
};

} // namespace Internal

PortList::PortList() : d(new Internal::PortListPrivate)
{
}

PortList::PortList(const PortList &other) : d(new Internal::PortListPrivate(*other.d))
{
}

PortList::~PortList()
{
    delete d;
}

PortList &PortList::operator=(const PortList &other)
{
    *d = *other.d;
    return *this;
}

PortList PortList::fromString(const QString &portsSpec)
{
    return Internal::PortsSpecParser(portsSpec).parse();
}

void PortList::addPort(Port port) { addRange(port, port); }

void PortList::addRange(Port startPort, Port endPort)
{
    d->ranges << Internal::Range(startPort, endPort);
}

bool PortList::hasMore() const { return !d->ranges.isEmpty(); }

bool PortList::contains(Port port) const
{
    for (const Internal::Range &r : std::as_const(d->ranges)) {
        if (port >= r.first && port <= r.second)
            return true;
    }
    return false;
}

int PortList::count() const
{
    int n = 0;
    for (const Internal::Range &r : std::as_const(d->ranges))
        n += r.second.number() - r.first.number() + 1;
    return n;
}

Port PortList::getNext()
{
    Q_ASSERT(!d->ranges.isEmpty());

    Internal::Range &firstRange = d->ranges.first();
    const Port next = firstRange.first;
    firstRange.first = Port(firstRange.first.number() + 1);
    if (firstRange.first > firstRange.second)
        d->ranges.removeFirst();
    return next;
}

Port PortList::getNextFreePort(const QList<Port> &usedPorts)
{
    while (hasMore()) {
        const Port port = getNext();
        if (!usedPorts.contains(port))
            return port;
    }
    return {};
}

QString PortList::toString() const
{
    QString stringRep;
    for (const Internal::Range &range : std::as_const(d->ranges)) {
        stringRep += QString::number(range.first.number());
        if (range.second != range.first)
            stringRep += QLatin1Char('-') + QString::number(range.second.number());
        stringRep += QLatin1Char(',');
    }
    if (!stringRep.isEmpty())
        stringRep.remove(stringRep.length() - 1, 1); // Trailing comma.
    return stringRep;
}

QString PortList::regularExpression()
{
    const QLatin1String portExpr("(\\d)+");
    const QString listElemExpr = QString::fromLatin1("%1(-%1)?").arg(portExpr);
    return QString::fromLatin1("((%1)(,%1)*)?").arg(listElemExpr);
}

} // namespace Utils
