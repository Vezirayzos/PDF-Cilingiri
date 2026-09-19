#pragma once

#include <QImage>
#include <QPolygonF>

class ImageEffects {
public:
    static void applyMagicColor(QImage& img);
    static void applyBwThreshold(QImage& img, int threshold = 140);
    static void applyFax300Dpi(QImage& img);
    static QImage rotateImage(const QImage& img, int deg);
    static QImage perspectiveWarp(const QImage& src, const QPolygonF& corners);
    static bool isColorImage(const QImage& img);
};
