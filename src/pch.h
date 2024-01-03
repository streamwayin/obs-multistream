#pragma once

#include <memory>
#include <string>
#include <optional>
#include <type_traits>

#include <QMainWindow>
#include <QDockWidget>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QEvent>
#include <QThread>
#include <QTabWidget>
#include <QLineEdit>
#include <QTimer>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QAction>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTableWidget>
#include <QEventLoop>
#include <QRadioButton>
#include <QScrollArea>
#include <QPixmap>
#include <QImage>
#include <QDesktopServices>
#include <QTabWidget>
#include "obs-multi-rtmp.h"
#include "obs-module.h"
#include "obs-frontend-api.h"
#include "util/config-file.h"
#include "obs.h"
#include "curl/curl.h"
#include <QIntValidator>
#include <QStackedWidget>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <QFile>
#include <random>
#include <filesystem>
#include <fstream> 
#include "StackedWidgetManager.h"
#include <unordered_set>
#include <algorithm>
#include <util/platform.h>
#include "json-util.hpp"
#include "AuthManager.h"
#include "thread"
#include "future"
#define TAG "[obs-multi-rtmp] "


inline std::string tostdu8(const QString& qs)
{
    auto b = qs.toUtf8();
    return std::string(b.begin(), b.end());
}
