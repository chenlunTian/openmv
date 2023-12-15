
#line 126 "qmljs.g"
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qmljs.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qmljs.g
//

#ifndef QMLJSPARSER_P_H
#define QMLJSPARSER_P_H

#include "qmljs/parser/qmljsglobal_p.h"
#include "qmljs/parser/qmljsgrammar_p.h"
#include "qmljs/parser/qmljsast_p.h"
#include "qmljs/parser/qmljsengine_p.h"
#include "qmljs/parser/qmljsdiagnosticmessage_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Engine;

class QML_PARSER_EXPORT Parser: protected QmlJSGrammar
{
public:
    union Value {
      int ival;
      double dval;
      AST::VariableScope scope;
      AST::ForEachType forEachType;
      AST::ArgumentList *ArgumentList;
      AST::CaseBlock *CaseBlock;
      AST::CaseClause *CaseClause;
      AST::CaseClauses *CaseClauses;
      AST::Catch *Catch;
      AST::DefaultClause *DefaultClause;
      AST::Elision *Elision;
      AST::ExpressionNode *Expression;
      AST::TemplateLiteral *Template;
      AST::Finally *Finally;
      AST::FormalParameterList *FormalParameterList;
      AST::FunctionDeclaration *FunctionDeclaration;
      AST::Node *Node;
      AST::PropertyName *PropertyName;
      AST::Statement *Statement;
      AST::StatementList *StatementList;
      AST::Block *Block;
      AST::VariableDeclarationList *VariableDeclarationList;
      AST::Pattern *Pattern;
      AST::PatternElement *PatternElement;
      AST::PatternElementList *PatternElementList;
      AST::PatternProperty *PatternProperty;
      AST::PatternPropertyList *PatternPropertyList;
      AST::ClassElementList *ClassElementList;
      AST::ImportClause *ImportClause;
      AST::FromClause *FromClause;
      AST::NameSpaceImport *NameSpaceImport;
      AST::ImportsList *ImportsList;
      AST::NamedImports *NamedImports;
      AST::ImportSpecifier *ImportSpecifier;
      AST::ExportSpecifier *ExportSpecifier;
      AST::ExportsList *ExportsList;
      AST::ExportClause *ExportClause;
      AST::ExportDeclaration *ExportDeclaration;
      AST::TypeAnnotation *TypeAnnotation;
      AST::TypeArgument *TypeArgument;
      AST::Type *Type;

      AST::UiProgram *UiProgram;
      AST::UiHeaderItemList *UiHeaderItemList;
      AST::UiPragma *UiPragma;
      AST::UiImport *UiImport;
      AST::UiParameterList *UiParameterList;
      AST::UiPropertyAttributes *UiPropertyAttributes;
      AST::UiPublicMember *UiPublicMember;
      AST::UiObjectDefinition *UiObjectDefinition;
      AST::UiObjectInitializer *UiObjectInitializer;
      AST::UiObjectBinding *UiObjectBinding;
      AST::UiScriptBinding *UiScriptBinding;
      AST::UiArrayBinding *UiArrayBinding;
      AST::UiObjectMember *UiObjectMember;
      AST::UiObjectMemberList *UiObjectMemberList;
      AST::UiArrayMemberList *UiArrayMemberList;
      AST::UiQualifiedId *UiQualifiedId;
      AST::UiEnumMemberList *UiEnumMemberList;
      AST::UiVersionSpecifier *UiVersionSpecifier;
      AST::UiAnnotation *UiAnnotation;
      AST::UiAnnotationList *UiAnnotationList;
    };

public:
    Parser(Engine *engine);
    ~Parser();

    // parse a UI program
    bool parse() { ++functionNestingLevel; bool r = parse(T_FEED_UI_PROGRAM); --functionNestingLevel; return r; }
    bool parseStatement() { return parse(T_FEED_JS_STATEMENT); }
    bool parseExpression() { return parse(T_FEED_JS_EXPRESSION); }
    bool parseUiObjectMember() { ++functionNestingLevel; bool r = parse(T_FEED_UI_OBJECT_MEMBER); --functionNestingLevel; return r; }
    bool parseProgram() { return parse(T_FEED_JS_SCRIPT); }
    bool parseScript() { return parse(T_FEED_JS_SCRIPT); }
    bool parseModule() { return parse(T_FEED_JS_MODULE); }

    AST::UiProgram *ast() const
    { return AST::cast<AST::UiProgram *>(program); }

    AST::Statement *statement() const
    {
        if (! program)
            return 0;

        return program->statementCast();
    }

    AST::ExpressionNode *expression() const
    {
        if (! program)
            return 0;

        return program->expressionCast();
    }

    AST::UiObjectMember *uiObjectMember() const
    {
        if (! program)
            return 0;

        return program->uiObjectMemberCast();
    }

    AST::Node *rootNode() const
    { return program; }

    QList<DiagnosticMessage> diagnosticMessages() const
    { return diagnostic_messages; }

    inline DiagnosticMessage diagnosticMessage() const
    {
        for (const DiagnosticMessage &d : diagnostic_messages) {
            if (d.isWarning())
                return d;
        }

        return DiagnosticMessage();
    }

    inline QString errorMessage() const
    { return diagnosticMessage().message; }

    inline int errorLineNumber() const
    { return diagnosticMessage().loc.startLine; }

    inline int errorColumnNumber() const
    { return diagnosticMessage().loc.startColumn; }

protected:
    bool parse(int startToken);

    void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    inline QStringView &stringRef(int index)
    { return string_stack [tos + index - 1]; }

    inline QStringView &rawStringRef(int index)
    { return rawString_stack [tos + index - 1]; }

    inline SourceLocation &loc(int index)
    { return location_stack [tos + index - 1]; }

    AST::UiQualifiedId *reparseAsQualifiedId(AST::ExpressionNode *expr);

    void pushToken(int token);
    int lookaheadToken(Lexer *lexer);

    static DiagnosticMessage compileError(const SourceLocation &location,
                                          const QString &message, QtMsgType kind = QtCriticalMsg)
    {
        DiagnosticMessage error;
        error.loc = location;
        error.message = message;
        error.kind = DiagnosticMessage::qtMsgTypeToKind(kind);
        return error;
    }

    void syntaxError(const SourceLocation &location, const char *message) {
        diagnostic_messages.append(compileError(location, QLatin1String(message)));
     }
     void syntaxError(const SourceLocation &location, const QString &message) {
        diagnostic_messages.append(compileError(location, message));
      }

    bool ensureNoFunctionTypeAnnotations(AST::TypeAnnotation *returnTypeAnnotation, AST::FormalParameterList *formals);

protected:
    Engine *driver;
    MemoryPool *pool;
    int tos = 0;
    int stack_size = 0;
    Value *sym_stack = nullptr;
    int *state_stack = nullptr;
    SourceLocation *location_stack = nullptr;
    std::vector<QStringView> string_stack;
    std::vector<QStringView> rawString_stack;

    AST::Node *program = nullptr;

    // error recovery and lookahead handling
    enum { TOKEN_BUFFER_SIZE = 5 };

    struct SavedToken {
       int token;
       double dval;
       SourceLocation loc;
       QStringView spell;
       QStringView raw;
    };

    int yytoken = -1;
    double yylval = 0.;
    QStringView yytokenspell;
    QStringView yytokenraw;
    SourceLocation yylloc;
    SourceLocation yyprevlloc;

    SavedToken token_buffer[TOKEN_BUFFER_SIZE];
    SavedToken *first_token = nullptr;
    SavedToken *last_token = nullptr;

    int functionNestingLevel = 0;
    int classNestingLevel = 0;

    enum CoverExpressionType {
        CE_Invalid,
        CE_ParenthesizedExpression,
        CE_FormalParameterList
    };
    SourceLocation coverExpressionErrorLocation;
    CoverExpressionType coverExpressionType = CE_Invalid;

    QList<DiagnosticMessage> diagnostic_messages;
};

} // end of namespace QmlJS

#line 1769 "qmljs.g"

#define J_SCRIPT_REGEXPLITERAL_RULE1 163

#line 1781 "qmljs.g"

#define J_SCRIPT_REGEXPLITERAL_RULE2 164

#line 3401 "qmljs.g"

#define J_SCRIPT_EXPRESSIONSTATEMENTLOOKAHEAD_RULE 465

#line 4053 "qmljs.g"

#define J_SCRIPT_CONCISEBODYLOOKAHEAD_RULE 535

#line 4594 "qmljs.g"

#define J_SCRIPT_EXPORTDECLARATIONLOOKAHEAD_RULE 603

#line 4878 "qmljs.g"

QT_QML_END_NAMESPACE



#endif // QMLJSPARSER_P_H
