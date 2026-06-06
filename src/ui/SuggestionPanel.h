#pragma once

#include <QDockWidget>
#include <QVector>
#include <QMap>
#include "api/ApiModels.h"

class QListWidget;
class QPushButton;
class QLabel;
class QStackedWidget;

class SuggestionPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit SuggestionPanel(QWidget *parent = nullptr);

    void setSuggestions(const QVector<Models::SuggestedPlayer> &suggestions);
    void setCompareData(const QMap<QString, double> &formationScores);
    void clearSuggestions();

signals:
    void applySuggestionRequested(const QVector<Models::SuggestedPlayer> &suggestions);
    void compareFormationsRequested();

private:
    void setupUi();
    void showSuggestionPage();
    void showComparePage();

    QStackedWidget *m_stack;

    // Suggestion page
    QListWidget *m_suggestionList;
    QPushButton *m_applyBtn;
    QPushButton *m_compareBtn;
    QLabel      *m_totalScoreLabel;

    // Compare page
    QListWidget *m_compareList;
    QPushButton *m_backBtn;

    QVector<Models::SuggestedPlayer> m_currentSuggestions;
};
