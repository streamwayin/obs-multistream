#include "pch.h"
#include "plugin-support.h"
#include <list>
#include <regex>
#include <filesystem>
#include <vector>
#include "push-widget.h"
#include "QFrame"
#include "output-config.h"
#include "Dashboard.h"
using Json = nlohmann::json;

Dashboard::Dashboard()
{
	std::cout << "Dashboard initilize";
}

QWidget *container_ = 0;
QScrollArea scroll_;
QVBoxLayout *itemLayout_ = 0;
QVBoxLayout *layout_ = 0;
QVBoxLayout *tab2Layout = 0;
std::string currentBroadcastId;
nlohmann::json currentBroadcast;
QTabWidget *tabWidget;

void removeCurrentBroadcastField()
{
	auto profiledir = obs_frontend_get_current_profile_path();
	if (profiledir) {
		QString filename = QString::fromStdString(
			std::string(profiledir) + "/obs-multi-rtmp_auth.json");
		QFile file(filename);

		if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
			QByteArray jsonData = file.readAll();
			QJsonDocument jsonDoc(
				QJsonDocument::fromJson(jsonData));

			if (!jsonDoc.isNull() && jsonDoc.isObject()) {
				QJsonObject jsonObject = jsonDoc.object();

				if (jsonObject.contains("current_broadcast")) {
					jsonObject.remove(
						"current_broadcast"); // Remove the "current_broadcast" field

					// Clear the file content
					file.resize(0);

					// Write the modified JSON back to the file
					QJsonDocument newDoc(jsonObject);
					file.write(newDoc.toJson());
				}
			}

			file.close();
		}
		bfree(profiledir);
	}
}

void getCurrentBroadcast()
{

	auto profiledir = obs_frontend_get_current_profile_path();
	if (profiledir) {
		QString filename = QString::fromStdString(
			std::string(profiledir) + "/obs-multi-rtmp_auth.json");
		QFile file(filename);

		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QByteArray jsonData = file.readAll();
			QJsonDocument jsonDoc(
				QJsonDocument::fromJson(jsonData));

			if (!jsonDoc.isNull() && jsonDoc.isObject()) {
				QJsonObject jsonObject = jsonDoc.object();

				if (jsonObject.contains("current_broadcast")) {
					QJsonValue currentBroadcastValue =
						jsonObject["current_broadcast"];

					if (currentBroadcastValue.isObject()) {
						QJsonObject currentBroad =
							currentBroadcastValue
								.toObject();

						if (currentBroad.contains(
							    "id")) {
							QString idValue =
								currentBroad["id"]
									.toString();
							currentBroadcast["id"] =
								idValue.toStdString();
							currentBroadcastId =
								currentBroadcast["id"]
									.dump();
							if (currentBroadcastId
									    .front() ==
								    '"' &&
							    currentBroadcastId
									    .back() ==
								    '"') {
								currentBroadcastId =
									currentBroadcastId
										.substr(1,
											currentBroadcastId
													.size() -
												2);
							}
						}

						if (currentBroad.contains(
							    "status")) {
							QString idValue =
								currentBroad["status"]
									.toString();
							currentBroadcast["status"] =
								idValue.toStdString();
						} else {
							currentBroadcast
								["status"] =
									"ready";
						}
					}
				}
			}

			file.close();
		}
		bfree(profiledir);
	}
};

bool isServerBaseEnabled() {
    auto profiledir = obs_frontend_get_current_profile_path();

    try {
        if (profiledir) {
            std::string filename = profiledir;
            filename += "/obs-multi-rtmp_auth.json";

            // Read existing JSON content from the file
            std::ifstream inFile(filename);
            nlohmann::json configJson;

            if (inFile.is_open()) {
                inFile >> configJson;
                inFile.close();

                // Check if "serverBase" key exists in the JSON object
                if (configJson.contains("serverBase")) {
                    // Return the boolean value of "serverBase"
                    return configJson["serverBase"].get<bool>();
                } else {
                    // If "serverBase" key is not present, return false (or handle accordingly)
                    return false;
                }
            } else {
                // If the file cannot be opened, return false (or handle accordingly)
                return false;
            }
        }
    } catch (const std::exception& e) {
        // Handle any exceptions that may occur during JSON parsing
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // If any errors occurred, return false
    return false;
}


void Dashboard::SaveConfig()
{
	SaveMultiOutputConfig();
}

std::vector<PushWidget *> Dashboard::GetAllPushWidgets()
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

void Dashboard::LoadConfig()
{
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

static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	std::string *data = (std::string *)stream;
	data->append((char *)ptr, size * nmemb);
	return size * nmemb;
};

QWidget *Dashboard::serverBaseStreaming(const QString& uid, const QString& key, QTabWidget* tabWidget , std::string id , bool isLive)
{
    // emit refreshBroadcasts();
    getCurrentBroadcast();
    QWidget* parent = new QWidget;

    QVBoxLayout* parentLayout = new QVBoxLayout(parent);
	parentLayout->setAlignment(Qt::AlignmentFlag::AlignTop);
    // Create buttons
    QPushButton* startButton = new QPushButton("Start", parent);
    QPushButton* stopButton = new QPushButton("Stop", parent);
    QPushButton* endBroadcastButton = new QPushButton("End", parent);
	 QPushButton* selectButton = new QPushButton("Select", parent);
	startButton->setVisible(false);
			endBroadcastButton->setVisible(false);
			stopButton->setVisible(false);

    // Initially, show only the start button
	if(isLive){
		if(currentBroadcast["id"]==id){
			startButton->setEnabled(false);
			endBroadcastButton->setEnabled(true);
			stopButton->setEnabled(true);
		}else{
			startButton->setEnabled(false);
			endBroadcastButton->setEnabled(false);
			stopButton->setEnabled(false);
		}
		
	}
    

    // Create a horizontal layout for start/stop buttons
    QHBoxLayout* startStopLayout = new QHBoxLayout;
    // startStopLayout->addWidget(startButton);
    // startStopLayout->addWidget(stopButton);
    // startStopLayout->addWidget(endBroadcastButton);
	startStopLayout->addWidget(selectButton);
    // Add buttons and layouts to the parent layout
    parentLayout->addLayout(startStopLayout);
	parentLayout->setContentsMargins(0, 0, 0, 0);


	QObject::connect(selectButton, &QPushButton::clicked, [this , startButton, stopButton, endBroadcastButton , id , selectButton , tabWidget]() {
				currentBroadcastId = id;
					if (currentBroadcastId.front() == '"' &&
					    currentBroadcastId.back() == '"') {
						currentBroadcastId =
							currentBroadcastId.substr(
								1,
								currentBroadcastId
										.size() -
									2);
					}

					auto profiledir =
						obs_frontend_get_current_profile_path();

					if (profiledir) {
						std::string filename =
							profiledir;
						filename +=
							"/obs-multi-rtmp_auth.json";

						// Read existing JSON content from the file
						std::ifstream inFile(filename);
						nlohmann::json configJson;

						if (inFile.is_open()) {
							inFile >> configJson;
							inFile.close();
						}

						// Create an object for the current broadcast with id and status
						nlohmann::json
							currentBroadcastObj;
						currentBroadcastObj["id"] =
							currentBroadcastId;
						currentBroadcastObj["status"] =
							"live";

						// Update uid and key in the existing JSON object
						configJson["current_broadcast"] =
							currentBroadcastObj;

						// Convert the updated JSON to a string
						std::string content =
							configJson.dump();

						// Write the updated content back to the file
						os_quick_write_utf8_file_safe(
							filename.c_str(),
							content.c_str(),
							content.size(), true,
							"tmp", "bak");
					}
					bfree(profiledir);

					auto &global =
						GlobalMultiOutputConfig();

					global.targets.clear();
					SaveMultiOutputConfig();
					LoadConfig();
					

					
					//  QJsonObject firstDestination = destinationsArray[0].toObject();
					obs_service_t *service =
						obs_frontend_get_streaming_service();
					obs_data_t *settings =
						obs_service_get_settings(
							service);
					// cout << obs_data_get_json_pretty(settings) << endl;
					std::string url = "rtmp://injest.streamway.in/LiveApp";

					
					obs_data_set_string(
						settings, "key",
						id.c_str());
					// obs_data_set_string(
					// 	settings, "service",
					// 	"Custom...");	
					obs_data_set_string(
						settings, "server",
						url.c_str());
					obs_data_set_string(
						settings, "server",
						url.c_str());	
					obs_data_release(settings);
					obs_service_update(service,settings);
			
		tabWidget->setCurrentIndex(1);
        
    });
	
    // Connect button signals to slots
    QObject::connect(startButton, &QPushButton::clicked, [this , startButton, stopButton, endBroadcastButton ,id]() {
        // When Start button is clicked, hide Start and show Stop and End Broadcast
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        endBroadcastButton->setEnabled(true);



		obs_frontend_streaming_start();
        emit refreshBroadcasts();
    });

    QObject::connect(stopButton, &QPushButton::clicked, [startButton, stopButton, endBroadcastButton]() {
        // When Stop button is clicked, hide Stop and show Start, hide End Broadcast
		stopButton->setEnabled(false);
		startButton->setEnabled(true);
        
		obs_frontend_streaming_stop();
        // Add your logic for stopping the streaming here
    });

    QObject::connect(endBroadcastButton, &QPushButton::clicked, [this , startButton, stopButton, endBroadcastButton , uid , key]() {
        // Add your logic for ending the broadcast here
		try {
				obs_frontend_streaming_stop();

				CURL *curl;
				curl = curl_easy_init();

				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
						 "POST");
				std::string url =
					"https://api.streamway.in/v1/broadcasts/" +
					currentBroadcastId;
			
				curl_easy_setopt(curl, CURLOPT_URL,
						 url.c_str());
				// Set authentication
				curl_easy_setopt(curl, CURLOPT_HTTPAUTH,
						 CURLAUTH_BASIC);
				curl_easy_setopt(curl, CURLOPT_USERNAME,
						 uid.toStdString().c_str());
				curl_easy_setopt(curl, CURLOPT_PASSWORD,
						 key.toStdString().c_str());

				struct curl_slist *headers = NULL;
				headers = curl_slist_append(headers,
							    "Accept: */*");
				headers = curl_slist_append(
					headers,
					"Content-Type: application/json");

				curl_easy_setopt(curl, CURLOPT_HTTPHEADER,
						 headers);

				CURLcode ret = curl_easy_perform(curl);
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
						  &response_code);
				// Check for errors
				if (response_code != 200) {
					curl_easy_cleanup(curl);
					// return false;
				}

				// Cleanup
				curl_easy_cleanup(curl);
				curl = NULL;

				

				auto &global = GlobalMultiOutputConfig();

				global.targets.clear();
				SaveMultiOutputConfig();
				LoadConfig();
				removeCurrentBroadcastField();
				currentBroadcast["status"] = "ready";
				startButton->setEnabled(true);
				endBroadcastButton->setEnabled(true);
				stopButton->setEnabled(true); 
				emit refreshBroadcasts();

			} catch (const std::exception &e) {
				// Handle the exception
				// Log or display an error message
				obs_log(LOG_INFO, "i am a error %s", e.what());
				qDebug() << "Exception occurred: " << e.what();
			} catch (...) {
				// Catch any other unexpected exceptions
				qDebug() << "Unknown exception occurred";
			}
    });

    return parent;
}


void Dashboard::handleSuccessfulLogin(const QString &uid, const QString &key,
				      QVBoxLayout *newUiLayout,
				      QTabWidget *tabWidget)
{
QPushButton *refreshButton = new QPushButton("Refresh");
// newUiLayout->addWidget(refreshButton);
newUiLayout->setAlignment(Qt::AlignmentFlag::AlignTop);

QObject::connect(refreshButton, &QPushButton::clicked, [this]() { emit refreshBroadcasts(); });

// Create a horizontal layout for Add and Cancel buttons
QHBoxLayout *buttonLayout = new QHBoxLayout;

// Create Cancel and Add buttons outside of the loop
// QPushButton* cancelButton = new QPushButton("Cancel");
QPushButton *addButton = new QPushButton("Schedule New");

// Add both buttons to the horizontal layout
buttonLayout->addWidget(addButton);
buttonLayout->addWidget(refreshButton);

QObject::connect(addButton, &QPushButton::clicked, []() {
    // Open the URL in the user's default web browser
    QDesktopServices::openUrl(QUrl("https://app.streamway.in/"));
});

// Create a container widget for the button layout
QWidget *buttonContainer = new QWidget;
buttonContainer->setLayout(buttonLayout);

// Add the button container to the main layout
newUiLayout->addWidget(buttonContainer);



try{
	CURL *curl;
	CURLcode res;
	std::string readBuffer;
	// Initialize curl
	curl = curl_easy_init();
	// Set the URL
	curl_easy_setopt(curl, CURLOPT_URL,
			 "https://api.streamway.in/v1/broadcasts/upcoming");

	// Set the request method to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

	// Set authentication
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERNAME, uid.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, key.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
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

	if (j3.is_array() && !j3.empty()) {
		QJsonArray qArray;

		// Iterate through the nlohmann::json array and build the QJsonArray
		for (const auto &obj : j3) {
			QJsonObject qObj;

			// Iterate through each key-value pair in the JSON object
			for (auto it = obj.begin(); it != obj.end(); ++it) {
				// Convert nlohmann::json types to QVariant and then to QJsonValue
				QVariant variantValue =
					QVariant::fromValue(it.value());
				QJsonValue qJsonValue =
					QJsonValue::fromVariant(variantValue);

				// Insert the QJsonValue into the QJsonObject
				qObj[QString::fromStdString(it.key())] =
					qJsonValue;
			}

			// Append the QJsonObject to the QJsonArray
			qArray.append(qObj);
		};

		QLabel *tokenLabelSize = new QLabel(
			"Total broadcasts " + QString::number(qArray.size()));
		newUiLayout->addWidget(tokenLabelSize);

		// Create a scroll area and set up a widget to contain the items
		QScrollArea *scrollArea = new QScrollArea;
		QWidget *scrollWidget = new QWidget;

		// scrollWidget->resize(300,300);
		scrollArea->setWidgetResizable(true);
		scrollArea->setWidget(scrollWidget);
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

		// Create a layout for the widget inside the scroll area
		QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
		scrollLayout->setAlignment(Qt::AlignmentFlag::AlignTop);
		// Set the margins of the layout to zero (removing spacing)
		// scrollLayout->setContentsMargins(0, 0, 0, 0);
		for (const auto &jsonObject : j3) {

			// QJsonObject jsonObject = jsonValue.toObject();
			if (jsonObject.contains("title")) {
				// Create a custom widget to represent each item
				QWidget *itemWidget = new QWidget;
				QVBoxLayout *itemLayout =
					new QVBoxLayout(itemWidget);

				itemLayout->setAlignment(
					Qt::AlignmentFlag::AlignTop);

				// Create a group for title and scheduledTime
				QGroupBox *titleScheduledGroup = new QGroupBox;
				QVBoxLayout *titleScheduledLayout =
					new QVBoxLayout(titleScheduledGroup);
				titleScheduledLayout->setAlignment(
					Qt::AlignmentFlag::AlignTop);
				// Set a fixed size for the QGroupBox
				titleScheduledGroup->setMinimumSize(
					220,
					110); // Set the minimum width and height

				// Show title from jsonObject
				QString title = QString::fromStdString(
					jsonObject["title"].get<std::string>());

				QLabel *titleLabel = new QLabel(title);
				// titleLabel->setMaximumHeight(20);
				titleLabel->setWordWrap(true);
				titleLabel->setStyleSheet(
					"QLabel{font-size: 17px;font-family: Arial;}");
				titleScheduledLayout->addWidget(titleLabel);
				titleScheduledLayout->setAlignment(
					Qt::AlignmentFlag::AlignTop);

				// Show scheduledTime from jsonObject
				QString scheduledTimeStr =
					QString::fromStdString(
						jsonObject["scheduledTime"]
							.get<std::string>());
				// scheduledTimeStr.resize(10);

				// QString scheduledTimeStr = "2023-11-06T16:24";

				// Parse the input string
				QDateTime scheduledTime = QDateTime::fromString(
					scheduledTimeStr, "yyyy-MM-ddTHH:mm");

				// Format the datetime as per your requirement
				QString formattedTime = scheduledTime.toString(
					"dddd, MMMM d 'at' h:mm AP");
				// QLabel* gfgf = new QLabel("scheduledTime");
				// titleScheduledLayout->addWidget(gfgf);

				QLabel *timeLabel = new QLabel(formattedTime);
				// timeLabel->setMaximumHeight(20);
				// titleScheduledLayout->addWidget(timeLabel);
				timeLabel->setStyleSheet(
					"QLabel{font-size: 12px;font-family: Arial;}");
				titleScheduledLayout->addWidget(timeLabel);
				QPushButton *SelectButton =
					new QPushButton("Select");


				QObject::connect(SelectButton, &QPushButton::clicked, [this, jsonObject, tabWidget]() {

					currentBroadcastId =
						jsonObject["id"].dump();
					if (currentBroadcastId.front() == '"' &&
					    currentBroadcastId.back() == '"') {
						currentBroadcastId =
							currentBroadcastId.substr(
								1,
								currentBroadcastId
										.size() -
									2);
					}

					auto profiledir =
						obs_frontend_get_current_profile_path();

					if (profiledir) {
						std::string filename =
							profiledir;
						filename +=
							"/obs-multi-rtmp_auth.json";

						// Read existing JSON content from the file
						std::ifstream inFile(filename);
						nlohmann::json configJson;

						if (inFile.is_open()) {
							inFile >> configJson;
							inFile.close();
						}

						// Create an object for the current broadcast with id and status
						nlohmann::json
							currentBroadcastObj;
						currentBroadcastObj["id"] =
							currentBroadcastId;
						currentBroadcastObj["status"] =
							"ready";

						// Update uid and key in the existing JSON object
						configJson["current_broadcast"] =
							currentBroadcastObj;

						// Convert the updated JSON to a string
						std::string content =
							configJson.dump();

						// Write the updated content back to the file
						os_quick_write_utf8_file_safe(
							filename.c_str(),
							content.c_str(),
							content.size(), true,
							"tmp", "bak");
					}
					bfree(profiledir);

					auto &global =
						GlobalMultiOutputConfig();

					global.targets.clear();
					SaveMultiOutputConfig();
					LoadConfig();
					// tab2Layout = 0;

					auto destinationsArray =
						jsonObject["destinations"];
					auto firstDestination =
						destinationsArray.at(0);
					//  QJsonObject firstDestination = destinationsArray[0].toObject();
					obs_service_t *service =
						obs_frontend_get_streaming_service();
					obs_data_t *settings =
						obs_service_get_settings(
							service);
					// cout << obs_data_get_json_pretty(settings) << endl;
					QString url =
						firstDestination["url"]
							.get<std::string>()
							.c_str();

					// Remove the trailing slash if it exists
					if (!url.isEmpty() &&
					    url.endsWith('/')) {
						url.chop(
							1); // Remove the last character (which is '/')
					}

					QString key =
						firstDestination["key"]
							.get<std::string>()
							.c_str();
					obs_data_set_string(
						settings, "key",
						key.toStdString().c_str());
					
					obs_data_set_string(
						settings, "server",
						url.toStdString().c_str());
					
					obs_data_release(settings);
					obs_service_update(service,settings);
					if (destinationsArray.size() != 1) {
						for (size_t i = 1;
						     i <
						     destinationsArray.size();
						     ++i) {
							auto destination =
								destinationsArray
									.at(i);

							QString platformUserName =
								destination["platformUserName"]
									.get<std::string>()
									.c_str();
							QString platformTitle =
								destination["platformTitle"]
									.get<std::string>()
									.c_str();
							QString title =
								platformUserName +
								" - " +
								platformTitle;

							auto newid = GenerateId(
								global);
							auto target = std::make_shared<
								OutputTargetConfig>();
							target->id = newid;
							target->name =
								title.toStdString();
							target->serviceParam = {
								{"server",
								 destination["url"]
									 .get<std::string>()},
								{"key",
								 destination["key"]
									 .get<std::string>()},
								{"use_auth",
								 false}};
							target->outputParam = {
								{"bind_ip",
								 "default"},
								{"drop_threshold_ms",
								 700},
								{"low_latency_mode_enabled",
								 false},
								{"max_shutdown_time_sec",
								 30},
								{"new_socket_loop_enabled",
								 false},
								{"pframe_drop_threshold_ms",
								 900}};
							target->syncStart =
								true;
							global.targets
								.emplace_back(
									target);

							auto pushwidget =
								createPushWidget(
									newid,
									container_);
							itemLayout_->addWidget(
								pushwidget);
							SaveConfig();
						}
					}

					tabWidget->setCurrentIndex(1);
				});

				if(isServerBaseEnabled()){
					QWidget* serverBaseStreamingWidget;

					// Create and add the serverBaseStreaming widget
					if(currentBroadcast["status"]=="live"){
						serverBaseStreamingWidget = serverBaseStreaming(uid, key, tabWidget , jsonObject["id"] , true);
					}else{
						serverBaseStreamingWidget = serverBaseStreaming(uid, key, tabWidget , jsonObject["id"] , false);
					}
					
					titleScheduledLayout->addWidget(serverBaseStreamingWidget);
				}else{
					titleScheduledLayout->addWidget(SelectButton);
				}

				// Add the title and scheduledTime group to the item layout
				itemLayout->addWidget(titleScheduledGroup);

				itemLayout->setContentsMargins(0, 0, 0, 0);

				// Add the custom widget to the scroll layout
				scrollLayout->addWidget(itemWidget);
			}
		}

		// Add the scroll area to your main layout or window
		newUiLayout->addWidget(scrollArea);

	} else {
		auto label = new QLabel(u8"No Broadcast Scheduled");
		label->setMaximumHeight(20);
		newUiLayout->addWidget(label);
	}

}catch(const std::exception& e){
            obs_log(LOG_INFO , "i am a error %s", e.what());
        }
}

// Function to create Tab 1 and its content
QWidget *Dashboard::createTab1(const QString &uid, const QString &key,
			       QTabWidget *tabWidget)
{
	// emit refreshBroadcasts();
	getCurrentBroadcast();
	QWidget *tab1 = new QWidget;
	QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);

	handleSuccessfulLogin(uid, key, tab1Layout, tabWidget);
	return tab1;
};

// Function to create Tab 2 and its content
QWidget *Dashboard::createTab2(const QString &uid, const QString &key,
			       QTabWidget *tabWidget)
{
	getCurrentBroadcast();
	// emit refreshBroadcasts();

	// QScrollArea* scrollArea = new QScrollArea;
	QWidget *tab2 = new QWidget;
	container_ = new QWidget(&scroll_);
	tab2Layout = new QVBoxLayout(container_);
	tab2Layout->setContentsMargins(0, 0, 0, 0); // Set all margins to zero
	// tab2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	tab2Layout->setAlignment(Qt::AlignmentFlag::AlignTop);
	// init widget
	auto addButton =
		new QPushButton(obs_module_text("Btn.NewTarget"), container_);
	QObject::connect(addButton, &QPushButton::clicked, [this]() {
		auto &global = GlobalMultiOutputConfig();
		auto newid = GenerateId(global);
		auto target = std::make_shared<OutputTargetConfig>();
		target->id = newid;
		global.targets.emplace_back(target);
		auto pushwidget = createPushWidget(newid, container_);
		itemLayout_->addWidget(pushwidget);
		if (pushwidget->ShowEditDlg())
			SaveConfig();
		else {
			auto it = std::find_if(
				global.targets.begin(), global.targets.end(),
				[newid](auto &x) { return x->id == newid; });
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
	auto startAllButton = new QPushButton(obs_module_text("Btn.StartAll"),
					      allBtnContainer);
	allBtnLayout->addWidget(startAllButton);
	auto stopAllButton = new QPushButton(obs_module_text("Btn.StopAll"),
					     allBtnContainer);
	allBtnLayout->addWidget(stopAllButton);
	allBtnContainer->setLayout(allBtnLayout);
	tab2Layout->addWidget(allBtnContainer);
	auto endAllBroadcastButton =
		new QPushButton("End Broadcast and Stop All");
	tab2Layout->addWidget(endAllBroadcastButton);
	endAllBroadcastButton->setEnabled(true);

	if (currentBroadcast["status"] == "live") {
		// Disable the button if the broadcast status is live

		startAllButton->setToolTip(
			"Another broadcast is already live Please stop it First");
	}

	QObject::connect(
		endAllBroadcastButton, &QPushButton::clicked,
		[this, uid, key, tabWidget]() {
			try {
				obs_frontend_streaming_stop();

				CURL *curl;
				curl = curl_easy_init();

				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
						 "POST");
				std::string url =
					"https://api.streamway.in/v1/broadcasts/" +
					currentBroadcastId;
				curl_easy_setopt(curl, CURLOPT_URL,
						 url.c_str());
				// Set authentication
				curl_easy_setopt(curl, CURLOPT_HTTPAUTH,
						 CURLAUTH_BASIC);
				curl_easy_setopt(curl, CURLOPT_USERNAME,
						 uid.toStdString().c_str());
				curl_easy_setopt(curl, CURLOPT_PASSWORD,
						 key.toStdString().c_str());

				struct curl_slist *headers = NULL;
				headers = curl_slist_append(headers,
							    "Accept: */*");
				headers = curl_slist_append(
					headers,
					"Content-Type: application/json");

				curl_easy_setopt(curl, CURLOPT_HTTPHEADER,
						 headers);

				CURLcode ret = curl_easy_perform(curl);
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
						  &response_code);
				// Check for errors
				if (response_code != 200) {
					curl_easy_cleanup(curl);
					// return false;
				}

				// Cleanup
				curl_easy_cleanup(curl);
				curl = NULL;

				tabWidget->setTabEnabled(0 , true);

				auto &global = GlobalMultiOutputConfig();

				global.targets.clear();
				SaveMultiOutputConfig();
				LoadConfig();
				removeCurrentBroadcastField();
				
				emit refreshBroadcasts();

			} catch (const std::exception &e) {
				// Handle the exception
				// Log or display an error message
				obs_log(LOG_INFO, "i am a error %s", e.what());
				qDebug() << "Exception occurred: " << e.what();
			} catch (...) {
				// Catch any other unexpected exceptions
				qDebug() << "Unknown exception occurred";
			}
		});

	QObject::connect(startAllButton, &QPushButton::clicked,
			 [this, uid, key, endAllBroadcastButton , tabWidget]() {
				 try {
					//  for (auto x : GetAllPushWidgets())
					// 	 x->StartStreaming();
					 obs_frontend_streaming_start();
                     tabWidget->setTabEnabled(0 , false);
                     
				 } catch (const std::exception &e) {
					 obs_log(LOG_INFO, "i am a error %s",
						 e.what());
				 };

				 //  endAllBroadcastButton->setEnabled(true);
			 });
	QObject::connect(stopAllButton, &QPushButton::clicked, [this , tabWidget]() {
		for (auto x : GetAllPushWidgets())
			x->StopStreaming();
		obs_frontend_streaming_stop();
        
	});

	// load config
	itemLayout_ = new QVBoxLayout(container_);
	LoadConfig();
	tab2Layout->addLayout(itemLayout_);

	return container_;
};

// Function to create Tab 1 and its content
QWidget *Dashboard::createTab3()
{
	// emit refreshBroadcasts();
	getCurrentBroadcast();
	QWidget *tab3 = new QWidget;

	QVBoxLayout *tab3Layout = new QVBoxLayout(tab3);
	tab3Layout->setAlignment(Qt::AlignmentFlag::AlignTop);
		  // Radio Input
    QCheckBox *radioButton = new QCheckBox("Server Base Streaming", tab3);
	// Set top margin (adjust the value as needed)
	QMargins margins = radioButton->contentsMargins();
	margins.setTop(10);  // Set the top margin to 10 pixels (adjust as needed)
	radioButton->setContentsMargins(margins);

	if(isServerBaseEnabled()){
		radioButton->setChecked(true);
	}else{
		radioButton->setChecked(false);
	}
	
    tab3Layout->addWidget(radioButton);

    // Save Button
    QPushButton *saveButton = new QPushButton("Save", tab3);
    tab3Layout->addWidget(saveButton);
    QObject::connect(saveButton, &QPushButton::clicked, [this, radioButton]() {
        // Handle save button click
        if (radioButton->isChecked()) {
            auto profiledir =
						obs_frontend_get_current_profile_path();

					if (profiledir) {
						std::string filename =
							profiledir;
						filename +=
							"/obs-multi-rtmp_auth.json";

						// Read existing JSON content from the file
						std::ifstream inFile(filename);
						nlohmann::json configJson;

						if (inFile.is_open()) {
							inFile >> configJson;
							inFile.close();
						}

					
						// Update uid and key in the existing JSON object
						configJson["serverBase"] =
							true;

						// Convert the updated JSON to a string
						std::string content =
							configJson.dump();

						// Write the updated content back to the file
						os_quick_write_utf8_file_safe(
							filename.c_str(),
							content.c_str(),
							content.size(), true,
							"tmp", "bak");
					}
					bfree(profiledir);
        }else{
			auto profiledir =
						obs_frontend_get_current_profile_path();

					if (profiledir) {
						std::string filename =
							profiledir;
						filename +=
							"/obs-multi-rtmp_auth.json";

						// Read existing JSON content from the file
						std::ifstream inFile(filename);
						nlohmann::json configJson;

						if (inFile.is_open()) {
							inFile >> configJson;
							inFile.close();
						}

						configJson["serverBase"] =
							false;
						

						// Convert the updated JSON to a string
						std::string content =
							configJson.dump();

						// Write the updated content back to the file
						os_quick_write_utf8_file_safe(
							filename.c_str(),
							content.c_str(),
							content.size(), true,
							"tmp", "bak");
					}
					bfree(profiledir);
		}
    
		emit refreshBroadcasts();
	});

	auto logOutButton = new QPushButton("LogOut", tab3);
	tab3Layout->addWidget(logOutButton);
	QObject::connect(logOutButton, &QPushButton::clicked, [this]() {
		emit logOutDashboard();
	});

	QString right = "This plugin is developed and maintained by streamway.in";
QLabel *rightLabel = new QLabel(right);

// Set top margin for the rightLabel
QMargins labelMargins = rightLabel->contentsMargins();
labelMargins.setTop(10);  // Set the top margin to 10 pixels (adjust as needed)
rightLabel->setContentsMargins(labelMargins);
rightLabel->setWordWrap(true);
tab3Layout->addWidget(rightLabel);


	QString quote =
		"Credit -> obs-multi-rtmp plugin by sorayuki\n"
		"This plugin is an extention of obs-multi-rtmp and it intents to empower users to multistream from within OBS \n itself. "
		"Purpose of plugin is to let user schedule event from within the OBS studio itself rathar than opening \n multiple tabs of diffrent social platforms like youtube and facebook and manually \n copy pasting the keys etc. "
		"\n https://github.com/sorayuki/obs-multi-rtmp";

	QLabel *quoteLabel = new QLabel(quote);
	tab3Layout->addWidget(quoteLabel);
	quoteLabel->setWordWrap(true);

	// Add the quote label to the layout or widget where you want to display it
	tab3Layout->addWidget(quoteLabel);



	return tab3;
};

QWidget *Dashboard::handleTab()
{
	// Create a QTabWidget to hold the tabs
	AuthManager authmanager;
	if (!authmanager.isAuthenticated()) {
		emit logOutDashboard();
	}

	getCurrentBroadcast();

	tabWidget = new QTabWidget;
	QWidget *tab1 = createTab1(uid, key, tabWidget);
	QWidget *tab2 = createTab2(uid, key, tabWidget);
	QWidget *tab3 = createTab3();

	// Set size policies to prevent unnecessary space
	// tabWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	// tab2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	// container_->setMaximumSize(250, 370);
	tab3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	// Set fixed sizes for tabs
	// tab1->setFixedSize(200, 50); // Set width and height for tab1
	// tab2->setMinimumSize(100, 200); // Set minimum width and height for tab2
	tab3->setMaximumSize(250, 350); // Set maximum width and height for tab3

	// Set background color for each tab
	// container_->setStyleSheet("background-color: lightblue;"); // Change color as needed
	// tab2->setStyleSheet("background-color: lightgreen;"); // Change color as needed
	// tab3->setStyleSheet("background-color: lightyellow;"); // Change color as needed

	// Add tabs to the QTabWidget
	tabWidget->addTab(tab1, "Broadcasts");
	tabWidget->addTab(tab2, "Go Live");
	tabWidget->addTab(tab3, "About");
    tabWidget->setTabEnabled(0 , true);
    

	tabWidget->setCurrentIndex(0);

	return tabWidget;
};



// void Dashboard::disableTab() {
//    tabWidget->setTabEnabled(0 , false);
// };