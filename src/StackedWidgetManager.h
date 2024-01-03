// StackedWidgetManager.h
#pragma once

#include <QStackedWidget>

class StackedWidgetManager {
public:
    static StackedWidgetManager& getInstance() {
        static StackedWidgetManager instance;
        return instance;
    }

    QStackedWidget& getStackedWidget() {
        return stackedWidget;
    }

private:
    StackedWidgetManager() {} // Private constructor to prevent instantiation
    QStackedWidget stackedWidget;
};
