#pragma once

#include <QCache>
#include <QImage>
#include <QString>
#include <QMutex>
#include <memory>

// LRU cache for full-resolution pages so we never exhaust RAM
class MemoryCache {
public:
    static MemoryCache& instance();

    // Cache key format: "filePath#pageIndex"
    static QString makeKey(const QString& filePath, int pageIndex);

    QImage getFullImage(const QString& filePath, int pageIndex);
    void putFullImage(const QString& filePath, int pageIndex, const QImage& image);
    void clear();

    // Max cache size in Megabytes (default: 200 MB)
    void setMaxCostMb(int mb);

private:
    MemoryCache();
    ~MemoryCache() = default;

    QImage loadFromDisk(const QString& filePath, int pageIndex);

    QCache<QString, QImage> m_cache;
    QMutex m_mutex;
};
