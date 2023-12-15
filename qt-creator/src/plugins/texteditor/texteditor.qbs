import qbs 1.0
import qbs.FileInfo
import qbs.Environment

Project {
    name: "TextEditor"

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "printsupport"] }
        Depends { name: "Aggregation" }
        Depends { name: "Utils" }
        Depends { name: "KSyntaxHighlighting" }

        Export {
            Depends { name: "KSyntaxHighlighting" }
        }

        Depends { name: "Core" }

        cpp.enableExceptions: true

        files: [
            "autocompleter.cpp",
            "autocompleter.h",
            "basefilefind.cpp",
            "basefilefind.h",
            "basehoverhandler.cpp",
            "basehoverhandler.h",
            "behaviorsettings.cpp",
            "behaviorsettings.h",
            "behaviorsettingspage.cpp",
            "behaviorsettingspage.h",
            "behaviorsettingswidget.cpp",
            "behaviorsettingswidget.h",
            "blockrange.h",
            "circularclipboard.cpp",
            "circularclipboard.h",
            "circularclipboardassist.cpp",
            "circularclipboardassist.h",
            "codecchooser.cpp",
            "codecchooser.h",
            "codestyleeditor.cpp",
            "codestyleeditor.h",
            "codestylepool.cpp",
            "codestylepool.h",
            "codestyleselectorwidget.cpp",
            "codestyleselectorwidget.h",
            "colorpreviewhoverhandler.cpp",
            "colorpreviewhoverhandler.h",
            "colorscheme.cpp",
            "colorscheme.h",
            "colorschemeedit.cpp",
            "colorschemeedit.h",
            "command.cpp",
            "command.h",
            "commentssettings.cpp",
            "commentssettings.h",
            "completionsettings.cpp",
            "completionsettings.h",
            "completionsettingspage.cpp",
            "completionsettingspage.h",
            "displaysettings.cpp",
            "displaysettings.h",
            "displaysettingspage.cpp",
            "displaysettingspage.h",
            "extraencodingsettings.cpp",
            "extraencodingsettings.h",
            "findincurrentfile.cpp",
            "findincurrentfile.h",
            "findinfiles.cpp",
            "findinfiles.h",
            "findinopenfiles.cpp",
            "findinopenfiles.h",
            "fontsettings.cpp",
            "fontsettings.h",
            "fontsettingspage.cpp",
            "fontsettingspage.h",
            "formatter.h",
            "formattexteditor.cpp",
            "formattexteditor.h",
            "highlighter.cpp",
            "highlighter.h",
            "highlightersettings.cpp",
            "highlightersettings.h",
            "highlightersettingspage.cpp",
            "highlightersettingspage.h",
            "icodestylepreferences.cpp",
            "icodestylepreferences.h",
            "icodestylepreferencesfactory.cpp",
            "icodestylepreferencesfactory.h",
            "indenter.h",
            "ioutlinewidget.h",
            "linenumberfilter.cpp",
            "linenumberfilter.h",
            "marginsettings.cpp",
            "marginsettings.h",
            "outlinefactory.cpp",
            "outlinefactory.h",
            "plaintexteditorfactory.cpp",
            "plaintexteditorfactory.h",
            "quickfix.cpp",
            "quickfix.h",
            "refactoringchanges.cpp",
            "refactoringchanges.h",
            "refactoroverlay.cpp",
            "refactoroverlay.h",
            "semantichighlighter.cpp",
            "semantichighlighter.h",
            "simplecodestylepreferences.cpp",
            "simplecodestylepreferences.h",
            "simplecodestylepreferenceswidget.cpp",
            "simplecodestylepreferenceswidget.h",
            "storagesettings.cpp",
            "storagesettings.h",
            "syntaxhighlighter.cpp",
            "syntaxhighlighter.h",
            "tabsettings.cpp",
            "tabsettings.h",
            "tabsettingswidget.cpp",
            "tabsettingswidget.h",
            "textdocument.cpp",
            "textdocument.h",
            "textdocumentlayout.cpp",
            "textdocumentlayout.h",
            "texteditor.cpp",
            "texteditor.h",
            "texteditor.qrc",
            "texteditor_global.h", "texteditortr.h",
            "texteditor_p.h",
            "texteditoractionhandler.cpp",
            "texteditoractionhandler.h",
            "texteditorconstants.cpp",
            "texteditorconstants.h",
            "texteditoroverlay.cpp",
            "texteditoroverlay.h",
            "texteditorplugin.cpp",
            "texteditorplugin.h",
            "texteditorsettings.cpp",
            "texteditorsettings.h",
            "textindenter.cpp",
            "textindenter.h",
            "textmark.cpp",
            "textmark.h",
            "textstyles.h",
            "typingsettings.cpp",
            "typingsettings.h",
        ]

        Group {
            name: "CodeAssist"
            prefix: "codeassist/"
            files: [
                "assistenums.h",
                "assistinterface.cpp",
                "assistinterface.h",
                "assistproposalitem.cpp",
                "assistproposalitem.h",
                "assistproposaliteminterface.h",
                "asyncprocessor.cpp",
                "asyncprocessor.h",
                "codeassistant.cpp",
                "codeassistant.h",
                "completionassistprovider.cpp",
                "completionassistprovider.h",
                "documentcontentcompletion.cpp",
                "documentcontentcompletion.h",
                "functionhintproposal.cpp",
                "functionhintproposal.h",
                "functionhintproposalwidget.cpp",
                "functionhintproposalwidget.h",
                "genericproposal.cpp",
                "genericproposal.h",
                "genericproposalmodel.cpp",
                "genericproposalmodel.h",
                "genericproposalwidget.cpp",
                "genericproposalwidget.h",
                "iassistprocessor.cpp",
                "iassistprocessor.h",
                "iassistproposal.cpp",
                "iassistproposal.h",
                "iassistproposalmodel.cpp",
                "iassistproposalmodel.h",
                "iassistproposalwidget.cpp",
                "iassistproposalwidget.h",
                "iassistprovider.cpp",
                "iassistprovider.h",
                "ifunctionhintproposalmodel.cpp",
                "ifunctionhintproposalmodel.h",
                "keywordscompletionassist.cpp",
                "keywordscompletionassist.h",
                "textdocumentmanipulator.cpp",
                "textdocumentmanipulator.h",
                "textdocumentmanipulatorinterface.h",
            ]
        }

        Group {
            name: "Snippets"
            prefix: "snippets/"
            files: [
                "reuse.h",
                "snippet.cpp",
                "snippet.h",
                "snippetassistcollector.cpp",
                "snippetassistcollector.h",
                "snippeteditor.cpp",
                "snippeteditor.h",
                "snippetoverlay.cpp",
                "snippetoverlay.h",
                "snippetparser.cpp",
                "snippetparser.h",
                "snippetprovider.cpp",
                "snippetprovider.h",
                "snippetscollection.cpp",
                "snippetscollection.h",
                "snippetssettings.cpp",
                "snippetssettings.h",
                "snippetssettingspage.cpp",
                "snippetssettingspage.h",
            ]
        }

        Group {
            name: "Tests"
            condition: qtc.testsEnabled
            files: [
                "texteditor_test.cpp",
            ]
        }
    }
}