#include "EditorDialog.h"
#include "ImageEffects.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QWidget>
#include <QLabel>
#include <QTransform>
#include <cmath>

class EditorCanvas : public QWidget {
public:
    explicit EditorCanvas(QWidget* parent = nullptr) : QWidget(parent) {
        setMouseTracking(true);
    }

    void setImage(const QImage& img) {
        m_image = img;
        resetCorners();
        update();
    }

    QImage getImage() const { return m_image; }

    void setEraserActive(bool active) {
        m_eraserActive = active;
        update();
    }

    void setBrushSize(int size) {
        m_brushSize = size;
    }

    void resetCorners() {
        if (m_image.isNull()) return;
        m_corners.clear();
        m_corners << QPointF(0, 0)
                  << QPointF(m_image.width(), 0)
                  << QPointF(m_image.width(), m_image.height())
                  << QPointF(0, m_image.height());
        update();
    }

    void rotateImage() {
        if (m_image.isNull()) return;
        saveUndo();
        m_image = ImageEffects::rotateImage(m_image, 90);
        resetCorners();
    }

    void processWarp() {
        if (m_image.isNull() || m_corners.size() != 4) return;
        saveUndo();
        m_image = ImageEffects::perspectiveWarp(m_image, m_corners);
        resetCorners();
    }

    void undo() {
        if (!m_undoStack.isEmpty()) {
            m_image = m_undoStack.pop();
            resetCorners();
        }
    }

    bool canUndo() const { return !m_undoStack.isEmpty(); }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.fillRect(rect(), QColor(15, 23, 42));

        if (m_image.isNull()) return;

        double scaleX = static_cast<double>(width() - 40) / m_image.width();
        double scaleY = static_cast<double>(height() - 40) / m_image.height();
        m_scale = std::min({scaleX, scaleY, 1.0});

        int dispW = static_cast<int>(m_image.width() * m_scale);
        int dispH = static_cast<int>(m_image.height() * m_scale);
        m_offsetX = (width() - dispW) / 2;
        m_offsetY = (height() - dispH) / 2;

        QRect targetRect(m_offsetX, m_offsetY, dispW, dispH);
        p.drawImage(targetRect, m_image);

        // Draw quad polygon
        if (m_corners.size() == 4) {
            QPolygonF viewPoly;
            for (const auto& pt : m_corners) {
                viewPoly << toView(pt);
            }

            p.setPen(QPen(QColor(56, 189, 248, m_eraserActive ? 70 : 220), 2));
            p.setBrush(QColor(56, 189, 248, m_eraserActive ? 15 : 45));
            p.drawPolygon(viewPoly);

            // Draw corner pins
            for (int i = 0; i < 4; ++i) {
                QPointF vp = viewPoly[i];
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(i == m_activeCorner ? QColor(239, 68, 68) : QColor(2, 132, 199));
                p.drawEllipse(vp, 8, 8);
                p.setBrush(Qt::white);
                p.drawEllipse(vp, 3, 3);
            }
        }

        // Draw Magnifying Loupe
        if (m_activeCorner >= 0 && m_activeCorner < m_corners.size()) {
            drawLoupe(p, m_corners[m_activeCorner]);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (m_image.isNull()) return;

        if (m_eraserActive) {
            saveUndo();
            m_isErasing = true;
            m_lastErasePos = toImage(event->position());
            eraseStroke(m_lastErasePos, m_lastErasePos);
            update();
            return;
        }

        // Check corner click
        m_activeCorner = -1;
        for (int i = 0; i < m_corners.size(); ++i) {
            QPointF vp = toView(m_corners[i]);
            double dist = std::hypot(vp.x() - event->position().x(), vp.y() - event->position().y());
            if (dist < 24) {
                m_activeCorner = i;
                update();
                break;
            }
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_image.isNull()) return;

        if (m_eraserActive && m_isErasing) {
            QPointF cur = toImage(event->position());
            eraseStroke(m_lastErasePos, cur);
            m_lastErasePos = cur;
            update();
            return;
        }

        if (m_activeCorner >= 0 && m_activeCorner < m_corners.size()) {
            QPointF imgPos = toImage(event->position());
            imgPos.setX(std::clamp(imgPos.x(), 0.0, static_cast<double>(m_image.width())));
            imgPos.setY(std::clamp(imgPos.y(), 0.0, static_cast<double>(m_image.height())));
            m_corners[m_activeCorner] = imgPos;
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        Q_UNUSED(event);
        m_isErasing = false;
        m_activeCorner = -1;
        update();
    }

private:
    QPointF toView(const QPointF& imgPt) const {
        return QPointF(imgPt.x() * m_scale + m_offsetX, imgPt.y() * m_scale + m_offsetY);
    }

    QPointF toImage(const QPointF& viewPt) const {
        if (m_scale <= 0) return QPointF(0, 0);
        return QPointF((viewPt.x() - m_offsetX) / m_scale, (viewPt.y() - m_offsetY) / m_scale);
    }

    void saveUndo() {
        if (m_undoStack.size() > 10) m_undoStack.removeFirst();
        m_undoStack.push(m_image);
    }

    void eraseStroke(const QPointF& p1, const QPointF& p2) {
        if (m_image.isNull()) return;
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing, true);
        double brushWidth = m_brushSize / m_scale;
        p.setPen(QPen(Qt::white, brushWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(p1, p2);
    }

    void drawLoupe(QPainter& p, const QPointF& targetImgPt) {
        const int loupeSize = 130;
        int loupeX = (toView(targetImgPt).x() < width() / 2) ? (width() - loupeSize - 20) : 20;
        int loupeY = 20;

        QRect loupeRect(loupeX, loupeY, loupeSize, loupeSize);

        p.save();
        p.setClipRect(loupeRect);
        p.fillRect(loupeRect, Qt::black);

        double zoom = 2.5;
        double srcW = loupeSize / zoom;
        double srcH = loupeSize / zoom;
        QRectF srcCrop(targetImgPt.x() - srcW / 2, targetImgPt.y() - srcH / 2, srcW, srcH);
        p.drawImage(loupeRect, m_image, srcCrop);

        // Crosshair
        p.setPen(QPen(QColor(56, 189, 248), 1));
        p.drawLine(loupeX + loupeSize / 2, loupeY, loupeX + loupeSize / 2, loupeY + loupeSize);
        p.drawLine(loupeX, loupeY + loupeSize / 2, loupeX + loupeSize, loupeY + loupeSize / 2);

        p.restore();

        // Loupe frame
        p.setPen(QPen(QColor(56, 189, 248), 2));
        p.drawRect(loupeRect);
    }

    QImage m_image;
    QPolygonF m_corners;
    int m_activeCorner = -1;
    double m_scale = 1.0;
    int m_offsetX = 0;
    int m_offsetY = 0;

    bool m_eraserActive = false;
    bool m_isErasing = false;
    int m_brushSize = 15;
    QPointF m_lastErasePos;
    QStack<QImage> m_undoStack;
};

EditorDialog::EditorDialog(const QImage& sourceImage, const QString& filter, QWidget* parent)
    : QDialog(parent), m_cleanOriginalImage(sourceImage), m_workingImage(sourceImage), m_currentFilter(filter) {
    setupUi();
}

void EditorDialog::setupUi() {
    setWindowTitle("Sayfa Düzenle & CamScanner İnce Ayar");
    resize(1100, 800);
    setStyleSheet("background-color: #0f172a; color: #f8fafc; font-family: system-ui;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Top Toolbar: Warp, Rotate, Corners, Filters, Eraser
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    auto makeBtn = [](const QString& text, const QString& color = "#1e293b", const QString& textCol = "#f8fafc") {
        auto* btn = new QPushButton(text);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  border: 1px solid #334155;"
            "  color: %2;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  border-color: #38bdf8;"
            "}"
        ).arg(color, textCol));
        return btn;
    };

    auto* btnReset = makeBtn("🎯 Köşeleri Sıfırla (%100)");
    connect(btnReset, &QPushButton::clicked, this, &EditorDialog::onResetCorners);
    topBar->addWidget(btnReset);

    auto* btnRotate = makeBtn("↻ 90°");
    connect(btnRotate, &QPushButton::clicked, this, &EditorDialog::onRotateSource);
    topBar->addWidget(btnRotate);

    auto* btnWarp = makeBtn("✨ Perspektifi Düzelt (Warp)", "#0284c7");
    connect(btnWarp, &QPushButton::clicked, this, &EditorDialog::onProcessWarp);
    topBar->addWidget(btnWarp);

    topBar->addSpacing(12);

    // Eraser Group
    m_btnEraser = makeBtn("🧹 Leke Silici: Kapalı");
    m_btnEraser->setCheckable(true);
    connect(m_btnEraser, &QPushButton::toggled, this, &EditorDialog::onToggleEraser);
    topBar->addWidget(m_btnEraser);

    m_btnBrush15 = makeBtn("15px");
    connect(m_btnBrush15, &QPushButton::clicked, this, [this]() { onSetBrushSize(15); });
    topBar->addWidget(m_btnBrush15);

    m_btnBrush30 = makeBtn("30px");
    connect(m_btnBrush30, &QPushButton::clicked, this, [this]() { onSetBrushSize(30); });
    topBar->addWidget(m_btnBrush30);

    m_btnBrush50 = makeBtn("50px");
    connect(m_btnBrush50, &QPushButton::clicked, this, [this]() { onSetBrushSize(50); });
    topBar->addWidget(m_btnBrush50);

    auto* btnUndo = makeBtn("↩️ Geri Al");
    connect(btnUndo, &QPushButton::clicked, this, &EditorDialog::onUndo);
    topBar->addWidget(btnUndo);

    topBar->addStretch(1);

    // Filter Buttons
    auto* btnMagic = makeBtn("✨ Sihirli");
    connect(btnMagic, &QPushButton::clicked, this, [this]() { onApplyFilter("magic"); });
    topBar->addWidget(btnMagic);

    auto* btnBw = makeBtn("📄 S/B");
    connect(btnBw, &QPushButton::clicked, this, [this]() { onApplyFilter("bw"); });
    topBar->addWidget(btnBw);

    auto* btnOrig = makeBtn("🌈 Orijinal");
    connect(btnOrig, &QPushButton::clicked, this, [this]() { onApplyFilter("orig"); });
    topBar->addWidget(btnOrig);

    mainLayout->addLayout(topBar);

    // Center Workspace
    m_canvas = new EditorCanvas(this);
    m_canvas->setImage(m_workingImage);
    mainLayout->addWidget(m_canvas, 1);

    // Bottom Action Bar: Cancel / Save
    auto* bottomBar = new QHBoxLayout();
    auto* hintLabel = new QLabel("💡 Köşeleri sürükleyerek perspektifi düzeltin veya leke siliciyle istenmeyen gölgeleri silin.", this);
    hintLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");
    bottomBar->addWidget(hintLabel, 1);

    auto* btnCancel = makeBtn("Vazgeç");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    bottomBar->addWidget(btnCancel);

    auto* btnSave = makeBtn("✓ Değişiklikleri Uygula", "#059669");
    connect(btnSave, &QPushButton::clicked, this, &EditorDialog::onApplyAndClose);
    bottomBar->addWidget(btnSave);

    mainLayout->addLayout(bottomBar);
}

void EditorDialog::onResetCorners() {
    if (m_canvas) m_canvas->resetCorners();
}

void EditorDialog::onRotateSource() {
    if (m_canvas) m_canvas->rotateImage();
}

void EditorDialog::onProcessWarp() {
    if (m_canvas) m_canvas->processWarp();
}

void EditorDialog::onToggleEraser(bool checked) {
    if (m_btnEraser) {
        m_btnEraser->setText(checked ? "🧹 Leke Silici: AÇIK" : "🧹 Leke Silici: Kapalı");
        m_btnEraser->setStyleSheet(checked ? "background-color: #38bdf8; color: #0f172a; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
                                           : "background-color: #1e293b; color: #f8fafc; font-weight: bold; border-radius: 6px; padding: 6px 12px;");
    }
    if (m_canvas) m_canvas->setEraserActive(checked);
}

void EditorDialog::onSetBrushSize(int size) {
    if (m_canvas) m_canvas->setBrushSize(size);
}

void EditorDialog::onUndo() {
    if (m_canvas) m_canvas->undo();
}

void EditorDialog::onApplyFilter(const QString& filter) {
    m_currentFilter = filter;
    if (m_canvas) {
        QImage current = m_canvas->getImage();
        if (filter == "magic") {
            ImageEffects::applyMagicColor(current);
        } else if (filter == "bw") {
            ImageEffects::applyBwThreshold(current, 140);
        } else if (filter == "orig") {
            current = m_cleanOriginalImage;
        }
        m_canvas->setImage(current);
    }
}

void EditorDialog::onApplyAndClose() {
    if (m_canvas) {
        m_workingImage = m_canvas->getImage();
    }
    accept();
}
