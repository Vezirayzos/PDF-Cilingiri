#pragma once

#include <QDialog>
#include <QImage>
#include <QPolygonF>
#include <QStack>
#include <QPushButton>

class EditorCanvas;

class EditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditorDialog(const QImage& sourceImage, const QString& filter, QWidget* parent = nullptr);

    QImage getResultImage() const { return m_workingImage; }
    QString getResultFilter() const { return m_currentFilter; }

private slots:
    void onResetCorners();
    void onRotateSource();
    void onProcessWarp();
    void onToggleEraser(bool checked);
    void onSetBrushSize(int size);
    void onUndo();
    void onApplyFilter(const QString& filter);
    void onApplyAndClose();

private:
    void setupUi();

    QImage m_cleanOriginalImage;
    QImage m_workingImage;
    QString m_currentFilter = "orig";

    EditorCanvas* m_canvas = nullptr;
    QPushButton* m_btnEraser = nullptr;
    QPushButton* m_btnBrush15 = nullptr;
    QPushButton* m_btnBrush30 = nullptr;
    QPushButton* m_btnBrush50 = nullptr;
};
