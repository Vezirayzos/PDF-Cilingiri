#include "PageCard.h"
#include "ImageEffects.h"
#include "MemoryCache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QPixmap>

PageCard::PageCard(const QString& filePath, int pageIndex, const QImage& thumbnail, QWidget* parent)
    : QFrame(parent), m_filePath(filePath), m_pageIndex(pageIndex), m_cleanThumbnail(thumbnail) {
    setupUi();
    updateThumbnail();
}

QString PageCard::getFileName() const {
    return QFileInfo(m_filePath).fileName();
}

bool PageCard::isSelected() const {
    return m_checkBox && m_checkBox->isChecked();
}

void PageCard::setSelected(bool sel) {
    if (m_checkBox && m_checkBox->isChecked() != sel) {
        m_checkBox->setChecked(sel);
    }
}

void PageCard::setRotation(int deg) {
    m_rotation = (deg % 360 + 360) % 360;
    updateThumbnail();
    updateAccessibilityInfo();
}

void PageCard::setFilter(const QString& filter) {
    m_filter = filter;
    updateThumbnail();
    updateAccessibilityInfo();
}

void PageCard::setCardNumber(int num) {
    m_cardNumber = num;
    if (m_numLabel) {
        m_numLabel->setText(QString("#%1").arg(m_cardNumber));
    }
    updateAccessibilityInfo();
}

void PageCard::setModifiedImage(const QImage& img) {
    // Put modified high-res image into MemoryCache
    MemoryCache::instance().putFullImage(m_filePath, m_pageIndex, img);
    m_cleanThumbnail = img.scaled(180, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_hasModifiedImage = true;
    updateThumbnail();
}

QImage PageCard::getFullImage() const {
    // Fetches full resolution on-demand via LRU MemoryCache
    QImage result = MemoryCache::instance().getFullImage(m_filePath, m_pageIndex);
    if (result.isNull()) {
        result = m_cleanThumbnail;
    }

    if (m_rotation != 0) {
        result = ImageEffects::rotateImage(result, m_rotation);
    }
    if (m_filter == "magic") {
        ImageEffects::applyMagicColor(result);
    } else if (m_filter == "bw") {
        ImageEffects::applyBwThreshold(result, 140);
    } else if (m_filter == "ccitt") {
        ImageEffects::applyFax300Dpi(result);
    }
    return result;
}

void PageCard::updateThumbnail() {
    QImage processed = m_cleanThumbnail;
    if (m_rotation != 0) {
        processed = ImageEffects::rotateImage(processed, m_rotation);
    }
    if (m_filter == "magic") {
        ImageEffects::applyMagicColor(processed);
    } else if (m_filter == "bw") {
        ImageEffects::applyBwThreshold(processed, 140);
    } else if (m_filter == "ccitt") {
        ImageEffects::applyFax300Dpi(processed);
    }

    if (!processed.isNull()) {
        QPixmap pix = QPixmap::fromImage(processed.scaled(180, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_thumbLabel->setPixmap(pix);
    }
    updateBadgeText();
}

void PageCard::updateBadgeText() {
    QString badges;
    if (m_filter == "magic") badges += "✨ Sihirli ";
    if (m_filter == "bw") badges += "📄 S/B ";
    if (m_filter == "ccitt") badges += "🏛️ 300 DPI ";
    if (m_rotation != 0) badges += QString("↻ %1° ").arg(m_rotation);

    if (m_badgeLabel) {
        m_badgeLabel->setText(badges.trimmed());
        m_badgeLabel->setVisible(!badges.isEmpty());
    }
}

void PageCard::updateAccessibilityInfo() {
    setAccessibleName(QString("%1. Sayfa: %2").arg(m_cardNumber).arg(getFileName()));
    setAccessibleDescription(QString("Seçili: %1, Filtre: %2, Döndürme: %3 derece. Seçimi değiştirmek için Boşluk, düzenlemek için F2 veya Enter tuşuna basın.")
                             .arg(isSelected() ? "Evet" : "Hayır")
                             .arg(m_filter)
                             .arg(m_rotation));
}

void PageCard::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        setSelected(!isSelected());
        event->accept();
    } else if (event->key() == Qt::Key_F2 || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit editRequested(this);
        event->accept();
    } else if (event->key() == Qt::Key_Delete) {
        emit deleteRequested(this);
        event->accept();
    } else if (event->key() == Qt::Key_R) {
        emit rotateRequested(this);
        event->accept();
    } else {
        QFrame::keyPressEvent(event);
    }
}

void PageCard::setupUi() {
    setObjectName("pageCard");
    setFixedWidth(200);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(
        "#pageCard {"
        "  background-color: #0f172a;"
        "  border: 2px solid #334155;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
        "#pageCard:hover, #pageCard:focus {"
        "  border-color: #38bdf8;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    // Top Header: Checkbox + Page Number + File Name
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    m_checkBox = new QCheckBox(this);
    m_checkBox->setChecked(true);
    m_checkBox->setAccessibleName("Sayfa Seçimi");
    connect(m_checkBox, &QCheckBox::toggled, this, [this]() {
        updateAccessibilityInfo();
        emit selectionChanged(this);
    });
    headerLayout->addWidget(m_checkBox);

    m_numLabel = new QLabel(QString("#%1").arg(m_cardNumber), this);
    m_numLabel->setStyleSheet("font-weight: bold; color: #38bdf8;");
    headerLayout->addWidget(m_numLabel);

    m_nameLabel = new QLabel(getFileName(), this);
    m_nameLabel->setStyleSheet("color: #94a3b8; font-size: 11px;");
    m_nameLabel->setToolTip(m_filePath);
    headerLayout->addWidget(m_nameLabel, 1);

    mainLayout->addLayout(headerLayout);

    // Badges
    m_badgeLabel = new QLabel(this);
    m_badgeLabel->setStyleSheet("background: rgba(56, 189, 248, 0.15); color: #38bdf8; padding: 2px 4px; border-radius: 4px; font-size: 10px; font-weight: bold;");
    m_badgeLabel->setVisible(false);
    mainLayout->addWidget(m_badgeLabel);

    // Thumbnail
    m_thumbLabel = new QLabel(this);
    m_thumbLabel->setAlignment(Qt::AlignCenter);
    m_thumbLabel->setFixedSize(188, 220);
    m_thumbLabel->setStyleSheet("background-color: #020617; border-radius: 4px; border: 1px solid #1e293b;");
    mainLayout->addWidget(m_thumbLabel);

    // Bottom Action Buttons
    auto* footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(0, 4, 0, 0);
    footerLayout->setSpacing(2);

    auto makeBtn = [](const QString& text, const QString& tip, const QString& a11y) {
        auto* btn = new QPushButton(text);
        btn->setToolTip(tip);
        btn->setAccessibleName(a11y);
        btn->setFixedSize(32, 28);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #1e293b;"
            "  border: 1px solid #334155;"
            "  color: #f8fafc;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover, QPushButton:focus {"
            "  background-color: #38bdf8;"
            "  color: #0f172a;"
            "}"
        );
        return btn;
    };

    m_btnLeft = makeBtn("◀", "Sola Taşı", "Sayfayı sola taşı");
    connect(m_btnLeft, &QPushButton::clicked, this, [this]() { emit moveRequested(this, -1); });
    footerLayout->addWidget(m_btnLeft);

    m_btnRight = makeBtn("▶", "Sağa Taşı", "Sayfayı sağa taşı");
    connect(m_btnRight, &QPushButton::clicked, this, [this]() { emit moveRequested(this, 1); });
    footerLayout->addWidget(m_btnRight);

    m_btnRotate = makeBtn("↻", "90° Döndür (R)", "Sayfayı 90 derece döndür");
    connect(m_btnRotate, &QPushButton::clicked, this, [this]() { emit rotateRequested(this); });
    footerLayout->addWidget(m_btnRotate);

    m_btnEdit = makeBtn("✏️", "İnce Ayar & CamScanner Düzenle (F2)", "Sayfayı düzenle");
    connect(m_btnEdit, &QPushButton::clicked, this, [this]() { emit editRequested(this); });
    footerLayout->addWidget(m_btnEdit);

    m_btnDelete = makeBtn("🗑️", "Sayfayı Sil (Delete)", "Sayfayı kaldır");
    m_btnDelete->setStyleSheet(
        "QPushButton {"
        "  background-color: #1e293b;"
        "  border: 1px solid #334155;"
        "  color: #ef4444;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover, QPushButton:focus {"
        "  background-color: #ef4444;"
        "  color: #ffffff;"
        "}"
    );
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() { emit deleteRequested(this); });
    footerLayout->addWidget(m_btnDelete);

    mainLayout->addLayout(footerLayout);
    updateAccessibilityInfo();
}
