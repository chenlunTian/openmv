// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "Symbol.h"
#include "Names.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Scope : public Symbol
{
public:
    Scope(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Scope(Clone *clone, Subst *subst, Scope *original);
    virtual ~Scope();

    /// Adds a Symbol to this Scope.
    void addMember(Symbol *symbol);

    /// Returns true if this Scope is empty; otherwise returns false.
    bool isEmpty() const;

    /// Returns the number of symbols is in the scope.
    int memberCount() const;

    /// Returns the Symbol at the given position.
    Symbol *memberAt(int index) const;

    typedef Symbol **iterator;

    /// Returns member iterator to the beginning.
    iterator memberBegin() const;

    /// Returns member iterator to the end.
    iterator memberEnd() const;

    Symbol *find(const Identifier *id) const;
    Symbol *find(OperatorNameId::Kind operatorId) const;
    Symbol *find(const ConversionNameId *conv) const;

    /// Set the start offset of the scope
    int startOffset() const { return _startOffset; }
    /// Set the start offset of the scope
    void setStartOffset(int offset) { _startOffset = offset; }

    /// Set the end offset of the scope
    int endOffset() const { return _endOffset; }
    /// Set the end offset of the scope
    void setEndOffset(int offset) { _endOffset = offset; }

    const Scope *asScope() const override { return this; }
    Scope *asScope() override { return this; }

private:
    SymbolTable *_members;
    int _startOffset;
    int _endOffset;
};

} // namespace CPlusPlus
