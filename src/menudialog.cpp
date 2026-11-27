#include "menudialog.h"
#include "fonts.h"
#include "levelprogress.h"

#include <QApplication>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <filesystem>
#include <initializer_list>
#include <iterator>

namespace marengo {
namespace amaze {

namespace {

// Green on black to match the in-game status display (see view.cpp).
// (The font isn't set here, as its weight can't be (see fonts.h), and any font
// setting here would replace the font set on the dialog.)
const char* const STYLE_SHEET = R"(
QDialog {
    background-color: black;
}
QFrame#border {
    border: 3px solid rgb(0, 224, 0);
    border-radius: 6px;
}
QLabel {
    color: rgb(0, 224, 0);
}
QLabel#optionLabel {
    padding: 2px 6px;
    border-radius: 3px;
}
QLabel#optionLabel[focused="true"] {
    background-color: rgb(0, 60, 0);
    color: rgb(0, 255, 0);
}
QTreeWidget {
    background-color: black;
    color: rgb(0, 224, 0);
    border: 2px solid rgb(0, 80, 0);
    outline: none;
}
QTreeWidget:focus {
    border-color: rgb(0, 255, 0);
}
QTreeWidget::item:hover {
    background-color: rgb(0, 40, 0);
}
QTreeWidget::item:disabled {
    color: rgb(0, 80, 0);
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
QPushButton:default {
    border-color: rgb(0, 192, 0);
}
QPushButton:focus {
    background-color: rgb(0, 60, 0);
    color: rgb(0, 255, 0);
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

QSlider* makeSlider(int min, int max, int step, int value)
{
    auto* slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setSingleStep(step);
    slider->setValue(value);
    slider->setFocusPolicy(Qt::StrongFocus);
    return slider;
}

QLabel* makeOptionLabel(const char* text)
{
    auto* label = new QLabel(text);
    label->setObjectName("optionLabel");
    return label;
}

bool isKey(const QKeyEvent* event, std::initializer_list<int> keys)
{
    for (int key : keys) {
        if (event->key() == key) {
            return true;
        }
    }
    return false;
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
    , m_musicVolume(makeSlider(0, 100, 5, initialValues.musicVolumePercent))
    , m_deadZone(makeSlider(MIN_STICK_DEAD_ZONE_PERCENT,
          MAX_STICK_DEAD_ZONE_PERCENT,
          1,
          initialValues.stickDeadZonePercent))
    , m_rumble(makeSlider(0, 100, 5, initialValues.rumbleIntensityPercent))
    , m_musicVolumeLabel(makeOptionLabel("Background music volume"))
    , m_deadZoneLabel(makeOptionLabel("Stick dead zone"))
    , m_rumbleLabel(makeOptionLabel("Rumble intensity"))
    , m_ok(new QPushButton("OK"))
    , m_cancel(new QPushButton("Cancel"))
    , m_exitGame(new QPushButton("Exit Game"))
{
    setWindowTitle("Amaze");
    setModal(true);
    // No title bar: the OS's light-mode one clashes with the game. The
    // green border drawn by the "border" frame below marks the edges instead.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // Options
    connect(m_musicVolume, &QSlider::valueChanged, this, [this](int value) {
        m_results.musicVolumePercent = value;
    });
    connect(m_deadZone, &QSlider::valueChanged, this, [this](int value) {
        m_results.stickDeadZonePercent = value;
    });
    connect(m_rumble, &QSlider::valueChanged, this, [this](int value) {
        m_results.rumbleIntensityPercent = value;
    });
    auto* options = new QFormLayout;
    options->addRow(m_musicVolumeLabel, m_musicVolume);
    options->addRow(m_deadZoneLabel, m_deadZone);
    options->addRow(m_rumbleLabel, m_rumble);

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
    m_ok->setDefault(true);
    connect(m_ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_exitGame, &QPushButton::clicked, this, [this]() { done(ExitGame); });
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_ok);
    buttons->addWidget(m_cancel);
    buttons->addStretch();
    buttons->addWidget(m_exitGame);
    for (QWidget* widget : std::initializer_list<QWidget*> {
             m_musicVolume, m_deadZone, m_rumble, m_levelList, m_ok, m_cancel, m_exitGame }) {
        widget->setFocusPolicy(Qt::StrongFocus);
        widget->installEventFilter(this);
    }
    connect(qApp, &QApplication::focusChanged, this, [this]() { updateFocusHighlight(); });

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

    // The game's font. The dialog may be created before the View has loaded
    // it, so load it here too; if that fails, Qt falls back to the default.
    // It's set on each widget, as on macOS, Qt gives some kinds of widget
    // (e.g. labels and buttons) their own font, rather than the dialog's.
    QFont font = fonts::load(dataDir);
    font.setPointSizeF(QApplication::font().pointSizeF() * 1.5);
    setFont(font);
    for (QWidget* widget : findChildren<QWidget*>()) {
        widget->setFont(font);
    }
    setStyleSheet(STYLE_SHEET);

    resize(840, 690);
    m_levelList->setFocus();
}

bool MenuDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress) {
        return QDialog::eventFilter(watched, event);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->modifiers() & ~Qt::KeypadModifier) {
        return QDialog::eventFilter(watched, event);
    }
    const bool up = keyEvent->key() == Qt::Key_Up;
    const bool down = keyEvent->key() == Qt::Key_Down;
    const bool leftRight = isKey(keyEvent, { Qt::Key_Left, Qt::Key_Right });
    const bool enter = isKey(keyEvent, { Qt::Key_Return, Qt::Key_Enter });

    // Page Up/Down (the gamepad's shoulder buttons) always scroll the level list
    if (watched != m_levelList && isKey(keyEvent, { Qt::Key_PageUp, Qt::Key_PageDown })) {
        m_levelList->setFocus();
        QCoreApplication::sendEvent(m_levelList, event);
        return true;
    }

    const auto sliderList = sliders();
    auto slider = std::find(sliderList.begin(), sliderList.end(), watched);
    if (slider != sliderList.end()) {
        // Up/Down would otherwise change the value; Left/Right still do
        if (up && slider != sliderList.begin()) {
            (*std::prev(slider))->setFocus();
        } else if (down) {
            if (std::next(slider) != sliderList.end()) {
                (*std::next(slider))->setFocus();
            } else {
                m_levelList->setFocus();
            }
        } else if (up) {
            // Nothing above the top slider
        } else if (enter) {
            accept();
        } else {
            return QDialog::eventFilter(watched, event);
        }
        return true;
    }

    if (watched == m_levelList) {
        // (Levels which can't be chosen yet are skipped)
        int row = m_levelList->currentIndex().row();
        if (up && row <= firstUnlockedRow()) {
            sliders().back()->setFocus();
        } else if (down && row >= lastUnlockedRow()) {
            m_ok->setFocus();
        } else if (keyEvent->key() == Qt::Key_Left) {
            // Shortcuts out of the list without scrolling to either end
            sliders().back()->setFocus();
        } else if (keyEvent->key() == Qt::Key_Right) {
            m_ok->setFocus();
        } else {
            return QDialog::eventFilter(watched, event);
        }
        return true;
    }

    const std::array<QPushButton*, 3> buttons { m_ok, m_cancel, m_exitGame };
    auto it = std::find(buttons.begin(), buttons.end(), watched);
    if (it != buttons.end()) {
        if (up) {
            m_levelList->setFocus();
        } else if (keyEvent->key() == Qt::Key_Left && it != buttons.begin()) {
            (*std::prev(it))->setFocus();
        } else if (keyEvent->key() == Qt::Key_Right && std::next(it) != buttons.end()) {
            (*std::next(it))->setFocus();
        } else if (enter) {
            // Don't rely on the platform pressing the focused button on Enter
            (*it)->click();
        } else if (down || leftRight) {
            // Nothing below or beyond the ends; no wrap-around, so it's hard
            // to overshoot onto Exit Game
        } else {
            return QDialog::eventFilter(watched, event);
        }
        return true;
    }

    return QDialog::eventFilter(watched, event);
}

void MenuDialog::updateLevels(const LevelProgress& progress, const std::string& currentLevelFile)
{
    const std::string current = std::filesystem::path(currentLevelFile).filename().string();
    for (int row = 0; row < m_levelList->topLevelItemCount(); ++row) {
        QTreeWidgetItem* item = m_levelList->topLevelItem(row);
        const std::string level
            = std::filesystem::path(item->data(0, Qt::UserRole).toString().toStdString())
                  .filename()
                  .string();
        // Disabled items are drawn dimmed, and can't be selected
        item->setDisabled(!progress.isUnlocked(level));
        if (level == current) {
            m_levelList->setCurrentItem(item);
        }
    }
}

int MenuDialog::firstUnlockedRow() const
{
    for (int row = 0; row < m_levelList->topLevelItemCount(); ++row) {
        if (!m_levelList->topLevelItem(row)->isDisabled()) {
            return row;
        }
    }
    return 0;
}

int MenuDialog::lastUnlockedRow() const
{
    for (int row = m_levelList->topLevelItemCount() - 1; row >= 0; --row) {
        if (!m_levelList->topLevelItem(row)->isDisabled()) {
            return row;
        }
    }
    return 0;
}

bool MenuDialog::focusNextPrevChild(bool next)
{
    const std::array<QWidget*, 7> order {
        m_musicVolume, m_deadZone, m_rumble, m_levelList, m_ok, m_cancel, m_exitGame
    };
    auto it = std::find(order.begin(), order.end(), focusWidget());
    int index = it == order.end() ? 0 : static_cast<int>(it - order.begin());
    int count = static_cast<int>(order.size());
    index = (index + (next ? 1 : count - 1)) % count;
    order[index]->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
    return true;
}

void MenuDialog::updateFocusHighlight()
{
    // Light up the label of the slider that has focus, so it's clear which
    // row the arrow keys / gamepad are on
    for (auto [label, slider] : { std::pair { m_musicVolumeLabel, m_musicVolume },
                                  std::pair { m_deadZoneLabel, m_deadZone },
                                  std::pair { m_rumbleLabel, m_rumble } }) {
        bool focused = slider->hasFocus();
        if (label->property("focused").toBool() != focused) {
            label->setProperty("focused", focused);
            label->style()->unpolish(label);
            label->style()->polish(label);
        }
    }
}

} // namespace amaze
} // namespace marengo
