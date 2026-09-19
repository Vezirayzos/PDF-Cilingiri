#include "ImageEffects.h"
#include <QPainter>
#include <QTransform>
#include <cmath>
#include <algorithm>

void ImageEffects::applyMagicColor(QImage& img) {
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }

    int width = img.width();
    int height = img.height();

    for (int y = 0; y < height; ++y) {
        QRgb* scanLine = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = scanLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qAlpha(pixel);

            // Magic color: Boost contrast & brightness
            r = std::clamp(static_cast<int>((r - 128) * 1.35 + 128 + 20), 0, 255);
            g = std::clamp(static_cast<int>((g - 128) * 1.35 + 128 + 20), 0, 255);
            b = std::clamp(static_cast<int>((b - 128) * 1.35 + 128 + 20), 0, 255);

            // Clean background: If light gray / off-white, snap to pure white
            if (r > 190 && g > 190 && b > 190) {
                r = 255;
                g = 255;
                b = 255;
            }

            scanLine[x] = qRgba(r, g, b, a);
        }
    }
}

void ImageEffects::applyBwThreshold(QImage& img, int threshold) {
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }

    int width = img.width();
    int height = img.height();

    for (int y = 0; y < height; ++y) {
        QRgb* scanLine = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = scanLine[x];
            int gray = (qRed(pixel) * 299 + qGreen(pixel) * 587 + qBlue(pixel) * 114) / 1000;
            int val = (gray > threshold) ? 255 : 0;
            scanLine[x] = qRgba(val, val, val, qAlpha(pixel));
        }
    }
}

void ImageEffects::applyFax300Dpi(QImage& img) {
    // 300 DPI for A4 width (approx 2480 pixels width)
    int targetW = 2480;
    if (img.width() > 0 && std::abs(img.width() - targetW) > 200) {
        int targetH = static_cast<int>(static_cast<double>(targetW) / img.width() * img.height());
        img = img.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    applyBwThreshold(img, 140);
}

QImage ImageEffects::rotateImage(const QImage& img, int deg) {
    deg = (deg % 360 + 360) % 360;
    if (deg == 0) return img;

    QTransform t;
    t.rotate(deg);
    return img.transformed(t, Qt::SmoothTransformation);
}

bool ImageEffects::isColorImage(const QImage& img) {
    if (img.isNull()) return false;

    int sampleStep = std::max(1, img.width() / 30);
    int colorPixels = 0;
    int total = 0;

    for (int y = 0; y < img.height(); y += sampleStep) {
        for (int x = 0; x < img.width(); x += sampleStep) {
            QRgb p = img.pixel(x, y);
            int r = qRed(p);
            int g = qGreen(p);
            int b = qBlue(p);
            if (std::abs(r - g) > 18 || std::abs(g - b) > 18 || std::abs(r - b) > 18) {
                colorPixels++;
            }
            total++;
        }
    }
    return (total > 0) && (static_cast<double>(colorPixels) / total > 0.02);
}

static QPointF bilinearInterpolate(const QPointF& tl, const QPointF& tr, const QPointF& br, const QPointF& bl, double u, double v) {
    double x = (1.0 - u) * (1.0 - v) * tl.x() + u * (1.0 - v) * tr.x() + u * v * br.x() + (1.0 - u) * v * bl.x();
    double y = (1.0 - u) * (1.0 - v) * tl.y() + u * (1.0 - v) * tr.y() + u * v * br.y() + (1.0 - u) * v * bl.y();
    return QPointF(x, y);
}

QImage ImageEffects::perspectiveWarp(const QImage& src, const QPolygonF& corners) {
    if (corners.size() != 4 || src.isNull()) return src;

    QPointF tl = corners[0];
    QPointF tr = corners[1];
    QPointF br = corners[2];
    QPointF bl = corners[3];

    double wTop = std::hypot(tr.x() - tl.x(), tr.y() - tl.y());
    double wBottom = std::hypot(br.x() - bl.x(), br.y() - bl.y());
    int targetW = std::max(100, static_cast<int>(std::round(std::max(wTop, wBottom))));

    double hLeft = std::hypot(bl.x() - tl.x(), bl.y() - tl.y());
    double hRight = std::hypot(br.x() - tr.x(), br.y() - tr.y());
    int targetH = std::max(100, static_cast<int>(std::round(std::max(hLeft, hRight))));

    QImage warped(targetW, targetH, QImage::Format_ARGB32);
    warped.fill(Qt::white);

    const int steps = 32;
    QPainter painter(&warped);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    for (int yi = 0; yi < steps; ++yi) {
        for (int xi = 0; xi < steps; ++xi) {
            double u0 = static_cast<double>(xi) / steps;
            double u1 = static_cast<double>(xi + 1) / steps;
            double v0 = static_cast<double>(yi) / steps;
            double v1 = static_cast<double>(yi + 1) / steps;

            QPointF p00 = bilinearInterpolate(tl, tr, br, bl, u0, v0);
            QPointF p10 = bilinearInterpolate(tl, tr, br, bl, u1, v0);
            QPointF p11 = bilinearInterpolate(tl, tr, br, bl, u1, v1);
            QPointF p01 = bilinearInterpolate(tl, tr, br, bl, u0, v1);

            double dx = u0 * targetW;
            double dy = v0 * targetH;
            double dw = (u1 - u0) * targetW;
            double dh = (v1 - v0) * targetH;

            double sx = std::min({p00.x(), p01.x()});
            double sy = std::min({p00.y(), p10.y()});
            double sw = std::max({p10.x(), p11.x()}) - sx;
            double sh = std::max({p01.y(), p11.y()}) - sy;

            if (sw > 0 && sh > 0) {
                QRectF srcRect(sx, sy, sw, sh);
                QRectF dstRect(dx, dy, dw, dh);
                painter.drawImage(dstRect, src, srcRect);
            }
        }
    }

    return warped;
}
