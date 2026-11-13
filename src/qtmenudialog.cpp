#include "qtmenudialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace marengo {
namespace amaze {

namespace {

QSlider* makeSlider(int min, int max, int value)
{
    auto* slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    return slider;
}

} // namespace

MenuDialog::MenuDialog(const std::string& dataDir, QWidget* parent)
    : QDialog(parent)
    , m_levelFileLister(dataDir)
    , m_levelList(new QListWidget)
{
    setWindowTitle("Amaze");
    setModal(true);

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
    for (const auto& [number, fileData] : m_levelFileLister.getFileMap()) {
        auto* item = new QListWidgetItem(QString::fromStdString(fileData.description));
        item->setData(Qt::UserRole, QString::fromStdString(fileData.filename));
        m_levelList->addItem(item);
    }
    connect(m_levelList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item) {
        if (item) {
            m_results.levelFileToLoad = item->data(Qt::UserRole).toString().toStdString();
        }
    });
    // Double-click or Enter on a level acts as OK
    connect(m_levelList, &QListWidget::itemActivated, this, &QDialog::accept);

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

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(options);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Level selection"));
    layout->addWidget(m_levelList, 1);
    layout->addLayout(buttons);

    resize(560, 460);
    m_levelList->setFocus();
}

} // namespace amaze
} // namespace marengo
