#pragma once

#include <functional>
#include <QString>

extern QString uid; // Declare uid as external variable
extern QString key; // Declare key as external variable
bool isAuthenticated(); // Declare the function

class GlobalService
{
public:
    ~GlobalService() {}
    virtual bool RunInUIThread(std::function<void()> task) = 0;
};

GlobalService& GetGlobalService();
