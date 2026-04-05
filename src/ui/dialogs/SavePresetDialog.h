#ifndef SAVEPRESETDIALOG_H
#define SAVEPRESETDIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;

// Dialog for saving the current atmosphere state as a custom preset
class SavePresetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SavePresetDialog(QWidget* parent = nullptr);
    ~SavePresetDialog() override;

    QString presetName() const;
    QString presetDescription() const;

private slots:
    void onNameChanged(const QString& text);
    void validateAndAccept();

private:
    void setupUI();

    QLineEdit* m_nameEdit;
    QLineEdit* m_descriptionEdit;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QLabel* m_warningLabel;
};

#endif // SAVEPRESETDIALOG_H
