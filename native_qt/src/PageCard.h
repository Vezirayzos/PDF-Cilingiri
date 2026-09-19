#pragma once

#include <QFrame>
#include <QImage>
#include <QString>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QKeyEvent>

class PageCard : public QFrame {
    Q_OBJECT

public:
    explicit PageCard(const QString& filePath, int pageIndex, const QImage& thumbnail, QWidget* parent = nullptr);

    bool isSelected() const;
    void setSelected(bool sel);

    int getRotation() const { return m_rotation; }
    void setRotation(int deg);

    QString getFilter() const { return m_filter; }
    void setFilter(const QString& filter);

    int getPageIndex() const { return m_pageIndex; }
    QString getFilePath() const { return m_filePath; }
    QString getFileName() const;

    // Fetches full resolution on-demand via LRU MemoryCache
    QImage getFullImage() const;
    void setModifiedImage(const QImage& img);

    void updateThumbnail();
    void setCardNumber(int num);

signals:
    void selectionChanged(PageCard* card);
    void moveRequested(PageCard* card, int direction);
    void deleteRequested(PageCard* card);
    void editRequested(PageCard* card);
    void rotateRequested(PageCard* card);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void updateBadgeText();
    void updateAccessibilityInfo();

    QString m_filePath;
    int m_pageIndex;
    int m_cardNumber = 1;
    int m_rotation = 0;
    QString m_filter = "none";

    QImage m_cleanThumbnail;
    bool m_hasModifiedImage = false;

    QCheckBox* m_checkBox = nullptr;
    QLabel* m_numLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_thumbLabel = nullptr;
    QLabel* m_badgeLabel = nullptr;
    QPushButton* m_btnLeft = nullptr;
    QPushButton* m_btnRight = nullptr;
    QPushButton* m_btnRotate = nullptr;
    QPushButton* m_btnEdit = nullptr;
    QPushButton* m_btnDelete = nullptr;
};
