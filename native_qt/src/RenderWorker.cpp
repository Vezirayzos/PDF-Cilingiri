#include "RenderWorker.h"
#include <poppler-qt6.h>
#include <QFileInfo>
#include <cmath>

RenderWorker::RenderWorker(const QStringList& filePaths, QObject* parent)
    : QObject(parent), m_filePaths(filePaths) {
    setAutoDelete(true);
}

void RenderWorker::cancel() {
    m_cancelRequested.store(true);
}

void RenderWorker::run() {
    int totalFiles = m_filePaths.size();

    for (int fileIdx = 0; fileIdx < totalFiles; ++fileIdx) {
        if (m_cancelRequested.load()) break;

        const QString& filePath = m_filePaths[fileIdx];
        QFileInfo fi(filePath);
        QString ext = fi.suffix().toLower();

        emit progressUpdated(fileIdx + 1, totalFiles, fi.fileName());

        if (ext == "pdf") {
            auto doc = Poppler::Document::load(filePath);
            if (!doc || doc->isLocked()) continue;

            int numPages = doc->numPages();
            for (int p = 0; p < numPages; ++p) {
                if (m_cancelRequested.load()) break;

                auto page = doc->page(p);
                if (page) {
                    // Render lightweight preview (100 DPI) for UI responsiveness
                    QImage img = page->renderToImage(100, 100);
                    if (!img.isNull()) {
                        QImage thumb = img.scaled(180, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        emit pageReady(filePath, p + 1, numPages, thumb);
                    }
                }
            }
        } else {
            // Image file (JPG, PNG, WebP, BMP)
            QImage img(filePath);
            if (!img.isNull()) {
                QImage thumb = img.scaled(180, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                emit pageReady(filePath, 1, 1, thumb);
            }
        }
    }

    emit finished();
}
