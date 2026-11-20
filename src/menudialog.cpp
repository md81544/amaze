#include "menudialog.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace marengo {
namespace amaze {

namespace {

// Green on black to match the in-game status display (see view.cpp).
// %1 is the font size in points, %2 the font family.
const char* const STYLE_SHEET = R"(
QDialog {
    background-color: black;
}
QWidget {
    font-size: %1pt;
    font-family: "%2";
}
QFrame#border {
    border: 3px solid rgb(0, 224, 0);
    border-radius: 6px;
}
QLabel {
    color: rgb(0, 224, 0);
}
QTreeWidget {
    background-color: black;
    color: rgb(0, 224, 0);
    border: 1px solid rgb(0, 128, 0);
    outline: none;
}
QTreeWidget::item:hover {
    background-color: rgb(0, 40, 0);
}
QTreeWidget::item:selected {
    background-color: rgb(0, 80, 0);
    color: rgb(0, 255, 0);
}
QPushButton {
    background-color: black;
    color: rgb(0, 224, 0);
    border: 1px solid rgb(0, 128, 0);
    border-radius: 3px;
    padding: 4px 16px;
}
QPushButton:hover {
    background-color: rgb(0, 40, 0);
}
QPushButton:pressed {
    background-color: rgb(0, 80, 0);
}
QPushButton:default, QPushButton:focus {
    border-color: rgb(0, 255, 0);
}
QSlider::groove:horizontal {
    height: 4px;
    background: rgb(0, 50, 0);
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: rgb(0, 128, 0);
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: rgb(0, 192, 0);
    width: 14px;
    margin: -6px 0;
    border-radius: 7px;
}
QSlider::handle:horizontal:hover, QSlider::handle:horizontal:focus {
    background: rgb(0, 255, 0);
}
QScrollBar:vertical {
    background: black;
    width: 10px;
}
QScrollBar::handle:vertical {
    background: rgb(0, 80, 0);
    border-radius: 4px;
    min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}
)";

QSlider* makeSlider(int min, int max, int value)
{
    auto* slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    return slider;
}

} // namespace

MenuDialog::MenuDialog(
    const std::string& dataDir,
    const MenuResults& initialValues,
    QWidget* parent)
    : QDialog(parent)
    , m_results(initialValues)
    , m_levelFileLister(dataDir)
    , m_levelList(new QTreeWidget)
{
    setWindowTitle("Amaze");
    setModal(true);
    // No title bar: the OS's light-mode one clashes with the game. The
    // green border drawn by the "border" frame below marks the edges instead.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    const double fontSize = QApplication::font().pointSizeF() * 1.5;
    // The game's font. The dialog may be created before the View has loaded
    // it, so load it here too; if that fails, Qt falls back to the default.
    QString fontFamily;
    int fontId = QFontDatabase::addApplicationFont(
        QString::fromStdString(dataDir + "/Oxanium-SemiBold.ttf"));
    if (fontId >= 0) {
        fontFamily = QFontDatabase::applicationFontFamilies(fontId).value(0);
    }
    setStyleSheet(QString(STYLE_SHEET).arg(fontSize).arg(fontFamily));

    // Options
    auto* musicVolume = makeSlider(0, 100, m_results.musicVolumePercent);
    connect(musicVolume, &QSlider::valueChanged, this, [this](int value) {
        m_results.musicVolumePercent = value;
    });
    auto* deadZone = makeSlider(0, 25, m_results.stickDeadZonePercent);
    connect(deadZone, &QSlider::valueChanged, this, [this](int value) {
        m_results.stickDeadZonePercent = value;
    });
    auto* options = new QFormLayout;
    options->addRow("Background music volume", musicVolume);
    options->addRow("Stick dead zone", deadZone);

    // Level selection
    // Number and name are separate columns, with the numbers right-aligned,
    // so the names line up however many digits the numbers have
    m_levelList->setColumnCount(2);
    m_levelList->setHeaderHidden(true);
    m_levelList->setRootIsDecorated(false);
    m_levelList->setIndentation(0);
    m_levelList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_levelList->header()->setStretchLastSection(true);
    for (const auto& [number, fileData] : m_levelFileLister.getFileMap()) {
        auto* item = new QTreeWidgetItem(
            { QString::number(number), QString::fromStdString(fileData.description) });
        item->setTextAlignment(0, Qt::AlignRight | Qt::AlignVCenter);
        item->setData(0, Qt::UserRole, QString::fromStdString(fileData.filename));
        m_levelList->addTopLevelItem(item);
    }
    connect(m_levelList, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        if (item) {
            m_results.levelFileToLoad = item->data(0, Qt::UserRole).toString().toStdString();
        }
    });
    // Double-click or Enter on a level acts as OK
    connect(m_levelList, &QTreeWidget::itemActivated, this, &QDialog::accept);

    // Buttons
    auto* ok = new QPushButton("OK");
    ok->setDefault(true);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    auto* cancel = new QPushButton("Cancel");
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    auto* exitGame = new QPushButton("Exit Game");
    connect(exitGame, &QPushButton::clicked, this, [this]() { done(ExitGame); });
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    buttons->addStretch();
    buttons->addWidget(exitGame);

    auto* border = new QFrame;
    border->setObjectName("border");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(border);

    auto* layout = new QVBoxLayout(border);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->addLayout(options);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Level selection"));
    layout->addWidget(m_levelList, 1);
    layout->addLayout(buttons);

    resize(840, 690);
    m_levelList->setFocus();
}

} // namespace amaze
} // namespace marengo
