#include "MemoryCache.h"
#include <poppler-qt6.h>
#include <QFileInfo>

MemoryCache& MemoryCache::instance() {
    static MemoryCache s_instance;
    return s_instance;
}

MemoryCache::MemoryCache() {
    // 200 MB max cost (in KB: 200 * 1024 = 204800 KB)
    m_cache.setMaxCost(200 * 1024);
}

void MemoryCache::setMaxCostMb(int mb) {
    QMutexLocker locker(&m_mutex);
    m_cache.setMaxCost(mb * 1024);
}

QString MemoryCache::makeKey(const QString& filePath, int pageIndex) {
    return QString("%1#%2").arg(filePath).arg(pageIndex);
}

QImage MemoryCache::getFullImage(const QString& filePath, int pageIndex) {
    QMutexLocker locker(&m_mutex);
    QString key = makeKey(filePath, pageIndex);

    QImage* cached = m_cache.object(key);
    if (cached) {
        return *cached;
    }

    // Load on demand
    QImage loaded = loadFromDisk(filePath, pageIndex);
    if (!loaded.isNull()) {
        int costKb = static_cast<int>(loaded.sizeInBytes() / 1024);
        m_cache.insert(key, new QImage(loaded), costKb);
    }
    return loaded;
}

void MemoryCache::putFullImage(const QString& filePath, int pageIndex, const QImage& image) {
    QMutexLocker locker(&m_mutex);
    QString key = makeKey(filePath, pageIndex);
    int costKb = static_cast<int>(image.sizeInBytes() / 1024);
    m_cache.insert(key, new QImage(image), costKb);
}

void MemoryCache::clear() {
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

QImage MemoryCache::loadFromDisk(const QString& filePath, int pageIndex) {
    QFileInfo fi(filePath);
    if (fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        auto doc = Poppler::Document::load(filePath);
        if (doc && !doc->isLocked() && pageIndex >= 1 && pageIndex <= doc->numPages()) {
            auto page = doc->page(pageIndex - 1);
            if (page) {
                // High resolution 300 DPI for editing and export
                return page->renderToImage(300, 300);
            }
        }
        return QImage();
    } else {
        return QImage(filePath);
    }
}
