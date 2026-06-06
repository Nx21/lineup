#pragma once

#include <QDialog>
#include <QDateTime>

class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;

class DatabaseManager;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(DatabaseManager *db, QWidget *parent = nullptr);

    QString apiKey()   const;
    QString theme()    const;   // "dark" | "light"

    void    loadSettings();
    void    saveSettings();

signals:
    void themeChanged(const QString &theme);

private slots:
    void onClearCache();
    void onAccepted();

private:
    void setupUi();
    void refreshLastFetch();

    QLineEdit   *m_apiKeyEdit;
    QComboBox   *m_themeCombo;
    QLabel      *m_lastFetchLabel;
    QPushButton *m_clearBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;

    DatabaseManager *m_db;
};
