#include "widgets/completion/CodeCompleter.h"
#include "widgets/completion/CompletionModel.h"
#include "widgets/completion/CompletionPopup.h"
#include "widgets/CustomCodeEditor.h"
#include "QLanguage.hpp"
#include "FileDataBuffer.h"

#include <QKeyEvent>
#include <QFile>
#include <QSet>
#include <QRegularExpression>

static bool isWordCharacter(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

QMap<QString, QString> CodeCompleter::s_snippetTemplates;

CodeCompleter::CodeCompleter(CustomCodeEditor* editor, QObject* parent)
    : QObject(parent)
    , m_editor(editor)
    , m_model(new CompletionModel(this))
    , m_popup(new CompletionPopup(editor))
{
    m_popup->setModel(m_model);

    connect(m_popup, &CompletionPopup::completionAccepted,
            this, &CodeCompleter::onCompletionAccepted);

    if (s_snippetTemplates.isEmpty()) {
        s_snippetTemplates = {
            {"reinterpret_cast", "reinterpret_cast<${1:Type}>(${2:expr})"},
            {"static_cast", "static_cast<${1:Type}>(${2:expr})"},
            {"dynamic_cast", "dynamic_cast<${1:Type}>(${2:expr})"},
            {"const_cast", "const_cast<${1:Type}>(${2:expr})"},
            {"if", "if (${1:condition}) {\n\t${2}\n}"},
            {"if-else", "if (${1:condition}) {\n\t${2}\n} else {\n\t${3}\n}"},
            {"for", "for (${1:int i = 0}; ${2:i < n}; ${3:++i}) {\n\t${4}\n}"},
            {"for-range", "for (${1:const auto&} ${2:item} : ${3:container}) {\n\t${4}\n}"},
            {"while", "while (${1:condition}) {\n\t${2}\n}"},
            {"switch", "switch (${1:value}) {\n\tcase ${2:0}:\n\t\t${3:break;}\n\tdefault:\n\t\t${4:break;}\n}"},
            {"try-catch", "try {\n\t${1}\n} catch (${2:const std::exception&} ${3:e}) {\n\t${4}\n}"},
            {"class", "class ${1:ClassName} {\npublic:\n\t${2:ClassName}() = default;\n\t~${1:ClassName}() = default;\n};"},
            {"struct", "struct ${1:StructName} {\n\t${2}\n};"},
            {"namespace", "namespace ${1:Name} {\n\t${2}\n}"},
            {"enum", "enum class ${1:EnumName} {\n\t${2}\n};"},
            {"function", "${1:void} ${2:functionName}(${3:params}) {\n\t${4}\n}"},
            {"lambda", "[${1}](${2:params}) { ${3} }"},
        };
    }
}

CodeCompleter::~CodeCompleter() = default;

QString CodeCompleter::expandSnippet(const QString& snippet)
{
    QString result = snippet;
    QRegularExpression re(R"(\$\{(\d+):([^}]*)\})");
    QRegularExpressionMatchIterator it = re.globalMatch(result);

    struct Placeholder { int pos; int len; int order; QString defaultVal; };
    QVector<Placeholder> placeholders;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        Placeholder ph;
        ph.pos = match.capturedStart();
        ph.len = match.capturedLength();
        ph.order = match.captured(1).toInt();
        ph.defaultVal = match.captured(2);
        placeholders.append(ph);
    }

    if (placeholders.isEmpty())
        return result;

    std::sort(placeholders.begin(), placeholders.end(),
              [](const Placeholder& a, const Placeholder& b) { return a.pos > b.pos; });

    for (const auto& ph : placeholders) {
        result.replace(ph.pos, ph.len, ph.defaultVal);
    }

    return result;
}

void CodeCompleter::triggerCompletion()
{
    loadLanguageKeywords();
    loadSnippets();
    collectDocumentWords();
    rebuildItems();

    filterByContext();

    WordInfo word = wordUnderCursor();
    QString prefix = word.text;
    QString linePrefix = currentLinePrefix().trimmed();

    // In preprocessor context (but NOT header context), prepend # for matching
    bool isHeaderContext = linePrefix.startsWith(QStringLiteral("#include"))
                           || linePrefix.startsWith(QStringLiteral("# include"));
    if (!isHeaderContext && linePrefix.startsWith(QStringLiteral("#"))
        && !prefix.startsWith(QStringLiteral("#")))
        prefix = QStringLiteral("#") + prefix;

    m_model->setFilterPrefix(prefix);

    if (m_model->filteredCount() == 0) {
        hide();
        return;
    }

    showPopup();
    m_popup->setSelectedIndex(0);
}

void CodeCompleter::updateCompletions(const QString& prefix)
{
    loadLanguageKeywords();
    loadSnippets();
    collectDocumentWords();
    rebuildItems();

    filterByContext();

    QString adjustedPrefix = prefix;
    QString linePrefix = currentLinePrefix().trimmed();

    // In preprocessor context (but NOT header context), prepend # for matching
    bool isHeaderContext = linePrefix.startsWith(QStringLiteral("#include"))
                           || linePrefix.startsWith(QStringLiteral("# include"));
    if (!isHeaderContext && linePrefix.startsWith(QStringLiteral("#"))
        && !adjustedPrefix.startsWith(QStringLiteral("#")))
        adjustedPrefix = QStringLiteral("#") + adjustedPrefix;
    m_model->setFilterPrefix(adjustedPrefix);

    if (m_model->filteredCount() == 0) {
        hide();
        return;
    }

    showPopup();
}

void CodeCompleter::hide()
{
    m_popup->hide();
}

bool CodeCompleter::isVisible() const
{
    return m_popup->isVisible();
}

bool CodeCompleter::handleKeyPress(QKeyEvent* event)
{
    if (!m_popup->isVisible())
        return false;

    switch (event->key()) {
    case Qt::Key_Escape:
        hide();
        event->accept();
        return true;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_popup->hasSelection()) {
            onCompletionAccepted(m_model->itemAt(m_popup->selectedIndex()).text);
            event->accept();
            return true;
        }
        hide();
        return false;

    case Qt::Key_Tab:
        if (m_popup->hasSelection()) {
            onCompletionAccepted(m_model->itemAt(m_popup->selectedIndex()).text);
            event->accept();
            return true;
        }
        hide();
        return false;

    case Qt::Key_Up:
        m_popup->selectPrevious();
        event->accept();
        return true;

    case Qt::Key_Down:
        m_popup->selectNext();
        event->accept();
        return true;

    default:
        break;
    }

    return false;
}

void CodeCompleter::setLanguageResource(const QString& resourcePath)
{
    if (m_languageResource == resourcePath)
        return;
    m_languageResource = resourcePath;
    m_languageItems.clear();
}

QString CodeCompleter::languageResource() const
{
    return m_languageResource;
}

CodeCompleter::WordInfo CodeCompleter::wordUnderCursor() const
{
    if (!m_editor || !m_editor->getBuffer())
        return {};

    const qint64 cursorPos = m_editor->cursorPosition();
    if (cursorPos <= 0)
        return {};

    QByteArray bufferData = m_editor->getBuffer()->read(0, m_editor->getBuffer()->size());
    if (bufferData.isEmpty())
        return {};

    QString text = QString::fromUtf8(bufferData);

    int charPos = 0;
    {
        qint64 byteOffset = 0;
        for (int i = 0; i < text.size(); ++i) {
            int charBytes = QString(text[i]).toUtf8().size();
            if (byteOffset + charBytes > cursorPos) {
                charPos = i;
                break;
            }
            byteOffset += charBytes;
            if (i == text.size() - 1)
                charPos = text.size();
        }
    }

    charPos = qBound(0, charPos, text.size());

    int start = charPos;
    while (start > 0 && isWordCharacter(text.at(start - 1)))
        --start;

    int end = charPos;
    while (end < text.size() && isWordCharacter(text.at(end)))
        ++end;

    if (end <= start)
        return {};

    QString word = text.mid(start, end - start);

    qint64 byteStart = 0;
    for (int i = 0; i < start; ++i)
        byteStart += QString(text[i]).toUtf8().size();

    qint64 byteEnd = byteStart;
    for (int i = start; i < end; ++i)
        byteEnd += QString(text[i]).toUtf8().size();

    return { word, byteStart, byteEnd - byteStart };
}

QString CodeCompleter::currentLinePrefix() const
{
    if (!m_editor || !m_editor->getBuffer())
        return {};

    const qint64 cursorPos = m_editor->cursorPosition();
    if (cursorPos <= 0)
        return {};

    QByteArray bufferData = m_editor->getBuffer()->read(0, m_editor->getBuffer()->size());
    if (bufferData.isEmpty())
        return {};

    QString text = QString::fromUtf8(bufferData);

    int charPos = 0;
    {
        qint64 byteOffset = 0;
        for (int i = 0; i < text.size(); ++i) {
            int charBytes = QString(text[i]).toUtf8().size();
            if (byteOffset + charBytes > cursorPos) {
                charPos = i;
                break;
            }
            byteOffset += charBytes;
            if (i == text.size() - 1)
                charPos = text.size();
        }
    }

    int lineStart = charPos;
    while (lineStart > 0 && text.at(lineStart - 1) != QLatin1Char('\n'))
        --lineStart;

    return text.mid(lineStart, charPos - lineStart);
}

bool CodeCompleter::checkTriggerCharacter(const QString& insertedChar, const QString& linePrefix)
{
    if (insertedChar == QStringLiteral("#")) {
        triggerCompletion();
        return true;
    }

    QString trimmed = linePrefix.trimmed();
    if ((insertedChar == QStringLiteral("<") || insertedChar == QStringLiteral("\""))
        && (trimmed.startsWith(QStringLiteral("#include"))
            || trimmed.startsWith(QStringLiteral("# include")))) {
        triggerCompletion();
        return true;
    }

    if (insertedChar == QStringLiteral(":") && linePrefix.endsWith(QStringLiteral("std:"))) {
        triggerCompletion();
        return true;
    }

    return false;
}

void CodeCompleter::loadLanguageKeywords()
{
    if (!m_languageItems.isEmpty())
        return;

    if (m_languageResource.isEmpty())
        return;

    QFile file(m_languageResource);
    if (!file.open(QIODevice::ReadOnly))
        return;

    Q_INIT_RESOURCE(codeeditor_res);

    QLanguage language(&file);
    if (!language.isLoaded())
        return;

    auto keys = language.keys();
    for (const QString& key : keys) {
        auto names = language.names(key);
        int priority;
        QString category;

        if (key == QStringLiteral("Keyword")) {
            priority = 0;
            category = QStringLiteral("Keyword");
        } else if (key == QStringLiteral("Preprocessor")) {
            priority = 0;
            category = QStringLiteral("Preprocessor");
        } else if (key == QStringLiteral("PrimitiveType")) {
            priority = 1;
            category = QStringLiteral("Type");
        } else if (key == QStringLiteral("StdLibrary")) {
            priority = 2;
            category = QStringLiteral("Std");
        } else if (key == QStringLiteral("HeaderFile")) {
            priority = 2;
            category = QStringLiteral("Header");
        } else if (key == QStringLiteral("Snippet")) {
            priority = 0;
            category = QStringLiteral("Snippet");
        } else {
            priority = 3;
            category = key;
        }

        for (const QString& name : names) {
            m_languageItems.append({ name, "", "", category, priority });
        }
    }
}

void CodeCompleter::loadSnippets()
{
    // Snippets are already loaded as language items with category "Snippet".
    // Now attach the snippet template text to matching items.
    for (auto& item : m_languageItems) {
        if (item.category == QStringLiteral("Snippet")) {
            auto it = s_snippetTemplates.find(item.text);
            if (it != s_snippetTemplates.end())
                item.snippet = it.value();
        }
    }
}

void CodeCompleter::collectDocumentWords()
{
    m_documentItems.clear();

    if (!m_editor || !m_editor->getBuffer())
        return;

    const qint64 bufSize = m_editor->getBuffer()->size();
    if (bufSize <= 0)
        return;

    const qint64 maxRead = qMin(bufSize, qint64(256 * 1024));
    QByteArray bufferData = m_editor->getBuffer()->read(0, maxRead);
    if (bufferData.isEmpty())
        return;

    QString text = QString::fromUtf8(bufferData);
    QSet<QString> seen;

    WordInfo cursorWord = wordUnderCursor();
    if (!cursorWord.text.isEmpty()) {
        seen.insert(cursorWord.text);
        m_documentItems.append({ cursorWord.text, "", "", QStringLiteral("Local"), 1 });
    }

    // Simple type keywords for variable detection
    static const QSet<QString> typeKeywords = {
        "int", "long", "short", "char", "float", "double", "bool", "void",
        "unsigned", "signed", "auto", "const", "static", "volatile",
        "size_t", "ssize_t", "intptr_t", "uintptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "string", "vector", "map", "set", "list", "deque", "array",
        "optional", "variant", "any", "pair", "tuple", "span",
        "unique_ptr", "shared_ptr", "weak_ptr",
        "QString", "QByteArray", "QList", "QMap", "QSet", "QVector",
        "QObject", "QWidget", "QStringList",
    };

    int i = 0;
    while (i < text.size() && m_documentItems.size() < kMaxDocumentWords) {
        if (isWordCharacter(text.at(i))) {
            int start = i;
            while (i < text.size() && isWordCharacter(text.at(i)))
                ++i;
            int len = i - start;
            if (len >= 2) {
                QString word = text.mid(start, len);
                if (!seen.contains(word)) {
                    seen.insert(word);

                    // Try to detect context
                    QString detail;

                    // Check if followed by ( → function
                    int nextNonSpace = i;
                    while (nextNonSpace < text.size() && text.at(nextNonSpace).isSpace())
                        ++nextNonSpace;
                    if (nextNonSpace < text.size() && text.at(nextNonSpace) == QLatin1Char('(')) {
                        // Look backward for return type
                        int typeEnd = start;
                        while (typeEnd > 0 && text.at(typeEnd - 1).isSpace())
                            --typeEnd;
                        int typeStart = typeEnd;
                        while (typeStart > 0 && isWordCharacter(text.at(typeStart - 1)))
                            --typeStart;
                        if (typeStart < typeEnd) {
                            QString retType = text.mid(typeStart, typeEnd - typeStart);
                            detail = retType + " " + word + "()";
                        } else {
                            detail = word + "()";
                        }
                        m_documentItems.append({ word, "", detail, QStringLiteral("Function"), 2 });
                    }
                    // Check if preceded by a type keyword → variable
                    else {
                        int prevEnd = start;
                        while (prevEnd > 0 && text.at(prevEnd - 1).isSpace())
                            --prevEnd;
                        int prevStart = prevEnd;
                        while (prevStart > 0 && isWordCharacter(text.at(prevStart - 1)))
                            --prevStart;
                        if (prevStart < prevEnd) {
                            QString prevWord = text.mid(prevStart, prevEnd - prevStart);
                            if (typeKeywords.contains(prevWord)) {
                                detail = prevWord + " " + word;
                                m_documentItems.append({ word, "", detail, QStringLiteral("Variable"), 2 });
                            } else {
                                m_documentItems.append({ word, "", "", QStringLiteral("Document"), 3 });
                            }
                        } else {
                            m_documentItems.append({ word, "", "", QStringLiteral("Document"), 3 });
                        }
                    }
                }
            }
        } else {
            ++i;
        }
    }
}

void CodeCompleter::rebuildItems()
{
    QVector<CompletionItem> all;
    all.reserve(m_languageItems.size() + m_documentItems.size());
    all.append(m_languageItems);
    all.append(m_documentItems);
    m_model->setItems(all);
}

void CodeCompleter::filterByContext()
{
    QString linePrefix = currentLinePrefix().trimmed();

    if (linePrefix.startsWith(QStringLiteral("#include"))
        || linePrefix.startsWith(QStringLiteral("# include"))) {
        m_model->setCategoryFilter(QStringLiteral("Header"));
        return;
    }

    if (linePrefix.startsWith(QStringLiteral("#"))) {
        m_model->setCategoryFilter(QStringLiteral("Preprocessor"));
        return;
    }

    m_model->setCategoryFilter(QString());
}

void CodeCompleter::showPopup()
{
    WordInfo word = wordUnderCursor();
    QPoint cursorPoint;

    if (!word.text.isEmpty()) {
        cursorPoint = m_editor->contentPointForBytePos(word.startByte);
    } else {
        cursorPoint = m_editor->contentPointForBytePos(m_editor->cursorPosition());
    }

    int lineHeight = static_cast<int>(m_editor->fontMetrics().height());
    m_popup->showNearPoint(cursorPoint, lineHeight);
}

void CodeCompleter::onCompletionAccepted(const QString& text)
{
    WordInfo word = wordUnderCursor();

    // Find the CompletionItem to check for snippet — prefer items with snippets
    QString insertText = text;
    QString snippet;
    for (const auto& item : m_languageItems) {
        if (item.text == text && !item.snippet.isEmpty()) {
            snippet = item.snippet;
            break;
        }
    }
    if (snippet.isEmpty()) {
        for (const auto& item : m_languageItems) {
            if (item.text == text) {
                snippet = item.snippet;
                break;
            }
        }
    }

    if (!snippet.isEmpty()) {
        insertText = expandSnippet(snippet);
    }

    // Check context: header insertion after #include <
    QString linePrefix = currentLinePrefix().trimmed();
    bool isHeaderContext = linePrefix.startsWith(QStringLiteral("#include"))
                           || linePrefix.startsWith(QStringLiteral("# include"));
    if (isHeaderContext) {
        insertText = text + QStringLiteral(">");
    }

    // If replacement text starts with # and there's a # just before the
    // replaced range, strip the leading # to avoid ## duplication
    if (insertText.startsWith(QStringLiteral("#")) && !isHeaderContext) {
        qint64 checkPos = (word.byteLength > 0) ? word.startByte : m_editor->cursorPosition();
        if (checkPos > 0) {
            QByteArray buf = m_editor->getBuffer()->read(checkPos - 1, 1);
            if (buf == "#") {
                insertText = insertText.mid(1);
            }
        }
    }

    QByteArray replacement = insertText.toUtf8();

    if (word.byteLength > 0) {
        m_editor->replaceRange(word.startByte, word.byteLength, replacement);
    } else {
        m_editor->insertText(insertText);
    }

    hide();
    emit completionInserted();
}
