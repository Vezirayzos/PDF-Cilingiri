#pragma once

#include <QUndoCommand>
#include <QList>
#include <QString>

class MainWindow;
class PageCard;

class RotateCommand : public QUndoCommand {
public:
    RotateCommand(const QList<PageCard*>& cards, int angleDelta, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    QList<PageCard*> m_cards;
    int m_angleDelta;
};

class FilterCommand : public QUndoCommand {
public:
    FilterCommand(const QList<PageCard*>& cards, const QString& newFilter, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    struct Item {
        PageCard* card;
        QString oldFilter;
    };
    QList<Item> m_items;
    QString m_newFilter;
};

class MoveCardCommand : public QUndoCommand {
public:
    MoveCardCommand(MainWindow* window, int fromIndex, int toIndex, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow* m_window;
    int m_fromIndex;
    int m_toIndex;
};

class DeleteCardsCommand : public QUndoCommand {
public:
    DeleteCardsCommand(MainWindow* window, const QList<PageCard*>& cards, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    struct DeletedCardItem {
        PageCard* card;
        int originalIndex;
    };
    MainWindow* m_window;
    QList<DeletedCardItem> m_deletedItems;
};
