#ifndef COMPLETIONITEM_H
#define COMPLETIONITEM_H

#include <QString>

struct CompletionItem {
    QString text;
    QString snippet;
    QString detail;
    QString category;
    int priority = 0;
};

#endif // COMPLETIONITEM_H
