#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QString>

class Dashboard {
public:
    Dashboard();
    
    QWidget* handleTab();

public:
    QString uid;
    QString key;
};

#endif // AUTH_MANAGER_H
