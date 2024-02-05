#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QTabWidget>

class PushWidget;


class Dashboard : public QObject {
    Q_OBJECT

public:
    Dashboard();
    std::vector<PushWidget*> GetAllPushWidgets();
    void SaveConfig();
    void LoadConfig();
    QWidget* handleTab();
    QWidget* createTab3();
    QWidget* createTab1(const QString& uid, const QString& key , QTabWidget *tabWidget);
    QWidget* createTab2(const QString& uid, const QString& key , QTabWidget *tabWidget);
    QWidget* serverBaseStreaming(const QString& uid, const QString& key , QTabWidget *tabWidget , std::string id , bool isLive);
    void handleSuccessfulLogin(const QString& uid, const QString& key, QVBoxLayout *newUiLayout , QTabWidget *tabWidget);
public:
    QString uid;
    QString key;

signals:
    void logOutDashboard();
    void refreshBroadcasts();

private:
    AuthManager authManager ;
    
};

#endif // DASHBOARD_H
