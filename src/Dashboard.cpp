#include "pch.h"

#include <list>
#include <regex>
#include <filesystem>
#include <vector>
#include "push-widget.h"

#include "output-config.h"
#include "Dashboard.h"
using Json = nlohmann::json;

Dashboard::Dashboard() {
    std::cout<<"Dashboard initilize";
}

    QWidget *container_ = 0;
	QScrollArea *scroll_ = 0;
	QVBoxLayout *itemLayout_ = 0;
	QVBoxLayout *layout_ = 0;
	QVBoxLayout* tab2Layout = 0;
    std::string currentBroadcast;

void SaveConfig() { SaveMultiOutputConfig(); }

std::vector<PushWidget *> GetAllPushWidgets()
	{
		std::vector<PushWidget *> result;
		for (auto &c : container_->children()) {
			if (c->objectName() == "push-widget") {
				auto w = dynamic_cast<PushWidget *>(c);
				result.push_back(w);
			}
		}
		return result;
	}

void LoadConfig() {
	 // Clear previous pushwidgets from itemLayout_
    while (QLayoutItem *item = itemLayout_->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->hide(); // Optional: Hide the widget before deleting
            delete widget;
        }
        delete item;
    };

    

	
    for (auto x : GetAllPushWidgets()) {
        delete x;
    }
    GlobalMultiOutputConfig() = {};

    if (LoadMultiOutputConfig() == false) {
        ImportLegacyMultiOutputConfig();
    }
	
    for (auto x : GlobalMultiOutputConfig().targets) {
        auto pushwidget = createPushWidget(x->id, container_);
        itemLayout_->addWidget(pushwidget);
	}
}


static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
  std::string *data = (std::string *)stream;
  data->append((char *)ptr, size * nmemb);
  return size * nmemb;
};

void Dashboard::handleSuccessfulLogin(const QString& uid, const QString& key, QVBoxLayout *newUiLayout , QTabWidget *tabWidget) {
	QPushButton *refreshButton = new QPushButton("Refresh");
    newUiLayout->addWidget(refreshButton);
	newUiLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 

	  QObject::connect(refreshButton, &QPushButton::clicked, [this]() {
			emit refreshBroadcasts();
	  });

	CURL *curl;
    CURLcode res;
	std::string readBuffer;
    // Initialize curl
    curl = curl_easy_init();
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, "https://testapi.streamway.in/v1/broadcasts/upcoming");

    // Set the request method to GET
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // Set authentication
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERNAME, uid.toStdString().c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, key.toStdString().c_str());
    curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);
	curl_easy_setopt(curl , CURLOPT_WRITEDATA , &readBuffer);
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
 
    // Cleanup
    curl_easy_cleanup(curl);
	curl = NULL;

	auto j3 = Json::parse(readBuffer);
	

	if(j3.is_array() && !j3.empty()){
		QJsonArray qArray;

        // Iterate through the nlohmann::json array and build the QJsonArray
        for (const auto& obj : j3) {
            QJsonObject qObj;
            
            // Iterate through each key-value pair in the JSON object
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                // Convert nlohmann::json types to QVariant and then to QJsonValue
                QVariant variantValue = QVariant::fromValue(it.value());
                QJsonValue qJsonValue = QJsonValue::fromVariant(variantValue);

                // Insert the QJsonValue into the QJsonObject
                qObj[QString::fromStdString(it.key())] = qJsonValue;
            }

            // Append the QJsonObject to the QJsonArray
            qArray.append(qObj);
        };

        QLabel* tokenLabelSize = new QLabel("Total broadcasts " + QString::number(qArray.size()));
        newUiLayout->addWidget(tokenLabelSize);


		// Create a scroll area and set up a widget to contain the items
		QScrollArea* scrollArea = new QScrollArea;
		QWidget* scrollWidget = new QWidget;
		
		// scrollWidget->resize(300,300);
		scrollArea->setWidgetResizable(true);
		scrollArea->setWidget(scrollWidget);
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		
		// Create a layout for the widget inside the scroll area
		QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
		scrollLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
		// Set the margins of the layout to zero (removing spacing)
		// scrollLayout->setContentsMargins(0, 0, 0, 0);
		for (const auto& jsonObject : j3) {
    
        	// QJsonObject jsonObject = jsonValue.toObject();
        	if (jsonObject.contains("title")) {
            // Create a custom widget to represent each item
            QWidget* itemWidget = new QWidget;
            QVBoxLayout* itemLayout = new QVBoxLayout(itemWidget);

			// Create a group for title and scheduledTime
            QGroupBox* titleScheduledGroup = new QGroupBox;
            QVBoxLayout* titleScheduledLayout = new QVBoxLayout(titleScheduledGroup);
			titleScheduledLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
			// Set a fixed size for the QGroupBox
			titleScheduledGroup->setMinimumSize(220, 110); // Set the minimum width and height

            // Show title from jsonObject
             QString title = QString::fromStdString(jsonObject["title"].get<std::string>());
			 
            QLabel* titleLabel = new QLabel(title);
			// titleLabel->setMaximumHeight(20);
			titleLabel->setWordWrap(true);
			titleLabel->setStyleSheet("QLabel{font-size: 17px;font-family: Arial;}"); 
            titleScheduledLayout->addWidget(titleLabel);
			titleScheduledLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 

            // Show scheduledTime from jsonObject
            QString scheduledTimeStr = QString::fromStdString(jsonObject["scheduledTime"].get<std::string>());
			// scheduledTimeStr.resize(10);

			// QString scheduledTimeStr = "2023-11-06T16:24";

			// Parse the input string
			QDateTime scheduledTime = QDateTime::fromString(scheduledTimeStr, "yyyy-MM-ddTHH:mm");

			// Format the datetime as per your requirement
			QString formattedTime = scheduledTime.toString("dddd, MMMM d 'at' h:mm AP");
			// QLabel* gfgf = new QLabel("scheduledTime");
            // titleScheduledLayout->addWidget(gfgf);

            QLabel* timeLabel = new QLabel(formattedTime);
			// timeLabel->setMaximumHeight(20);
            // titleScheduledLayout->addWidget(timeLabel);
				timeLabel->setStyleSheet("QLabel{font-size: 12px;font-family: Arial;}");
				titleScheduledLayout->addWidget(timeLabel);
			QPushButton* SelectButton = new QPushButton("Select");
			QObject::connect(SelectButton, &QPushButton::clicked, [jsonObject , tabWidget]() {
					currentBroadcast = jsonObject["id"].dump();
					if (currentBroadcast.front() == '"' && currentBroadcast.back() == '"') {
   						currentBroadcast = currentBroadcast.substr(1, currentBroadcast.size() - 2);
					}
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
            } 

            // Update uid and key in the existing JSON object
            configJson["current_broadcast"] = currentBroadcast;
            
			 

            // Convert the updated JSON to a string
            std::string content = configJson.dump();

			

            // Write the updated content back to the file
            os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
        }
        			bfree(profiledir);

                	auto &global = GlobalMultiOutputConfig();
				
                	global.targets.clear();
					SaveMultiOutputConfig();
					LoadConfig();
					// tab2Layout = 0;

			auto destinationsArray = jsonObject["destinations"];
			 auto firstDestination = destinationsArray.at(0);
			//  QJsonObject firstDestination = destinationsArray[0].toObject();
			 	obs_service_t *service = obs_frontend_get_streaming_service();
    			obs_data_t *settings = obs_service_get_settings(service);
    			// cout << obs_data_get_json_pretty(settings) << endl;
				QString url = firstDestination["url"].get<std::string>().c_str();

				// Remove the trailing slash if it exists
				if (!url.isEmpty() && url.endsWith('/')) {
    				url.chop(1); // Remove the last character (which is '/')
				}

        		QString key = firstDestination["key"].get<std::string>().c_str();
				obs_data_set_string(settings, "key", key.toStdString().c_str());
				obs_data_set_string(settings, "server", url.toStdString().c_str());
    			obs_data_release(settings);

				
					for (size_t i = 1; i < destinationsArray.size(); ++i) {
            auto destination = destinationsArray.at(i);

            QString platformUserName = destination["platformUserName"].get<std::string>().c_str();
            QString platformTitle = destination["platformTitle"].get<std::string>().c_str();
            QString title = platformUserName + " / " + platformTitle;

            auto newid = GenerateId(global);
            auto target = std::make_shared<OutputTargetConfig>();
            target->id = newid;
            target->name = title.toStdString();
            target->serviceParam = {
                {"server", destination["url"].get<std::string>()},
                {"key", destination["key"].get<std::string>()}
            };
            target->syncStart = true;
            global.targets.emplace_back(target);

            auto pushwidget = createPushWidget(newid, container_);
            itemLayout_->addWidget(pushwidget);
            SaveConfig();
        }
				
				tabWidget->setCurrentIndex(1);
    		});

		
			titleScheduledLayout->addWidget(SelectButton);

			// Add the title and scheduledTime group to the item layout
            itemLayout->addWidget(titleScheduledGroup);

			itemLayout->setContentsMargins(0, 0, 0, 0);

            // Add the custom widget to the scroll layout
            scrollLayout->addWidget(itemWidget);
        }


   
}

		// Add the scroll area to your main layout or window
		newUiLayout->addWidget(scrollArea);

	}else{
			auto label = new QLabel(
				u8"No Broadcast Scheduled");
				label->setMaximumHeight(20);
			newUiLayout->addWidget(label);
	}
 		

// Create a horizontal layout for Add and Cancel buttons
QHBoxLayout* buttonLayout = new QHBoxLayout;
// Create Cancel and Add buttons outside of the loop
// QPushButton* cancelButton = new QPushButton("Cancel");
QPushButton* addButton = new QPushButton("Schedule New Broadcast");
addButton->setMaximumHeight(20);


// Add Cancel and Add buttons to the layout or wherever appropriate
// buttonLayout->addWidget(cancelButton);
buttonLayout->addWidget(addButton);
QObject::connect(addButton, &QPushButton::clicked,
				 []() {
        // Open the URL in the user's default web browser
        QDesktopServices::openUrl(QUrl("https://app.streamway.in/"));
				 });
// Create a container widget for the button layout
QWidget* buttonContainer = new QWidget;
buttonContainer->setLayout(buttonLayout);
newUiLayout->addWidget(buttonContainer);

}


// Function to create Tab 1 and its content
QWidget* Dashboard::createTab1(const QString& uid, const  QString& key , QTabWidget *tabWidget) {
    QWidget* tab1 = new QWidget;
    QVBoxLayout* tab1Layout = new QVBoxLayout(tab1);
	
	handleSuccessfulLogin(uid , key , tab1Layout , tabWidget);
    return tab1;
};

// Function to create Tab 2 and its content
QWidget* Dashboard::createTab2( const QString& uid, const QString& key , QTabWidget *tabWidget) {
	// QScrollArea* scrollArea = new QScrollArea;
    QWidget* tab2 = new QWidget;
    tab2Layout = new QVBoxLayout(tab2);
	tab2Layout->setContentsMargins(0, 0, 0, 0); // Set all margins to zero
	// tab2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	tab2Layout->setAlignment(Qt::AlignmentFlag::AlignTop); 
	// init widget
		auto addButton = new QPushButton(
			obs_module_text("Btn.NewTarget"), container_);
		QObject::connect(addButton, &QPushButton::clicked, []() {
            auto& global = GlobalMultiOutputConfig();
            auto newid = GenerateId(global);
            auto target = std::make_shared<OutputTargetConfig>();
            target->id = newid;
            global.targets.emplace_back(target);
            auto pushwidget = createPushWidget(newid, container_);
            itemLayout_->addWidget(pushwidget);
            if (pushwidget->ShowEditDlg())
                SaveConfig();
            else {
                auto it = std::find_if(global.targets.begin(), global.targets.end(), [newid](auto& x) {
                    return x->id == newid;
                });
                if (it != global.targets.end())
                    global.targets.erase(it);
                delete pushwidget;
            }
        });
		tab2Layout->addWidget(addButton);
		
		// start all, stop all
		auto allBtnContainer = new QWidget();
		
		allBtnContainer->setMaximumHeight(70);
		// allBtnContainer->setStyleSheet("background-color: lightblue;"); // Change color as needed
		auto allBtnLayout = new QHBoxLayout(allBtnContainer);
		allBtnLayout->setAlignment(Qt::AlignmentFlag::AlignTop);
		auto startAllButton = new QPushButton(
			obs_module_text("Btn.StartAll"), allBtnContainer);
		allBtnLayout->addWidget(startAllButton);
		auto stopAllButton = new QPushButton(
			obs_module_text("Btn.StopAll"), allBtnContainer);
		allBtnLayout->addWidget(stopAllButton);
		allBtnContainer->setLayout(allBtnLayout);
		tab2Layout->addWidget(allBtnContainer);
		auto endAllBroadcastButton = new QPushButton("End Broadcast & Stop All");
		tab2Layout->addWidget(endAllBroadcastButton);
		// endAllBroadcastButton->setEnabled(false);
		

		QObject::connect(endAllBroadcastButton, &QPushButton::clicked, [this, uid, key, tabWidget]() {
    		try {
        obs_frontend_streaming_stop();
        CURL *curl;
        curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        std::string url = "https://testapi.streamway.in/v1/broadcasts/" + currentBroadcast;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // Set authentication
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERNAME, uid.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, key.toStdString().c_str());

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: */*");
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode ret = curl_easy_perform(curl);
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        // Check for errors
        if (response_code != 200) {
            curl_easy_cleanup(curl);
            // return false;
        }

        // Cleanup
        curl_easy_cleanup(curl);
        curl = NULL;
        // emit refreshBroadcasts();

        tabWidget->setCurrentIndex(0);
    } catch (const std::exception& e) {
        // Handle the exception
        // Log or display an error message
        qDebug() << "Exception occurred: " << e.what();
    } catch (...) {
        // Catch any other unexpected exceptions
        qDebug() << "Unknown exception occurred" ;
    }
});



		QObject::connect(startAllButton, &QPushButton::clicked,
				 [ uid , key , endAllBroadcastButton]() {
					 
					 endAllBroadcastButton->setEnabled(true);
					 for (auto x : GetAllPushWidgets())
						 x->StartStreaming();

					obs_frontend_streaming_start();
				 });
		QObject::connect(stopAllButton, &QPushButton::clicked,
				 []() {
					 for (auto x : GetAllPushWidgets())
						 x->StopStreaming();
				 });

		// load config
		itemLayout_ = new QVBoxLayout(tab2);
		LoadConfig();
		tab2Layout->addLayout(itemLayout_);

    return tab2;
};

// Function to create Tab 1 and its content
QWidget* Dashboard::createTab3() {
QWidget* tab3 = new QWidget;

QVBoxLayout* tab3Layout = new QVBoxLayout(tab3);
tab3Layout->setAlignment(Qt::AlignmentFlag::AlignTop); 
		auto logOutButton = new QPushButton("LogOut" , tab3);
		tab3Layout->addWidget(logOutButton);
		QObject::connect(logOutButton, &QPushButton::clicked,
				 [this]() {
				emit logOutDashboard();

 });

QString quote = "Credit -> obs-multi-rtmp plugin by sorayuki\n"
				"This plugin is an extention of obs-multi-rtmp and it intents to empower users to multistream from within OBS \n itself. "
				"Purpose of plugin is to let user schedule event from within the OBS studio itself rathar than opening \n multiple tabs of diffrent social platforms like youtube and facebook and manually \n copy pasting the keys etc. "
				"\n https://github.com/sorayuki/obs-multi-rtmp";
			

QLabel* quoteLabel = new QLabel(quote);
tab3Layout->addWidget(quoteLabel);
quoteLabel->setWordWrap(true);

// Add the quote label to the layout or widget where you want to display it
tab3Layout->addWidget(quoteLabel);
    return tab3;
};

QWidget* Dashboard::handleTab() {
	// Create a QTabWidget to hold the tabs

        container_ = new QWidget();
		
        layout_ = new QVBoxLayout(container_);
        layout_->setAlignment(Qt::AlignmentFlag::AlignTop); 

	QTabWidget* tabWidget = new QTabWidget;
	QWidget* tab1 = createTab1(uid, key , tabWidget);
    QWidget* tab2 = createTab2(uid,  key , tabWidget);
    QWidget* tab3 = createTab3();

    // Set size policies to prevent unnecessary space
    // tabWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    // tab2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	// container_->setMaximumSize(250, 370);
    tab3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	 // Set fixed sizes for tabs
    // tab1->setFixedSize(200, 50); // Set width and height for tab1
    // tab2->setMinimumSize(100, 200); // Set minimum width and height for tab2
    tab3->setMaximumSize(250, 270); // Set maximum width and height for tab3

	 // Set background color for each tab
    // container_->setStyleSheet("background-color: lightblue;"); // Change color as needed
    // tab2->setStyleSheet("background-color: lightgreen;"); // Change color as needed
    // tab3->setStyleSheet("background-color: lightyellow;"); // Change color as needed

    // Add tabs to the QTabWidget
    tabWidget->addTab(tab1, "Broadcasts");
    tabWidget->addTab(tab2, "Go Live");
    tabWidget->addTab(tab3, "About");
	

    return tabWidget;
};


