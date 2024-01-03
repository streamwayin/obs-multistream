#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <QWidget>
#include <QString>
#include <QObject>


class AuthManager  : public QObject {
    Q_OBJECT

public:

    bool isAuthenticated();
    QWidget* handleAuthTab();

public:
    QString uid;
    QString key;

    // Signal authentication success/failure events
signals:
    void authenticationSuccess();

};

#endif // AUTH_MANAGER_H
