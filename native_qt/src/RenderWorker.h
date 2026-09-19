#pragma once

#include <QObject>
#include <QRunnable>
#include <QStringList>
#include <QImage>
#include <atomic>

class RenderWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit RenderWorker(const QStringList& filePaths, QObject* parent = nullptr);

    void run() override;
    void cancel();
    bool isCancelled() const { return m_cancelRequested.load(); }

signals:
    void pageReady(const QString& filePath, int pageIndex, int totalPages, const QImage& thumbnail);
    void progressUpdated(int current, int total, const QString& currentFile);
    void finished();

private:
    QStringList m_filePaths;
    std::atomic<bool> m_cancelRequested{false};
};
