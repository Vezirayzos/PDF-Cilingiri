#pragma once

#include <QMainWindow>
#include <QList>
#include <QStringList>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUndoStack>

class QScrollArea;
class QStackedWidget;
class QListWidget;
class QLabel;
class QPushButton;
class QProgressBar;
class QAction;
class PageCard;
class RenderWorker;

struct DocumentFile {
    QString id;
    QString filePath;
    QString fileName;
    int pageCount = 0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Helpers for QUndoCommand
    int indexOfCard(PageCard* card) const;
    void internalMoveCard(int from, int to);
    void internalRemoveCard(PageCard* card);
    void internalInsertCard(int index, PageCard* card);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onAddFiles();
    void onClearAll();
    void onSwitchToPagesStage();
    void onSwitchToFilesStage();

    void onSelectAllPages(bool select);
    void onInvertSelection();
    void onSelectPageRange();
    void onDeleteSelectedPages();
    void onBatchRotateSelected(int deg);
    void onBatchFilterSelected(const QString& filter);

    // Kaydetme & Dışa Aktarma Eylemleri (Atomik Dosya Kaydetme)
    void onSaveMergedPdf();
    void onExportSeparatePdfs();
    void onExportSeparateImages(bool isPng);
    void onExportTextFile();

    // Sayfa Kartı Olayları
    void onCardSelectionChanged(PageCard* card);
    void onCardMoveRequested(PageCard* card, int direction);
    void onCardDeleteRequested(PageCard* card);
    void onCardEditRequested(PageCard* card);
    void onCardRotateRequested(PageCard* card);

    // Arka Plan İşçi Olayları
    void onWorkerPageReady(const QString& filePath, int pageIndex, int totalPages, const QImage& thumbnail);
    void onWorkerProgress(int current, int total, const QString& currentFile);
    void onWorkerFinished();
    void onCancelWorker();

private:
    void setupUi();
    void setupShortcuts();
    void applyDarkTheme();
    void startBackgroundProcess(const QStringList& paths);
    void updateStats();
    void renumberCards();

    QList<DocumentFile> m_files;
    QList<PageCard*> m_cards;

    // Universal Undo/Redo
    QUndoStack* m_undoStack = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;

    // Background worker
    RenderWorker* m_currentWorker = nullptr;

    QStackedWidget* m_stackedWidget = nullptr;
    
    // Kademe 1: Dosya Masası
    QWidget* m_filesStageWidget = nullptr;
    QListWidget* m_fileListWidget = nullptr;
    QLabel* m_filesCountLabel = nullptr;

    // Kademe 2: Sayfa Stüdyosu
    QWidget* m_pagesStageWidget = nullptr;
    QWidget* m_cardContainer = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QLabel* m_statsLabel = nullptr;
    QWidget* m_selectionActionBar = nullptr;
    QLabel* m_selectedCountLabel = nullptr;

    // Durum Çubuğu
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_btnCancelWorker = nullptr;
};
