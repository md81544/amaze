#include "menudialog.h"
#include "fonts.h"
#include "levelprogress.h"
#include "palette.h"

#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QStyle>
#include <QTabBar>
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

// Styled like the in-game ship status panel: see palette.h. The "@" names are
// replaced by its colours.
// (The font isn't set here, as its weight can't be (see fonts.h), and any font
// setting here would replace the font set on the dialog.)
const char* const STYLE_SHEET = R"(
QDialog {
    background-color: black;
}
QFrame#border {
    background-color: @background;
    border: 2px solid @status;
    border-radius: 10px;
}
QFrame#bezel {
    border: 1px solid @bezel;
    border-radius: 5px;
}
QFrame#divider {
    background-color: @bezel;
    border: none;
}
QLabel {
    color: @status;
    background: transparent;
}
QLabel#optionLabel {
    padding: 3px 8px;
    border-radius: 3px;
}
QLabel#optionLabel[focused="true"] {
    background-color: @barFill;
    color: @value;
}
QLabel#readout {
    color: @value;
}
QLabel#shoulderHint {
    color: @value;
    border: 1px solid @status;
    border-radius: 4px;
    padding: 0px 6px;
}
QTabBar::tab {
    background: transparent;
    color: @status;
    border: none;
    border-bottom: 2px solid transparent;
    padding: 4px 24px 6px 24px;
}
QTabBar::tab:selected {
    color: @value;
    border-bottom: 2px solid @value;
}
QTabBar::tab:hover:!selected {
    color: @value;
}
QTreeWidget {
    background-color: @background;
    color: @value;
    border: 1px solid @bezel;
    border-radius: 3px;
    outline: none;
}
QTreeWidget:focus {
    border-color: @status;
}
QTreeWidget::item {
    padding: 1px 0px;
}
QTreeWidget::item:hover {
    background-color: rgb(0, 30, 0);
}
QTreeWidget::item:selected {
    background-color: @barFill;
    color: rgb(0, 255, 0);
}
QPushButton {
    background-color: @background;
    color: @value;
    border: 1px solid @status;
    border-radius: 4px;
    padding: 4px 18px;
}
QPushButton:hover {
    background-color: rgb(0, 30, 0);
}
QPushButton:focus {
    background-color: @barFill;
    color: rgb(0, 255, 0);
    border-color: @value;
}
QPushButton:pressed {
    background-color: @status;
}
QPushButton#exitGame {
    color: @fault;
    border-color: rgb(110, 0, 0);
}
QPushButton#exitGame:hover {
    background-color: rgb(30, 0, 0);
}
QPushButton#exitGame:focus {
    background-color: rgb(80, 0, 0);
    color: rgb(255, 90, 90);
    border-color: @fault;
}
QFrame#displayChoice {
    border: 1px solid @bezel;
    border-radius: 4px;
}
QFrame#displayChoice:focus {
    border-color: @value;
}
QPushButton#choice {
    background: transparent;
    color: @status;
    border: none;
    border-radius: 3px;
    padding: 3px 16px;
}
QPushButton#choice:checked {
    background-color: @barFill;
    color: @value;
}
QSlider::groove:horizontal {
    height: 4px;
    background: @bezel;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: @status;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: @status;
    width: 14px;
    margin: -6px 0;
    border-radius: 7px;
}
QSlider::handle:horizontal:hover, QSlider::handle:horizontal:focus {
    background: @value;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
}
QScrollBar::handle:vertical {
    background: @bezel;
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

QString makeStyleSheet()
{
    QString sheet = STYLE_SHEET;
    for (const auto& [name, colour] : { std::pair { "@background", PANEL_BACKGROUND_COLOUR },
                                        std::pair { "@status", STATUS_COLOUR },
                                        std::pair { "@bezel", PANEL_BEZEL_COLOUR },
                                        std::pair { "@barFill", BAR_FILL_COLOUR },
                                        std::pair { "@value", PANEL_VALUE_COLOUR },
                                        std::pair { "@fault", PANEL_FAULT_COLOUR } }) {
        sheet.replace(name, cssColour(colour));
    }
    return sheet;
}

// The level list's columns
enum LevelColumn {
    NumberColumn,
    NameColumn,
    StatusColumn,
};
// Level states, shown like the status panel's system states
const QColor LOCKED_LEVEL_COLOUR(0, 70, 0);
const QColor LOCKED_STATUS_COLOUR(120, 30, 30);

QLabel* makeOptionLabel(const char* text)
{
    auto* label = new QLabel(text);
    label->setObjectName("optionLabel");
    return label;
}

bool isKey(const QKeyEvent* event, std::initializer_list<int> keys)
{
    return std::ranges::find(keys, event->key()) != keys.end();
}

} // namespace

MenuDialog::MenuDialog(
    const std::string& dataDir,
    const MenuResults& initialValues,
    QWidget* parent)
    : QDialog(parent)
    , m_results(initialValues)
    , m_levelFileLister(dataDir)
    , m_tabs(new QTabBar)
    , m_pages(new QStackedWidget)
    , m_leftShoulderHint(new QLabel)
    , m_rightShoulderHint(new QLabel)
    , m_levelList(new QTreeWidget)
    , m_ok(new QPushButton("OK"))
    , m_cancel(new QPushButton("Cancel"))
    , m_exitGame(new QPushButton("Exit Game"))
{
    setWindowTitle("Amaze");
    setModal(true);
    // No title bar: the OS's light-mode one clashes with the game. The
    // green border drawn by the "border" frame below marks the edges instead.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // Tabs, with hints at either side for the gamepad buttons which switch
    // between them
    m_tabs->addTab("LEVELS");
    m_tabs->addTab("OPTIONS");
    m_tabs->setDrawBase(false);
    m_tabs->setExpanding(false);
    m_tabs->setFocusPolicy(Qt::NoFocus);
    m_pages->addWidget(makeLevelsPage());
    m_pages->addWidget(makeOptionsPage());
    connect(m_tabs, &QTabBar::currentChanged, this, [this](int index) { showTab(index); });
    for (QLabel* hint : { m_leftShoulderHint, m_rightShoulderHint }) {
        hint->setObjectName("shoulderHint");
        hint->setAlignment(Qt::AlignCenter);
        // (Hidden without a gamepad, but still taking up space, so the tabs
        // don't move when one is connected)
        QSizePolicy policy = hint->sizePolicy();
        policy.setRetainSizeWhenHidden(true);
        hint->setSizePolicy(policy);
        hint->hide();
    }
    auto* tabRow = new QHBoxLayout;
    tabRow->addStretch();
    tabRow->addWidget(m_leftShoulderHint);
    tabRow->addSpacing(8);
    tabRow->addWidget(m_tabs);
    tabRow->addSpacing(8);
    tabRow->addWidget(m_rightShoulderHint);
    tabRow->addStretch();

    // Buttons
    m_exitGame->setObjectName("exitGame");
    m_ok->setDefault(true);
    connect(m_ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_exitGame, &QPushButton::clicked, this, [this]() { done(ExitGame); });
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_ok);
    buttons->addWidget(m_cancel);
    buttons->addStretch();
    buttons->addWidget(m_exitGame);

    // Keyboard (and gamepad) navigation: see eventFilter()
    std::vector<QWidget*> focusable { m_levelList, m_ok, m_cancel, m_exitGame };
    for (const auto& option : m_options) {
        focusable.push_back(option.control);
    }
    for (QWidget* widget : focusable) {
        widget->setFocusPolicy(Qt::StrongFocus);
        widget->installEventFilter(this);
    }
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* now) {
        for (const auto& option : m_options) {
            if (option.control == now) {
                m_lastOption = now;
            }
        }
        updateFocusHighlight();
    });

    // A border and bezel, like the status panel's
    auto* border = new QFrame;
    border->setObjectName("border");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(border);
    auto* bezel = new QFrame;
    bezel->setObjectName("bezel");
    auto* borderLayout = new QVBoxLayout(border);
    borderLayout->setContentsMargins(5, 5, 5, 5);
    borderLayout->addWidget(bezel);

    auto* divider = new QFrame;
    divider->setObjectName("divider");
    divider->setFixedHeight(1);
    auto* layout = new QVBoxLayout(bezel);
    layout->setContentsMargins(16, 10, 16, 16);
    layout->addLayout(tabRow);
    layout->addSpacing(6);
    layout->addWidget(m_pages, 1);
    layout->addSpacing(6);
    layout->addWidget(divider);
    layout->addSpacing(6);
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
    // The tabs are spaced out, like the status panel's title
    QFont tabFont = font;
    tabFont.setLetterSpacing(QFont::PercentageSpacing, 130);
    m_tabs->setFont(tabFont);
    QFont hintFont = font;
    hintFont.setPointSizeF(font.pointSizeF() * 0.75);
    m_leftShoulderHint->setFont(hintFont);
    m_rightShoulderHint->setFont(hintFont);
    // Room for the widest readout, so the sliders don't change size
    for (QLabel* readout : findChildren<QLabel*>("readout")) {
        readout->setMinimumWidth(QFontMetrics(font).horizontalAdvance("100%") + 8);
    }
    setStyleSheet(makeStyleSheet());

    resize(840, 690);
    showTab(LevelsTab);
}

QWidget* MenuDialog::makeLevelsPage()
{
    // Number, name and state are separate columns, with the numbers
    // right-aligned, so the names line up however many digits the numbers
    // have
    m_levelList->setColumnCount(3);
    m_levelList->setHeaderHidden(true);
    m_levelList->setRootIsDecorated(false);
    m_levelList->setIndentation(0);
    m_levelList->header()->setStretchLastSection(false);
    m_levelList->header()->setSectionResizeMode(NumberColumn, QHeaderView::ResizeToContents);
    m_levelList->header()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
    m_levelList->header()->setSectionResizeMode(StatusColumn, QHeaderView::ResizeToContents);
    for (const auto& [number, fileData] : m_levelFileLister.getFileMap()) {
        auto* item = new QTreeWidgetItem(
            { QString::number(number), QString::fromStdString(fileData.description) });
        item->setTextAlignment(NumberColumn, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(StatusColumn, Qt::AlignRight | Qt::AlignVCenter);
        item->setForeground(NumberColumn, STATUS_COLOUR);
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
    return m_levelList;
}

QWidget* MenuDialog::makeOptionsPage()
{
    auto* page = new QWidget;
    auto* grid = new QGridLayout(page);
    grid->setContentsMargins(0, 12, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);
    grid->setColumnStretch(1, 1);

    // A slider, with a readout of its value as a percentage
    auto addSlider = [&](const char* text, int min, int max, int step, int MenuResults::* value) {
        auto* slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setSingleStep(step);
        slider->setPageStep(step * 2);
        slider->setValue(m_results.*value);
        auto* readout = new QLabel;
        readout->setObjectName("readout");
        readout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto update = [this, readout, value](int newValue) {
            m_results.*value = newValue;
            readout->setText(QString("%1%").arg(newValue));
        };
        update(slider->value());
        connect(slider, &QSlider::valueChanged, this, update);
        const int row = static_cast<int>(m_options.size());
        auto* label = makeOptionLabel(text);
        grid->addWidget(label, row, 0);
        grid->addWidget(slider, row, 1);
        grid->addWidget(readout, row, 2);
        m_options.push_back({ label, slider });
    };
    addSlider("Background music volume", 0, 100, 5, &MenuResults::musicVolumePercent);
    addSlider(
        "Stick dead zone",
        MIN_STICK_DEAD_ZONE_PERCENT,
        MAX_STICK_DEAD_ZONE_PERCENT,
        1,
        &MenuResults::stickDeadZonePercent);
    addSlider("Rumble intensity", 0, 100, 5, &MenuResults::rumbleIntensityPercent);

    // Display: full screen or windowed, as two halves of one control, chosen
    // with Left / Right (see eventFilter()) or by clicking
    m_displayChoice = new QFrame;
    m_displayChoice->setObjectName("displayChoice");
    m_fullScreen = new QPushButton("Full screen");
    m_windowed = new QPushButton("Windowed");
    auto* choiceLayout = new QHBoxLayout(m_displayChoice);
    choiceLayout->setContentsMargins(3, 3, 3, 3);
    choiceLayout->setSpacing(3);
    for (QPushButton* choice : { m_fullScreen, m_windowed }) {
        choice->setObjectName("choice");
        choice->setCheckable(true);
        choice->setFocusPolicy(Qt::NoFocus);
        choiceLayout->addWidget(choice);
    }
    connect(m_fullScreen, &QPushButton::clicked, this, [this]() {
        setFullScreenChoice(true);
        m_displayChoice->setFocus();
    });
    connect(m_windowed, &QPushButton::clicked, this, [this]() {
        setFullScreenChoice(false);
        m_displayChoice->setFocus();
    });
    setFullScreenChoice(m_results.fullScreen);
    const int row = static_cast<int>(m_options.size());
    auto* label = makeOptionLabel("Display");
    grid->addWidget(label, row, 0);
    grid->addWidget(m_displayChoice, row, 1, Qt::AlignLeft);
    m_options.push_back({ label, m_displayChoice });

    grid->setRowStretch(row + 1, 1);
    return page;
}

void MenuDialog::setFullScreenChoice(bool fullScreen)
{
    m_results.fullScreen = fullScreen;
    m_fullScreen->setChecked(fullScreen);
    m_windowed->setChecked(!fullScreen);
}

void MenuDialog::prepareToShow(bool fullScreen)
{
    setFullScreenChoice(fullScreen);
    m_lastOption = nullptr;
    if (m_tabs->currentIndex() == LevelsTab) {
        showTab(LevelsTab);
    } else {
        m_tabs->setCurrentIndex(LevelsTab); // which calls showTab()
    }
}

void MenuDialog::setShoulderButtonNames(
    const std::optional<std::pair<std::string, std::string>>& names)
{
    if (names == m_shoulderButtonNames) {
        return;
    }
    m_shoulderButtonNames = names;
    if (names.has_value()) {
        m_leftShoulderHint->setText(QString::fromStdString(names->first));
        m_rightShoulderHint->setText(QString::fromStdString(names->second));
    }
    m_leftShoulderHint->setVisible(names.has_value());
    m_rightShoulderHint->setVisible(names.has_value());
}

void MenuDialog::switchTab(int direction)
{
    const int index = m_tabs->currentIndex() + direction;
    if (index >= 0 && index < m_tabs->count()) {
        m_tabs->setCurrentIndex(index); // which calls showTab()
    }
}

void MenuDialog::showTab(int index)
{
    m_pages->setCurrentIndex(index);
    if (index == LevelsTab) {
        m_levelList->setFocus();
    } else {
        (m_lastOption ? m_lastOption : m_options.front().control)->setFocus();
    }
}

std::vector<QWidget*> MenuDialog::rows() const
{
    if (m_tabs->currentIndex() == LevelsTab) {
        return { m_levelList };
    }
    std::vector<QWidget*> controls;
    for (const auto& option : m_options) {
        controls.push_back(option.control);
    }
    return controls;
}

bool MenuDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress) {
        return QDialog::eventFilter(watched, event);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(event);

    // Ctrl+Tab / Ctrl+Shift+Tab switch tabs, like the gamepad's shoulder
    // buttons. (On macOS, Qt reports the Control key as Meta.)
    const auto modifiers = keyEvent->modifiers() & ~Qt::KeypadModifier;
    if (modifiers & (Qt::ControlModifier | Qt::MetaModifier)) {
        if (keyEvent->key() == Qt::Key_Tab) {
            switchTab(1);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Backtab) {
            switchTab(-1);
            return true;
        }
    }
    if (modifiers) {
        return QDialog::eventFilter(watched, event);
    }
    const bool up = keyEvent->key() == Qt::Key_Up;
    const bool down = keyEvent->key() == Qt::Key_Down;
    const bool left = keyEvent->key() == Qt::Key_Left;
    const bool right = keyEvent->key() == Qt::Key_Right;
    const bool enter = isKey(keyEvent, { Qt::Key_Return, Qt::Key_Enter });

    if (watched == m_levelList) {
        // (Levels which can't be chosen yet are skipped)
        int row = m_levelList->currentIndex().row();
        if (up && row <= firstUnlockedRow()) {
            // Nothing above
        } else if ((down && row >= lastUnlockedRow()) || right) {
            // Right is a shortcut to the buttons without scrolling to the end
            m_ok->setFocus();
        } else if (left) {
            // Nothing to the left
        } else {
            return QDialog::eventFilter(watched, event);
        }
        return true;
    }

    const auto rowList = rows();
    auto row = std::ranges::find(rowList, watched);
    if (row != rowList.end()) {
        // An option. (Up/Down would otherwise change a slider's value;
        // Left/Right still do.)
        if (up) {
            if (row != rowList.begin()) {
                (*std::prev(row))->setFocus();
            }
        } else if (down) {
            if (std::next(row) != rowList.end()) {
                (*std::next(row))->setFocus();
            } else {
                m_ok->setFocus();
            }
        } else if (watched == m_displayChoice && (left || right)) {
            setFullScreenChoice(left);
        } else if (enter) {
            accept();
        } else {
            return QDialog::eventFilter(watched, event);
        }
        return true;
    }

    const std::array<QPushButton*, 3> buttons { m_ok, m_cancel, m_exitGame };
    auto button = std::ranges::find(buttons, watched);
    if (button != buttons.end()) {
        if (up) {
            // Back to the tab's last row, or the option last used
            if (m_tabs->currentIndex() == OptionsTab && m_lastOption) {
                m_lastOption->setFocus();
            } else {
                rowList.back()->setFocus();
            }
        } else if (left && button != buttons.begin()) {
            (*std::prev(button))->setFocus();
        } else if (right && std::next(button) != buttons.end()) {
            (*std::next(button))->setFocus();
        } else if (enter) {
            // Don't rely on the platform pressing the focused button on Enter
            (*button)->click();
        } else if (down || left || right) {
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
        const bool unlocked = progress.isUnlocked(level);
        // Disabled items can't be selected
        item->setDisabled(!unlocked);
        if (progress.isCompleted(level)) {
            item->setText(StatusColumn, "COMPLETE");
            item->setForeground(StatusColumn, STATUS_COLOUR);
        } else if (unlocked) {
            item->setText(StatusColumn, "NEXT");
            item->setForeground(StatusColumn, PANEL_DEPLETED_COLOUR);
        } else {
            item->setText(StatusColumn, "LOCKED");
            item->setForeground(StatusColumn, LOCKED_STATUS_COLOUR);
        }
        item->setForeground(NumberColumn, unlocked ? STATUS_COLOUR : LOCKED_LEVEL_COLOUR);
        item->setForeground(NameColumn, unlocked ? PANEL_VALUE_COLOUR : LOCKED_LEVEL_COLOUR);
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
    std::vector<QWidget*> order = rows();
    order.insert(order.end(), { m_ok, m_cancel, m_exitGame });
    auto it = std::ranges::find(order, focusWidget());
    int index = it == order.end() ? 0 : static_cast<int>(it - order.begin());
    int count = static_cast<int>(order.size());
    index = (index + (next ? 1 : count - 1)) % count;
    order[index]->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
    return true;
}

void MenuDialog::updateFocusHighlight()
{
    // Light up the label of the option that has focus, so it's clear which
    // row the arrow keys / gamepad are on
    for (const auto& option : m_options) {
        bool focused = option.control->hasFocus();
        if (option.label->property("focused").toBool() != focused) {
            option.label->setProperty("focused", focused);
            option.label->style()->unpolish(option.label);
            option.label->style()->polish(option.label);
        }
    }
}

} // namespace amaze
} // namespace marengo
