#ifndef CODECOMPLETER_H
#define CODECOMPLETER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include "CompletionItem.h"

class CustomCodeEditor;
class CompletionModel;
class CompletionPopup;
class QKeyEvent;

class CodeCompleter : public QObject {
    Q_OBJECT

public:
    struct WordInfo {
        QString text;
        qint64 startByte = 0;
        qint64 byteLength = 0;
    };

    explicit CodeCompleter(CustomCodeEditor* editor, QObject* parent = nullptr);
    ~CodeCompleter() override;

    void triggerCompletion();
    void updateCompletions(const QString& prefix);
    void hide();

    bool isVisible() const;
    bool handleKeyPress(QKeyEvent* event);

    void setLanguageResource(const QString& resourcePath);
    QString languageResource() const;

    WordInfo wordUnderCursor() const;
    bool checkTriggerCharacter(const QString& insertedChar, const QString& linePrefix);
    QString currentLinePrefix() const;

signals:
    void completionInserted();

private slots:
    void onCompletionAccepted(const QString& text);

private:
    void loadLanguageKeywords();
    void loadSnippets();
    void collectDocumentWords();
    void rebuildItems();
    void showPopup();
    void filterByContext();
    static QString expandSnippet(const QString& snippet);
    static QMap<QString, QString> s_snippetTemplates;

    CustomCodeEditor* m_editor;
    CompletionModel* m_model;
    CompletionPopup* m_popup;
    QString m_languageResource;
    QVector<CompletionItem> m_languageItems;
    QVector<CompletionItem> m_documentItems;
    static constexpr int kMaxDocumentWords = 500;
};

#endif // CODECOMPLETER_H
