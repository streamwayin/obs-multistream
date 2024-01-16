#include "pch.h"

#include <list>
#include <regex>
#include <filesystem>

#include "push-widget.h"
#include "plugin-support.h"

#include "output-config.h"
#include "Dashboard.h"
#include "AuthManager.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#define ConfigSection "obs-multi-rtmp"



static class GlobalServiceImpl : public GlobalService
{
public:
    bool RunInUIThread(std::function<void()> task) override {
        if (uiThread_ == nullptr)
            return false;
        QMetaObject::invokeMethod(uiThread_, [func = std::move(task)]() {
            func();
        });
        return true;
    }

    QThread* uiThread_ = nullptr;
} s_service;


GlobalService& GetGlobalService() {
    return s_service;
}



    MultiOutputWidget::MultiOutputWidget(QWidget* parent)
        : QMainWindow(parent)
        , reopenShown_(false)
    {
        setWindowTitle(obs_module_text("Title"));
        
        container_ = new QWidget(&scroll_);
        layout_ = new QVBoxLayout(container_);
        layout_->setAlignment(Qt::AlignmentFlag::AlignTop); 

        connect(&authManager , SIGNAL(authenticationSuccess()) , this , SLOT(switchToDashboard()));
        connect(&dashboardManager , SIGNAL(logOutDashboard()) , this , SLOT(logOut()));
        connect(&dashboardManager , SIGNAL(refreshBroadcasts()) , this , SLOT(switchToDashboard()));

         if (authManager.isAuthenticated()) {
            dashboardManager.uid = authManager.uid;
            dashboardManager.key = authManager.key;
            QWidget* dashboardWidget = dashboardManager.handleTab();
            stackedWidget_.addWidget(dashboardWidget);
        } else {
            
            QWidget* authTabWidget = authManager.handleAuthTab();
            stackedWidget_.addWidget(authTabWidget);
        }
       
     QScrollArea* scrollArea = new QScrollArea;
     stackedWidget_.setFixedHeight(500);
    scrollArea->setWidget(&stackedWidget_);
    scrollArea->setWidgetResizable(true); // Allow the scroll area to resize its contents
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Add the scroll area to the layout instead of the stacked widget directly
    layout_->addWidget(scrollArea);

    // Create and add the footer widget
    QWidget* footer = footerWidget(); // Create your footer widget here
    layout_->addWidget(footer);

    // Set the content widget as the central widget
    setCentralWidget(container_);


      
    }


    QWidget* MultiOutputWidget::footerWidget() {
        QWidget* footer = new QWidget;

        // Assuming this code is within the footerWidget setup function

        // Create a layout for the footer
        QVBoxLayout* footerLayout = new QVBoxLayout;

        // First QLabel
        auto watchLabel = new QLabel(u8"<p><a href=\"https://support.streamway.in/obs-multistream-plugin/\">Watch how to use plugin</a> or <a href=\"https://support.streamway.in/contact/\">Contact Us </a></p>");
        watchLabel->setTextFormat(Qt::RichText);
        watchLabel->setWordWrap(true);
        watchLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        watchLabel->setOpenExternalLinks(true);
        footerLayout->addWidget(watchLabel);

        // Second QLabel
        // auto supportLabel = new QLabel(u8"<p> <a href=\"https://support.streamway.in/contact/\">Contact Us</a></p>");
        // supportLabel->setTextFormat(Qt::RichText);
        // supportLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        // supportLabel->setOpenExternalLinks(true);
        // supportLabel->setWordWrap(true);
        // footerLayout->addWidget(supportLabel);

        // Set the footer layout to your footer widget
        footer->setLayout(footerLayout);

        return footer;
};

    void MultiOutputWidget::visibleToggled(bool visible)
    {
        dockVisible_ = visible;

        if (visible == false
            && reopenShown_ == false
            && !config_has_user_value(obs_frontend_get_global_config(), ConfigSection, "DockVisible"))
        {
            reopenShown_ = true;
            QMessageBox(QMessageBox::Icon::Information, 
                obs_module_text("Notice.Title"), 
                obs_module_text("Notice.Reopen"), 
                QMessageBox::StandardButton::Ok,
                this
            ).exec();
        }
    }

    std::vector<PushWidget*> MultiOutputWidget::GetAllPushWidgets()
    {
        std::vector<PushWidget*> result;
        for(auto& c : container_->children())
        {
            if (c->objectName() == "push-widget")
            {
                auto w = dynamic_cast<PushWidget*>(c);
                result.push_back(w);
            }
        }
        return result;
    }

    void MultiOutputWidget::SaveConfig()
    {
        SaveMultiOutputConfig();
    }

    void MultiOutputWidget::LoadConfig()
    {
        for(auto x: GetAllPushWidgets()) {
            delete x;
        }
        GlobalMultiOutputConfig() = {};

        if (LoadMultiOutputConfig() == false) {
            ImportLegacyMultiOutputConfig();
        }
        
        for(auto x: GlobalMultiOutputConfig().targets)
        {
            auto pushwidget = createPushWidget(x->id, container_);
            itemLayout_->addWidget(pushwidget);
        }
    }

//     void MultiOutputWidget::startStreamingListner(){
//         try{
//             std::string currBrod ;
// 	auto profiledir = obs_frontend_get_current_profile_path();
//     if (profiledir) {
//         QString filename = QString::fromStdString(std::string(profiledir) + "/obs-multi-rtmp_auth.json");
//         QFile file(filename);

//     if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         QByteArray jsonData = file.readAll();
//         QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));

//         if (!jsonDoc.isNull() && jsonDoc.isObject()) {
//             QJsonObject jsonObject = jsonDoc.object();

//             if (jsonObject.contains("current_broadcast")) {
//                 QJsonValue currentBroadcastValue = jsonObject["current_broadcast"];

//                 if (currentBroadcastValue.isObject()) {
//                     QJsonObject currentBroadcast = currentBroadcastValue.toObject();

//                     if (currentBroadcast.contains("id")) {
//                         QString idValue = currentBroadcast["id"].toString();
//                         currBrod = idValue.toStdString();
//                         // Use currBrod further in your code
//                     }
//                 }
//             }
//         }

//         file.close();
//     }
//         bfree(profiledir);
//     }

//     if (!currBrod.empty()) {
			
// 				CURL *curl;
// 				curl = curl_easy_init();
// 				std::string url = "https://api.streamway.in/v1/webhook/obs/" + currBrod
// 				;
// 				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
// 				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
// 			   	// Set authentication
//     			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
//     			curl_easy_setopt(curl, CURLOPT_USERNAME, uid.toStdString().c_str());
//     			curl_easy_setopt(curl, CURLOPT_PASSWORD, key.toStdString().c_str());

// 				struct curl_slist *headers = NULL;
// 				headers = curl_slist_append(headers, "Accept: */*");
// 				headers = curl_slist_append(headers, "Content-Type: application/json");
// 				headers = curl_slist_append(headers, "obs-webhook-auth: b1d66555d2a1cfe4e773457dd44dc664");
// 				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			

// 				CURLcode ret = curl_easy_perform(curl);

// 				long response_code;
// 					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

// 	    			if (response_code != 200) {
//         				curl_easy_cleanup(curl);
//     				}
//                     else{
//                         auto profiledir = obs_frontend_get_current_profile_path();

//                     if (profiledir) {
//                         std::string filename = profiledir;
//                         filename += "/obs-multi-rtmp_auth.json";

//                         // Read existing JSON content from the file
//                         std::ifstream inFile(filename);
//                         nlohmann::json configJson;

//                     if (inFile.is_open()) {
//                         inFile >> configJson;
//                         inFile.close();
//                     }

//                     // Create an object for the current broadcast with id and status
//                     nlohmann::json currentBroadcastObj;
                    
//                     currentBroadcastObj["status"] = "live";
//                     currentBroadcastObj["id"] = currBrod;
//                     // Update uid and key in the existing JSON object
//                     configJson["current_broadcast"] = currentBroadcastObj;
                    

//                     // Convert the updated JSON to a string
//                     std::string content = configJson.dump();

//                     // Write the updated content back to the file
//                     os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
//                 }
//                 bfree(profiledir);
//                     }

// 	  			curl_easy_cleanup(curl);
// 				curl = NULL;
			
//     }
			
//         }catch(const std::exception& e){
//             obs_log(LOG_INFO , "i am a error %s", e.what());
//         }

	
// }


void MultiOutputWidget::switchToDashboard() {
    dashboardManager.uid = authManager.uid;
    dashboardManager.key = authManager.key;
    QWidget* dashboardWidget = dashboardManager.handleTab();
     while (stackedWidget_.count() > 0) {
        QWidget* widget = stackedWidget_.widget(0);
        stackedWidget_.removeWidget(widget);
        delete widget;
    };
    stackedWidget_.addWidget(dashboardWidget);
    stackedWidget_.setCurrentWidget(dashboardWidget);
};

void MultiOutputWidget::logOut() {
     auto profiledir = obs_frontend_get_current_profile_path();
    if (profiledir) {
        std::string filename = profiledir;
        filename += "/obs-multi-rtmp_auth.json";

        // Delete the file
        std::filesystem::remove(filename);
    }

    bfree(profiledir);
    QWidget* authTabWidget = authManager.handleAuthTab();
    stackedWidget_.addWidget(authTabWidget);
    stackedWidget_.setCurrentWidget(authTabWidget);
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-multi-rtmp", "en-US")
OBS_MODULE_AUTHOR("雷鳴 (@sorayukinoyume)")

AuthManager authmanager;

bool obs_module_load()
{
    auto mainwin = (QMainWindow*)obs_frontend_get_main_window();
   
    

    if (mainwin == nullptr)
        return false;
    QMetaObject::invokeMethod(mainwin, []() {
        s_service.uiThread_ = QThread::currentThread();
    });
    
    auto dock = new MultiOutputWidget();
    dock->setObjectName("obs-multi-rtmp-dock");
    if (!obs_frontend_add_dock_by_id("obs-multi-rtmp-dock", "Multistream by Streamway", dock))
    {
        delete dock;
        return false;
    }

    blog(LOG_INFO, TAG "version: %s by SoraYuki https://github.com/sorayuki/obs-multi-rtmp/", PLUGIN_VERSION);



    obs_frontend_add_event_callback(
        [](enum obs_frontend_event event, void *private_data) {

          
            auto dock = static_cast<Dashboard*>(private_data);


            
            if((authmanager.*&AuthManager::isAuthenticated)()){
                for(auto x: dock->GetAllPushWidgets())
                x->OnOBSEvent(event);
            }
            

            if (event == obs_frontend_event::OBS_FRONTEND_EVENT_EXIT)
            {   
                dock->SaveConfig();
               auto profiledir = obs_frontend_get_current_profile_path();

if (profiledir) {
    std::string filename = profiledir;
    filename += "/obs-multi-rtmp_auth.json";

    // Read existing JSON content from the file
    std::ifstream inFile(filename);
    nlohmann::json configJson;

    if (inFile.is_open()) {
        inFile >> configJson;
        inFile.close();

        // Check if "current_broadcast" exists
        if (configJson.contains("current_broadcast")) {
            // Access the existing "current_broadcast" object
            nlohmann::json &currentBroadcastObj = configJson["current_broadcast"];

            // Check if "id" exists
            if (currentBroadcastObj.contains("id")) {
                // Update the "status" field
                currentBroadcastObj["status"] = "ready";
            }

            // Convert the updated JSON to a string
            std::string content = configJson.dump();

            // Write the updated content back to the file
            os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
        }
    }

    bfree(profiledir);
}

            }
            else if (event == obs_frontend_event::OBS_FRONTEND_EVENT_PROFILE_CHANGED)
            {
                dock->LoadConfig();
            }
            // else if (event ==
			// 		obs_frontend_event::
			// 		OBS_FRONTEND_EVENT_STREAMING_STARTING){
			// 			MultiOutputWidget myWidget;
			// 			myWidget.startStreamingListner(); // Call the member function using the object
			// 		}
        }, dock
    );
    

    return true;
}

const char *obs_module_description(void)
{
    return "Multiple RTMP Output Plugin";
}
