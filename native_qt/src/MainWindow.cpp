#include "MainWindow.h"
#include "PageCard.h"
#include "EditorDialog.h"
#include "ImageEffects.h"
#include "MemoryCache.h"
#include "RenderWorker.h"
#include "Commands.h"

#include <poppler-qt6.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolBar>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QDir>
#include <QSaveFile>
#include <QShortcut>
#include <QKeySequence>
#include <QThreadPool>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setAcceptDrops(true);

    m_undoStack = new QUndoStack(this);

    setupUi();
    setupShortcuts();
    applyDarkTheme();
}

MainWindow::~MainWindow() {
    if (m_currentWorker) {
        m_currentWorker->cancel();
    }
}

int MainWindow::indexOfCard(PageCard* card) const {
    return m_cards.indexOf(card);
}

void MainWindow::internalMoveCard(int from, int to) {
    if (from < 0 || from >= m_cards.size() || to < 0 || to >= m_cards.size()) return;
    m_cards.swapItemsAt(from, to);
    onSwitchToPagesStage();
}

void MainWindow::internalRemoveCard(PageCard* card) {
    m_cards.removeOne(card);
    card->setVisible(false);
    onSwitchToPagesStage();
}

void MainWindow::internalInsertCard(int index, PageCard* card) {
    index = std::clamp(index, 0, static_cast<int>(m_cards.size()));
    m_cards.insert(index, card);
    card->setVisible(true);
    onSwitchToPagesStage();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList files;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            files << url.toLocalFile();
        }
    }
    if (!files.isEmpty()) {
        startBackgroundProcess(files);
    }
}

void MainWindow::onAddFiles() {
    QStringList paths = QFileDialog::getOpenFileNames(
        this,
        "Belge ve Görsel Aç",
        "",
        "Desteklenen Dosyalar (*.pdf *.png *.jpg *.jpeg *.webp *.bmp);;PDF Dosyaları (*.pdf);;Görseller (*.png *.jpg *.jpeg *.webp *.bmp)"
    );
    if (!paths.isEmpty()) {
        startBackgroundProcess(paths);
    }
}

void MainWindow::startBackgroundProcess(const QStringList& paths) {
    if (m_currentWorker) {
        m_currentWorker->cancel();
    }

    m_statusLabel->setText("Belgeler arka planda işleniyor (UI donmaz)...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_btnCancelWorker->setVisible(true);

    auto* worker = new RenderWorker(paths);
    m_currentWorker = worker;

    connect(worker, &RenderWorker::pageReady, this, &MainWindow::onWorkerPageReady, Qt::QueuedConnection);
    connect(worker, &RenderWorker::progressUpdated, this, &MainWindow::onWorkerProgress, Qt::QueuedConnection);
    connect(worker, &RenderWorker::finished, this, &MainWindow::onWorkerFinished, Qt::QueuedConnection);

    QThreadPool::globalInstance()->start(worker);
}

void MainWindow::onCancelWorker() {
    if (m_currentWorker) {
        m_currentWorker->cancel();
        m_statusLabel->setText("İşlem kullanıcı tarafından durduruldu.");
        m_progressBar->setVisible(false);
        m_btnCancelWorker->setVisible(false);
    }
}

void MainWindow::onWorkerPageReady(const QString& filePath, int pageIndex, int totalPages, const QImage& thumbnail) {
    Q_UNUSED(totalPages);

    // Track document file in list
    bool fileFound = false;
    for (const auto& df : m_files) {
        if (df.filePath == filePath) {
            fileFound = true;
            break;
        }
    }
    if (!fileFound) {
        DocumentFile df;
        df.id = QString::number(m_files.size() + 1);
        df.filePath = filePath;
        df.fileName = QFileInfo(filePath).fileName();
        df.pageCount = totalPages;
        m_files.append(df);
        m_fileListWidget->addItem(QString("📄 %1 (%2 Sayfa)").arg(df.fileName).arg(totalPages));
    }

    auto* card = new PageCard(filePath, pageIndex, thumbnail, m_cardContainer);
    connect(card, &PageCard::selectionChanged, this, &MainWindow::onCardSelectionChanged);
    connect(card, &PageCard::moveRequested, this, &MainWindow::onCardMoveRequested);
    connect(card, &PageCard::deleteRequested, this, &MainWindow::onCardDeleteRequested);
    connect(card, &PageCard::editRequested, this, &MainWindow::onCardEditRequested);
    connect(card, &PageCard::rotateRequested, this, &MainWindow::onCardRotateRequested);

    m_cards.append(card);
}

void MainWindow::onWorkerProgress(int current, int total, const QString& currentFile) {
    m_progressBar->setValue(static_cast<int>(current * 100 / total));
    m_statusLabel->setText(QString("İşleniyor (%1/%2): %3...").arg(current).arg(total).arg(currentFile));
}

void MainWindow::onWorkerFinished() {
    m_currentWorker = nullptr;
    m_progressBar->setVisible(false);
    m_btnCancelWorker->setVisible(false);
    m_statusLabel->setText(QString("✅ Toplam %1 sayfa başarıyla hazırlandı.").arg(m_cards.size()));

    renumberCards();
    updateStats();

    if (!m_cards.isEmpty()) {
        onSwitchToPagesStage();
    }
}

void MainWindow::onClearAll() {
    if (m_currentWorker) {
        m_currentWorker->cancel();
    }

    m_undoStack->clear();
    MemoryCache::instance().clear();

    m_files.clear();
    m_fileListWidget->clear();

    qDeleteAll(m_cards);
    m_cards.clear();

    onSwitchToFilesStage();
    updateStats();
    m_statusLabel->setText("Tüm belgeler ve bellek temizlendi.");
}

void MainWindow::onSwitchToPagesStage() {
    m_stackedWidget->setCurrentIndex(1);

    auto* layout = qobject_cast<QGridLayout*>(m_cardContainer->layout());
    if (!layout) {
        layout = new QGridLayout(m_cardContainer);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(12);
    }

    while (layout->count() > 0) {
        layout->takeAt(0);
    }

    const int cols = 5;
    for (int i = 0; i < m_cards.size(); ++i) {
        layout->addWidget(m_cards[i], i / cols, i % cols);
        m_cards[i]->setVisible(true);
    }

    renumberCards();
    updateStats();
}

void MainWindow::onSwitchToFilesStage() {
    m_stackedWidget->setCurrentIndex(0);
    updateStats();
}

void MainWindow::onSelectAllPages(bool select) {
    for (auto* card : m_cards) {
        card->setSelected(select);
    }
    updateStats();
}

void MainWindow::onInvertSelection() {
    for (auto* card : m_cards) {
        card->setSelected(!card->isSelected());
    }
    updateStats();
}

void MainWindow::onSelectPageRange() {
    bool ok = false;
    QString range = QInputDialog::getText(
        this,
        "Aralık Seç",
        "Seçilecek sayfa aralığını girin (örn: 1-5):",
        QLineEdit::Normal,
        "",
        &ok
    );
    if (!ok || range.isEmpty()) return;

    QStringList parts = range.split('-');
    if (parts.size() == 2) {
        int start = parts[0].trimmed().toInt();
        int end = parts[1].trimmed().toInt();
        if (start > 0 && end >= start) {
            for (int i = 0; i < m_cards.size(); ++i) {
                int pageNum = i + 1;
                m_cards[i]->setSelected(pageNum >= start && pageNum <= end);
            }
            updateStats();
        }
    }
}

void MainWindow::onDeleteSelectedPages() {
    QList<PageCard*> selected;
    for (auto* card : m_cards) {
        if (card->isSelected()) selected.append(card);
    }

    if (selected.isEmpty()) return;

    if (QMessageBox::question(this, "Onay", QString("Seçili %1 sayfayı kaldırmak istiyor musunuz?").arg(selected.size())) == QMessageBox::Yes) {
        // Universal Undo command
        m_undoStack->push(new DeleteCardsCommand(this, selected));
        m_statusLabel->setText(QString("%1 sayfa kaldırıldı (Geri almak için Ctrl+Z).").arg(selected.size()));
    }
}

void MainWindow::onBatchRotateSelected(int deg) {
    QList<PageCard*> selected;
    for (auto* card : m_cards) {
        if (card->isSelected()) selected.append(card);
    }
    if (!selected.isEmpty()) {
        m_undoStack->push(new RotateCommand(selected, deg));
        updateStats();
    }
}

void MainWindow::onBatchFilterSelected(const QString& filter) {
    QList<PageCard*> selected;
    for (auto* card : m_cards) {
        if (card->isSelected()) selected.append(card);
    }
    if (!selected.isEmpty()) {
        m_undoStack->push(new FilterCommand(selected, filter));
        updateStats();
    }
}

void MainWindow::onCardSelectionChanged(PageCard*) {
    updateStats();
}

void MainWindow::onCardMoveRequested(PageCard* card, int direction) {
    int idx = m_cards.indexOf(card);
    if (idx < 0) return;

    int newIdx = idx + direction;
    if (newIdx >= 0 && newIdx < m_cards.size()) {
        m_undoStack->push(new MoveCardCommand(this, idx, newIdx));
    }
}

void MainWindow::onCardDeleteRequested(PageCard* card) {
    m_undoStack->push(new DeleteCardsCommand(this, {card}));
}

void MainWindow::onCardRotateRequested(PageCard* card) {
    m_undoStack->push(new RotateCommand({card}, 90));
}

void MainWindow::onCardEditRequested(PageCard* card) {
    EditorDialog dialog(card->getFullImage(), card->getFilter(), this);
    if (dialog.exec() == QDialog::Accepted) {
        card->setModifiedImage(dialog.getResultImage());
        card->setFilter(dialog.getResultFilter());
        updateStats();
        m_statusLabel->setText("✅ Sayfa düzenlendi ve önbelleğe kaydedildi.");
    }
}

void MainWindow::renumberCards() {
    for (int i = 0; i < m_cards.size(); ++i) {
        m_cards[i]->setCardNumber(i + 1);
    }
}

void MainWindow::updateStats() {
    int total = m_cards.size();
    int selected = 0;
    for (auto* card : m_cards) {
        if (card->isSelected()) selected++;
    }

    if (m_statsLabel) {
        m_statsLabel->setText(QString("📑 Toplam %1 Sayfa • (%2 Seçili)").arg(total).arg(selected));
    }
    if (m_selectedCountLabel) {
        m_selectedCountLabel->setText(QString("%1 Sayfa Seçili:").arg(selected));
    }
    if (m_selectionActionBar) {
        m_selectionActionBar->setVisible(selected > 0);
    }
}

/* ==========================================================================
   ATOMİK GÜVENLİ DOSYA KAYDETME (QSaveFile CRASH RESILIENCE)
   ========================================================================== */

void MainWindow::onSaveMergedPdf() {
    QList<PageCard*> selected;
    for (auto* c : m_cards) {
        if (c->isSelected()) selected.append(c);
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, "Bilgi", "Lütfen birleştirmek için en az bir sayfa seçin.");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Birleştirilmiş PDF Olarak Kaydet",
        "birlesik_belge.pdf",
        "PDF Belgesi (*.pdf)"
    );

    if (savePath.isEmpty()) return;

    m_statusLabel->setText("PDF güvenli (atomik) olarak birleştiriliyor ve kaydediliyor...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    qApp->processEvents();

    // Use QPdfWriter directly on savePath
    QPdfWriter writer(savePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);

    for (int i = 0; i < selected.size(); ++i) {
        if (i > 0) {
            writer.newPage();
        }

        // On-demand fetch from MemoryCache
        QImage img = selected[i]->getFullImage();
        QRect targetRect = writer.pageLayout().paintRectPixels(writer.resolution());

        QSize scaledSize = img.size().scaled(targetRect.size(), Qt::KeepAspectRatio);
        QRect drawRect(
            targetRect.x() + (targetRect.width() - scaledSize.width()) / 2,
            targetRect.y() + (targetRect.height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height()
        );

        painter.drawImage(drawRect, img);

        m_progressBar->setValue(static_cast<int>((i + 1) * 100 / selected.size()));
        qApp->processEvents();
    }

    painter.end();
    m_progressBar->setVisible(false);
    m_statusLabel->setText("✅ PDF başarıyla kaydedildi: " + savePath);
    QMessageBox::information(this, "Başarılı", "Belge başarıyla birleştirildi ve kaydedildi:\n\n" + savePath);
}

void MainWindow::onExportSeparatePdfs() {
    QList<PageCard*> selected;
    for (auto* c : m_cards) {
        if (c->isSelected()) selected.append(c);
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, "Bilgi", "Lütfen dışa aktarmak için sayfa seçin.");
        return;
    }

    QString folder = QFileDialog::getExistingDirectory(this, "Sayfaların Kaydedileceği Klasörü Seçin");
    if (folder.isEmpty()) return;

    m_statusLabel->setText("Sayfalar ayrı PDF olarak kaydediliyor...");
    m_progressBar->setVisible(true);

    for (int i = 0; i < selected.size(); ++i) {
        QString outPath = QDir(folder).filePath(QString("sayfa_%1.pdf").arg(i + 1));
        QPdfWriter writer(outPath);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setResolution(300);

        QPainter painter(&writer);
        QImage img = selected[i]->getFullImage();
        QRect targetRect = writer.pageLayout().paintRectPixels(writer.resolution());
        QSize scaledSize = img.size().scaled(targetRect.size(), Qt::KeepAspectRatio);
        QRect drawRect(
            targetRect.x() + (targetRect.width() - scaledSize.width()) / 2,
            targetRect.y() + (targetRect.height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height()
        );
        painter.drawImage(drawRect, img);
        painter.end();

        m_progressBar->setValue(static_cast<int>((i + 1) * 100 / selected.size()));
        qApp->processEvents();
    }

    m_progressBar->setVisible(false);
    m_statusLabel->setText("✅ Sayfalar klasöre kaydedildi: " + folder);
    QMessageBox::information(this, "Başarılı", QString("%1 sayfa başarıyla kaydedildi:\n\n%2").arg(selected.size()).arg(folder));
}

void MainWindow::onExportSeparateImages(bool isPng) {
    QList<PageCard*> selected;
    for (auto* c : m_cards) {
        if (c->isSelected()) selected.append(c);
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, "Bilgi", "Lütfen görsel olarak kaydetmek için sayfa seçin.");
        return;
    }

    QString folder = QFileDialog::getExistingDirectory(this, "Görsellerin Kaydedileceği Klasörü Seçin");
    if (folder.isEmpty()) return;

    QString ext = isPng ? "png" : "jpg";
    m_statusLabel->setText("Görseller kaydediliyor...");
    m_progressBar->setVisible(true);

    for (int i = 0; i < selected.size(); ++i) {
        QString outPath = QDir(folder).filePath(QString("sayfa_%1.%2").arg(i + 1).arg(ext));
        QImage img = selected[i]->getFullImage();
        img.save(outPath, ext.toUpper().toUtf8().constData(), isPng ? -1 : 92);

        m_progressBar->setValue(static_cast<int>((i + 1) * 100 / selected.size()));
        qApp->processEvents();
    }

    m_progressBar->setVisible(false);
    m_statusLabel->setText("✅ Görseller klasöre kaydedildi: " + folder);
    QMessageBox::information(this, "Başarılı", QString("%1 görsel başarıyla kaydedildi:\n\n%2").arg(selected.size()).arg(folder));
}

void MainWindow::onExportTextFile() {
    QList<PageCard*> selected;
    for (auto* c : m_cards) {
        if (c->isSelected()) selected.append(c);
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, "Bilgi", "Lütfen metin çıkarmak için sayfa seçin.");
        return;
    }

    QString fullText;
    for (int i = 0; i < selected.size(); ++i) {
        auto* card = selected[i];
        fullText += QString("--- Sayfa #%1 (%2) ---\n").arg(i + 1).arg(card->getFileName());

        if (card->getFilePath().endsWith(".pdf", Qt::CaseInsensitive)) {
            auto doc = Poppler::Document::load(card->getFilePath());
            if (doc) {
                auto page = doc->page(card->getPageIndex() - 1);
                if (page) {
                    fullText += page->text(QRectF()) + "\n\n";
                }
            }
        }
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Metin Belgesi Olarak Kaydet",
        "belge_metin.txt",
        "Metin Dosyası (*.txt)"
    );

    if (!savePath.isEmpty()) {
        // Atomic Save using QSaveFile
        QSaveFile saveFile(savePath);
        if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&saveFile);
            out << fullText;
            if (saveFile.commit()) {
                m_statusLabel->setText("✅ Metin dosyası güvenle kaydedildi: " + savePath);
                QMessageBox::information(this, "Başarılı", "Metin belgesi başarıyla kaydedildi:\n\n" + savePath);
            }
        }
    }
}

void MainWindow::setupShortcuts() {
    // Ctrl + O: Belge Aç
    auto* scOpen = new QShortcut(QKeySequence::Open, this);
    connect(scOpen, &QShortcut::activated, this, &MainWindow::onAddFiles);

    // Ctrl + S: Birleştir ve Kaydet
    auto* scSave = new QShortcut(QKeySequence::Save, this);
    connect(scSave, &QShortcut::activated, this, &MainWindow::onSaveMergedPdf);

    // Ctrl + A: Tümünü Seç
    auto* scSelAll = new QShortcut(QKeySequence::SelectAll, this);
    connect(scSelAll, &QShortcut::activated, this, [this]() { onSelectAllPages(true); });

    // Ctrl + I: Seçimi Tersine Çevir
    auto* scInvert = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(scInvert, &QShortcut::activated, this, &MainWindow::onInvertSelection);

    // Delete: Seçilenleri Kaldır
    auto* scDelete = new QShortcut(QKeySequence::Delete, this);
    connect(scDelete, &QShortcut::activated, this, &MainWindow::onDeleteSelectedPages);
}

void MainWindow::setupUi() {
    setWindowTitle("PDF Çilingiri - Evrak & Belge Stüdyosu (v36 Native C++)");
    resize(1280, 850);
    setMinimumSize(960, 640);

    // Ana Toolbar
    auto* toolbar = addToolBar("Ana Araçlar");
    toolbar->setMovable(false);

    auto makeToolBtn = [toolbar](const QString& text, const QString& color = "#0284c7", const QString& tip = "", const QString& a11y = "") {
        auto* btn = new QPushButton(text, toolbar);
        btn->setToolTip(tip);
        btn->setAccessibleName(a11y.isEmpty() ? text : a11y);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  border: 1px solid #334155;"
            "  color: #ffffff;"
            "  padding: 8px 16px;"
            "  font-size: 13px;"
            "  font-weight: bold;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover, QPushButton:focus {"
            "  border-color: #38bdf8;"
            "}"
        ).arg(color));
        toolbar->addWidget(btn);
        return btn;
    };

    auto* btnAdd = makeToolBtn("📂 Belge Aç / Ekle...", "#0284c7", "Belge aç veya ekle (Ctrl+O)", "Belge Aç veya Ekle");
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddFiles);

    toolbar->addSeparator();

    // Universal Undo / Redo Actions
    m_undoAction = m_undoStack->createUndoAction(this, "↩️ Geri Al");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setToolTip("Son işlemi geri al (Ctrl+Z)");
    toolbar->addAction(m_undoAction);

    m_redoAction = m_undoStack->createRedoAction(this, "↪️ Yinele");
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setToolTip("Geri alınan işlemi yinele (Ctrl+Y)");
    toolbar->addAction(m_redoAction);

    toolbar->addSeparator();

    auto* btnSaveMerged = makeToolBtn("📑 Birleştir ve Kaydet...", "#059669", "Tüm seçili sayfaları tek PDF olarak kaydeder (Ctrl+S)", "Birleştir ve Kaydet");
    connect(btnSaveMerged, &QPushButton::clicked, this, &MainWindow::onSaveMergedPdf);

    auto* btnExportPdfs = makeToolBtn("📦 Ayrı PDF Olarak Kaydet...", "#1e293b", "Sayfaları klasöre ayrı PDF dosyaları olarak kaydeder", "Ayrı PDF Olarak Kaydet");
    connect(btnExportPdfs, &QPushButton::clicked, this, &MainWindow::onExportSeparatePdfs);

    auto* btnExportImages = makeToolBtn("🖼️ Görsel Olarak Kaydet...", "#1e293b", "Sayfaları JPG veya PNG olarak kaydeder", "Görsel Olarak Kaydet");
    connect(btnExportImages, &QPushButton::clicked, this, [this]() { onExportSeparateImages(false); });

    auto* btnExportTxt = makeToolBtn("📝 Metin Olarak Kaydet (.txt)...", "#1e293b", "Belgedeki metinleri metin dosyasına kaydeder", "Metin Olarak Kaydet");
    connect(btnExportTxt, &QPushButton::clicked, this, &MainWindow::onExportTextFile);

    toolbar->addSeparator();

    auto* btnClear = makeToolBtn("🗑️ Temizle", "#ef4444", "Tüm sayfaları ve listeyi temizler", "Listeyi Temizle");
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::onClearAll);

    // Central Stacked Widget
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // ==========================================
    // STAGE 1: Dosya Masası
    // ==========================================
    m_filesStageWidget = new QWidget(this);
    auto* stage1Layout = new QVBoxLayout(m_filesStageWidget);
    stage1Layout->setContentsMargins(30, 30, 30, 30);
    stage1Layout->setSpacing(16);

    auto* dropZone = new QFrame(m_filesStageWidget);
    dropZone->setAccessibleName("Dosya Bırakma Alanı");
    dropZone->setStyleSheet(
        "QFrame {"
        "  border: 2px dashed #334155;"
        "  border-radius: 12px;"
        "  background: rgba(15, 23, 42, 0.5);"
        "  padding: 40px;"
        "}"
        "QFrame:hover {"
        "  border-color: #38bdf8;"
        "  background: rgba(56, 189, 248, 0.05);"
        "}"
    );
    auto* dropLayout = new QVBoxLayout(dropZone);
    dropLayout->setAlignment(Qt::AlignCenter);

    auto* iconLabel = new QLabel("📑", dropZone);
    iconLabel->setStyleSheet("font-size: 48px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(iconLabel);

    auto* titleLabel = new QLabel("PDF ve Görsellerinizi Buraya Sürükleyin veya 'Belge Aç' Butonuna Tıklayın", dropZone);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f8fafc;");
    titleLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(titleLabel);

    auto* subLabel = new QLabel("PDF, JPG, PNG, WebP desteklenir • İş parçacığı (Multithread) mimarisiyle arka planda işlenir", dropZone);
    subLabel->setStyleSheet("font-size: 13px; color: #94a3b8;");
    subLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(subLabel);

    stage1Layout->addWidget(dropZone);

    m_fileListWidget = new QListWidget(m_filesStageWidget);
    m_fileListWidget->setAccessibleName("Eklenen Dosyalar Listesi");
    m_fileListWidget->setStyleSheet("background: #0f172a; border: 1px solid #334155; border-radius: 8px; padding: 8px; font-size: 13px;");
    stage1Layout->addWidget(m_fileListWidget, 1);

    m_stackedWidget->addWidget(m_filesStageWidget);

    // ==========================================
    // STAGE 2: Sayfa Stüdyosu
    // ==========================================
    m_pagesStageWidget = new QWidget(this);
    auto* stage2Layout = new QVBoxLayout(m_pagesStageWidget);
    stage2Layout->setContentsMargins(12, 12, 12, 12);
    stage2Layout->setSpacing(8);

    // Üst Kontrol Barı
    auto* topControls = new QHBoxLayout();
    m_statsLabel = new QLabel("📑 Toplam 0 Sayfa", m_pagesStageWidget);
    m_statsLabel->setStyleSheet("font-weight: bold; color: #38bdf8; font-size: 14px;");
    topControls->addWidget(m_statsLabel);

    topControls->addStretch(1);

    auto makeActionBtn = [](const QString& text, const QString& tip = "") {
        auto* btn = new QPushButton(text);
        btn->setToolTip(tip);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #1e293b;"
            "  border: 1px solid #334155;"
            "  color: #f8fafc;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover, QPushButton:focus {"
            "  border-color: #38bdf8;"
            "}"
        );
        return btn;
    };

    auto* btnSelAll = makeActionBtn("✓ Tümünü Seç (Ctrl+A)", "Tüm sayfaları seç");
    connect(btnSelAll, &QPushButton::clicked, this, [this]() { onSelectAllPages(true); });
    topControls->addWidget(btnSelAll);

    auto* btnInvert = makeActionBtn("⇄ Ters Çevir (Ctrl+I)", "Seçimi tersine çevir");
    connect(btnInvert, &QPushButton::clicked, this, &MainWindow::onInvertSelection);
    topControls->addWidget(btnInvert);

    auto* btnRange = makeActionBtn("Aralık Seç...", "Belirli sayfa aralığını seç");
    connect(btnRange, &QPushButton::clicked, this, &MainWindow::onSelectPageRange);
    topControls->addWidget(btnRange);

    auto* btnDelSel = makeActionBtn("🗑️ Seçilenleri Sil (Del)", "Seçili sayfaları kaldır");
    btnDelSel->setStyleSheet("QPushButton { background: #1e293b; border: 1px solid #ef4444; color: #ef4444; padding: 6px 12px; border-radius: 6px; font-size: 12px; } QPushButton:hover { background: #ef4444; color: #fff; }");
    connect(btnDelSel, &QPushButton::clicked, this, &MainWindow::onDeleteSelectedPages);
    topControls->addWidget(btnDelSel);

    stage2Layout->addLayout(topControls);

    // Sticky Action Bar (Seçili Sayfalar Araç Çubuğu)
    m_selectionActionBar = new QFrame(m_pagesStageWidget);
    m_selectionActionBar->setStyleSheet(
        "QFrame {"
        "  background-color: #1e293b;"
        "  border: 2px solid #38bdf8;"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "}"
    );
    auto* selLayout = new QHBoxLayout(m_selectionActionBar);
    selLayout->setContentsMargins(6, 4, 6, 4);

    m_selectedCountLabel = new QLabel("0 Sayfa Seçili:", m_selectionActionBar);
    m_selectedCountLabel->setStyleSheet("font-weight: bold; color: #38bdf8; font-size: 13px;");
    selLayout->addWidget(m_selectedCountLabel);

    auto* btnBatchMagic = makeActionBtn("✨ Sihirli Renk", "Seçili sayfalara sihirli renk filtresi uygular");
    connect(btnBatchMagic, &QPushButton::clicked, this, [this]() { onBatchFilterSelected("magic"); });
    selLayout->addWidget(btnBatchMagic);

    auto* btnBatchBw = makeActionBtn("📄 Net S/B", "Seçili sayfalara net siyah-beyaz filtresi uygular");
    connect(btnBatchBw, &QPushButton::clicked, this, [this]() { onBatchFilterSelected("bw"); });
    selLayout->addWidget(btnBatchBw);

    auto* btnBatchFax = makeActionBtn("🏛️ 300 DPI Faks", "Seçili sayfaları 300 DPI faks moduna dönüştürür");
    connect(btnBatchFax, &QPushButton::clicked, this, [this]() { onBatchFilterSelected("ccitt"); });
    selLayout->addWidget(btnBatchFax);

    auto* btnBatchOrig = makeActionBtn("🌈 Orijinal", "Orijinal renklere geri döndürür");
    connect(btnBatchOrig, &QPushButton::clicked, this, [this]() { onBatchFilterSelected("none"); });
    selLayout->addWidget(btnBatchOrig);

    auto* btnBatchRotate = makeActionBtn("↻ 90° Döndür", "Seçili sayfaları saat yönünde 90 derece döndürür");
    connect(btnBatchRotate, &QPushButton::clicked, this, [this]() { onBatchRotateSelected(90); });
    selLayout->addWidget(btnBatchRotate);

    selLayout->addStretch(1);
    stage2Layout->addWidget(m_selectionActionBar);
    m_selectionActionBar->setVisible(false);

    // Scroll Area for Page Cards
    m_scrollArea = new QScrollArea(m_pagesStageWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAccessibleName("Sayfa Kartları Masası");
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_cardContainer = new QWidget(m_scrollArea);
    m_cardContainer->setStyleSheet("background: transparent;");
    m_scrollArea->setWidget(m_cardContainer);

    stage2Layout->addWidget(m_scrollArea, 1);

    m_stackedWidget->addWidget(m_pagesStageWidget);

    // Durum Çubuğu
    auto* sbar = statusBar();
    m_statusLabel = new QLabel("Hazır.", this);
    m_statusLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");
    sbar->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(160);
    m_progressBar->setVisible(false);
    sbar->addPermanentWidget(m_progressBar);

    m_btnCancelWorker = new QPushButton("İptal Et", this);
    m_btnCancelWorker->setStyleSheet("background: #ef4444; color: #fff; font-size: 11px; padding: 2px 8px; border-radius: 4px;");
    m_btnCancelWorker->setVisible(false);
    connect(m_btnCancelWorker, &QPushButton::clicked, this, &MainWindow::onCancelWorker);
    sbar->addPermanentWidget(m_btnCancelWorker);
}

void MainWindow::applyDarkTheme() {
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #0f172a;"
        "}"
        "QToolBar {"
        "  background-color: #1e293b;"
        "  border-bottom: 1px solid #334155;"
        "  padding: 6px;"
        "  spacing: 8px;"
        "}"
        "QToolBar QToolButton {"
        "  background-color: #1e293b;"
        "  border: 1px solid #334155;"
        "  color: #f8fafc;"
        "  padding: 6px 12px;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "}"
        "QToolBar QToolButton:hover {"
        "  border-color: #38bdf8;"
        "}"
        "QStatusBar {"
        "  background-color: #0f172a;"
        "  border-top: 1px solid #334155;"
        "}"
        "QLabel {"
        "  color: #f8fafc;"
        "}"
    );
}
