#pragma once

#include <functional>
#include <QString>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QTabWidget>
#include <QGridLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QThread>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "AuthManager.h"
#include "Dashboard.h"
#include "pch.h"

extern QString uid; // Declare uid as external variable
extern QString key; // Declare key as external variable



// Forward declarations
class PushWidget;
class AuthManager;
class Dashboard;

class GlobalService;

class MultiOutputWidget : public QMainWindow {
    Q_OBJECT

    int dockLocation_;
    bool dockVisible_;
    bool reopenShown_;
    std::string currentBroadcast;
    QJsonDocument jsonDoc;
	QJsonObject jsonObj;
	QString uid , key;
    QStackedWidget stackedWidget_;



public:
    MultiOutputWidget(QWidget* parent = nullptr);

    QWidget* footerWidget();


private slots:
    void switchToDashboard();
    void logOut();

public:
    void visibleToggled(bool visible);
    std::vector<PushWidget*> GetAllPushWidgets();
    void SaveConfig();
    void LoadConfig();
    void startStreamingListner();

    QWidget* container_ = nullptr;
    QScrollArea scroll_;
    QVBoxLayout* itemLayout_ = nullptr;
    QVBoxLayout* layout_ = nullptr;

private:
    AuthManager authManager ;
    Dashboard dashboardManager;

};

class GlobalService
{
public:
    
    ~GlobalService() {}
    virtual bool RunInUIThread(std::function<void()> task) = 0;
   
};

GlobalService& GetGlobalService();
