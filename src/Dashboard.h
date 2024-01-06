#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QTabWidget>

class Dashboard : public QObject {
    Q_OBJECT

public:
    Dashboard();
    
    QWidget* handleTab();
    QWidget* createTab3();
    QWidget* createTab1(const QString& uid, const QString& key , QTabWidget *tabWidget);
    QWidget* createTab2(const QString& uid, const QString& key , QTabWidget *tabWidget);
    void handleSuccessfulLogin(const QString& uid, const QString& key, QVBoxLayout *newUiLayout , QTabWidget *tabWidget);
public:
    QString uid;
    QString key;

signals:
    void logOutDashboard();
    void refreshBroadcasts();
};

#endif // DASHBOARD_H
