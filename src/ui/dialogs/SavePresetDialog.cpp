#include "SavePresetDialog.h"
#include "utils/CustomPresetManager.h"
#include "controllers/AtmospherePreset.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

SavePresetDialog::SavePresetDialog(QWidget* parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_warningLabel(nullptr)
{
    setWindowTitle("Save as Custom Preset");
    setModal(true);
    setMinimumWidth(400);
    setupUI();
}

SavePresetDialog::~SavePresetDialog()
{
    // Qt parent-child handles cleanup
}

void SavePresetDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Info label
    QLabel* infoLabel = new QLabel("Save the current atmosphere settings as a custom preset.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #808080;");
    mainLayout->addWidget(infoLabel);

    // Form layout for name and description
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("My Custom Preset");
    m_nameEdit->setMaxLength(50);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &SavePresetDialog::onNameChanged);
    formLayout->addRow("Name:", m_nameEdit);

    m_descriptionEdit = new QLineEdit();
    m_descriptionEdit->setPlaceholderText("Optional description");
    m_descriptionEdit->setMaxLength(200);
    formLayout->addRow("Description:", m_descriptionEdit);

    mainLayout->addLayout(formLayout);

    // Warning label (hidden by default)
    m_warningLabel = new QLabel();
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setStyleSheet("color: #E24A4A; padding: 4px;");
    m_warningLabel->hide();
    mainLayout->addWidget(m_warningLabel);

    mainLayout->addStretch();

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton("Save Preset");
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false);  // Disabled until valid name entered
    connect(m_okButton, &QPushButton::clicked, this, &SavePresetDialog::validateAndAccept);
    buttonLayout->addWidget(m_okButton);

    mainLayout->addLayout(buttonLayout);

    // Focus name field
    m_nameEdit->setFocus();
}

QString SavePresetDialog::presetName() const
{
    return m_nameEdit->text().trimmed();
}

QString SavePresetDialog::presetDescription() const
{
    return m_descriptionEdit->text().trimmed();
}

void SavePresetDialog::onNameChanged(const QString& text)
{
    QString name = text.trimmed();
    bool valid = !name.isEmpty();
    bool exists = false;
    bool isBuiltIn = false;

    if (valid) {
        // Check if name matches a built-in preset
        QList<AtmospherePreset> builtIn = AtmospherePreset::getBuiltInPresets();
        for (const auto& preset : builtIn) {
            if (preset.name.compare(name, Qt::CaseInsensitive) == 0) {
                isBuiltIn = true;
                break;
            }
        }

        // Check if name matches existing custom preset
        exists = CustomPresetManager::instance().isCustomPreset(name);
    }

    // Update warning label
    if (isBuiltIn) {
        m_warningLabel->setText("Cannot use a built-in preset name. Please choose a different name.");
        m_warningLabel->show();
        m_okButton->setEnabled(false);
    } else if (exists) {
        m_warningLabel->setText("A preset with this name already exists. Saving will overwrite it.");
        m_warningLabel->setStyleSheet("color: #FFA500; padding: 4px;");  // Orange for warning
        m_warningLabel->show();
        m_okButton->setEnabled(true);
    } else if (valid) {
        m_warningLabel->hide();
        m_okButton->setEnabled(true);
    } else {
        m_warningLabel->hide();
        m_okButton->setEnabled(false);
    }
}

void SavePresetDialog::validateAndAccept()
{
    QString name = presetName();
    if (name.isEmpty()) {
        return;
    }

    // Check built-in presets (disallow)
    QList<AtmospherePreset> builtIn = AtmospherePreset::getBuiltInPresets();
    for (const auto& preset : builtIn) {
        if (preset.name.compare(name, Qt::CaseInsensitive) == 0) {
            m_warningLabel->setText("Cannot overwrite built-in presets.");
            m_warningLabel->setStyleSheet("color: #E24A4A; padding: 4px;");
            m_warningLabel->show();
            return;
        }
    }

    accept();
}
