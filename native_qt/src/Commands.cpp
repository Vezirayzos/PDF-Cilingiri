#include "Commands.h"
#include "PageCard.h"
#include "MainWindow.h"

// ==========================================
// RotateCommand
// ==========================================
RotateCommand::RotateCommand(const QList<PageCard*>& cards, int angleDelta, QUndoCommand* parent)
    : QUndoCommand(parent), m_cards(cards), m_angleDelta(angleDelta) {
    setText(QString("%1 Sayfayı Döndür (%2°)").arg(cards.size()).arg(angleDelta));
}

void RotateCommand::undo() {
    for (auto* card : m_cards) {
        card->setRotation(card->getRotation() - m_angleDelta);
    }
}

void RotateCommand::redo() {
    for (auto* card : m_cards) {
        card->setRotation(card->getRotation() + m_angleDelta);
    }
}

// ==========================================
// FilterCommand
// ==========================================
FilterCommand::FilterCommand(const QList<PageCard*>& cards, const QString& newFilter, QUndoCommand* parent)
    : QUndoCommand(parent), m_newFilter(newFilter) {
    for (auto* card : cards) {
        m_items.append({card, card->getFilter()});
    }
    setText(QString("%1 Sayfaya Filtre Uygula: %2").arg(cards.size()).arg(newFilter));
}

void FilterCommand::undo() {
    for (const auto& item : m_items) {
        item.card->setFilter(item.oldFilter);
    }
}

void FilterCommand::redo() {
    for (const auto& item : m_items) {
        item.card->setFilter(m_newFilter);
    }
}

// ==========================================
// MoveCardCommand
// ==========================================
MoveCardCommand::MoveCardCommand(MainWindow* window, int fromIndex, int toIndex, QUndoCommand* parent)
    : QUndoCommand(parent), m_window(window), m_fromIndex(fromIndex), m_toIndex(toIndex) {
    setText("Sayfa Sırasını Değiştir");
}

void MoveCardCommand::undo() {
    m_window->internalMoveCard(m_toIndex, m_fromIndex);
}

void MoveCardCommand::redo() {
    m_window->internalMoveCard(m_fromIndex, m_toIndex);
}

// ==========================================
// DeleteCardsCommand
// ==========================================
DeleteCardsCommand::DeleteCardsCommand(MainWindow* window, const QList<PageCard*>& cards, QUndoCommand* parent)
    : QUndoCommand(parent), m_window(window) {
    for (auto* card : cards) {
        int idx = m_window->indexOfCard(card);
        if (idx >= 0) {
            m_deletedItems.append({card, idx});
        }
    }
    setText(QString("%1 Sayfayı Sil").arg(cards.size()));
}

void DeleteCardsCommand::undo() {
    // Restore cards in reverse order of index
    for (int i = m_deletedItems.size() - 1; i >= 0; --i) {
        const auto& item = m_deletedItems[i];
        m_window->internalInsertCard(item.originalIndex, item.card);
    }
}

void DeleteCardsCommand::redo() {
    for (const auto& item : m_deletedItems) {
        m_window->internalRemoveCard(item.card);
    }
}
