#include "pch.h"

#include <list>
#include <regex>
#include <filesystem>
#include <vector>
#include "push-widget.h"
#include "Dashboard.h"
#include "AuthManager.h"
#include "output-config.h"
#include "obs-multi-rtmp.h"

using Json = nlohmann::json;



// QString uid; // Declare uid globally
// QString key; // Declare key globally

bool AuthManager::isAuthenticated() {
    auto profiledir = obs_frontend_get_current_profile_path();
    if (profiledir) {
        QString filename = QString::fromStdString(std::string(profiledir) + "/obs-multi-rtmp_auth.json");

        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray jsonData = file.readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));

            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                QJsonObject jsonObject = jsonDoc.object();

                if (jsonObject.contains("uid_key") && jsonObject["uid_key"].isObject()) {
                    QJsonObject uidKeyObj = jsonObject["uid_key"].toObject();
                    if (uidKeyObj.contains("uid") && uidKeyObj.contains("key")) {
                        uid = uidKeyObj["uid"].toString();
                        key = uidKeyObj["key"].toString();
                        file.close();
                        bfree(profiledir);
                        return true;
                    }
                }
            }
            file.close();
        }
        bfree(profiledir);
		
    }
    return false; // Return false if authentication fails or file reading fails
}


static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
  std::string *data = (std::string *)stream;
  data->append((char *)ptr, size * nmemb);
  return size * nmemb;
};

bool sendHttpRequest( const QString& uid, const QString& key) {
    CURL *curl;
    CURLcode res;

    // Initialize curl
    curl = curl_easy_init();
    if (!curl) {
        return false;
    };

    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.streamway.in/v1/obs/version");

    // Set the request method to GET
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // Set authentication
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERNAME, uid.toStdString().c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, key.toStdString().c_str());

    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    // Check for errors
    if (response_code != 200) {
        curl_easy_cleanup(curl);
        return false;
    }

	if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }

    // Cleanup
    curl_easy_cleanup(curl);
	curl = NULL;

    return true;
}





QWidget* AuthManager::LoginWithPhoneWidget(QTabWidget *tabWidget) {
    QWidget* loginWithPhoneWidget = new QWidget;
QVBoxLayout* LoginLayout = new QVBoxLayout(loginWithPhoneWidget);
LoginLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
// QLabel* sloganLabel_ = new QLabel("Get More Views By Multistreaming Directly From OBS", loginWithPhoneWidget);
// sloganLabel_->setStyleSheet("QLabel{font-size: 14px;font-family: Arial;}");
// LoginLayout->addWidget(sloganLabel_);
// sloganLabel_->setWordWrap(true);

QWidget* scrollWidget = new QWidget;
		// scrollArea->setWidgetResizable(true);
		
		// Create a layout for the widget inside the scroll area
		QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
		scrollLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
		LoginLayout->addWidget(scrollWidget);
		scrollWidget->setVisible(false);

// Mobile Number text box
QLabel* phone_ = new QLabel("Enter Phone Number");
LoginLayout->addWidget(phone_);

auto horizontalPhoneLayout = new QHBoxLayout;
horizontalPhoneLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
QLineEdit* prePhoneLineEdit_ = new QLineEdit();
prePhoneLineEdit_->setText("+91");
prePhoneLineEdit_->setMaximumWidth(40);
horizontalPhoneLayout->addWidget(prePhoneLineEdit_);

QLineEdit* phoneLineEdit_ = new QLineEdit();
phoneLineEdit_->setValidator(new QIntValidator(0, 1000000000));
horizontalPhoneLayout->addWidget(phoneLineEdit_);

LoginLayout->addLayout(horizontalPhoneLayout);

// Add a button for getting OTP
QPushButton* getOTPButton_ = new QPushButton("Get OTP");
LoginLayout->addWidget(getOTPButton_);

// OTP input and Verify button
QLineEdit* otpLineEdit_ = new QLineEdit();
otpLineEdit_->setValidator(new QIntValidator(0, 1000));
otpLineEdit_->setPlaceholderText("Enter OTP here");

QPushButton* verifyOTPButton_ = new QPushButton("Verify");

LoginLayout->addWidget(otpLineEdit_);
LoginLayout->addWidget(verifyOTPButton_);
verifyOTPButton_->setVisible(false);
otpLineEdit_->setReadOnly(true);

QLabel* errorL = new QLabel();
LoginLayout->addWidget(errorL);
errorL->setWordWrap(true);
errorL->setStyleSheet("QLabel{font-size: 15px;font-family: Arial;color:red;}");
errorL->setVisible(false);

QString uidQString;
QString keyQString;

		QObject::connect(getOTPButton_ , &QPushButton::clicked, [scrollLayout , scrollWidget  ,phone_ , phoneLineEdit_ , getOTPButton_ , prePhoneLineEdit_ , otpLineEdit_ , verifyOTPButton_  ,errorL , tabWidget](){
			QString fullPhoneNumber_ = prePhoneLineEdit_->text() + phoneLineEdit_->text();
			
			Json jsonObject;
			jsonObject["phoneNumber"] = fullPhoneNumber_.toStdString();
			// Convert JSON object to string
    		std::string jsonData = jsonObject.dump(2);

			CURL *curl;
    		
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_URL, "https://api.streamway.in/v1/otp/obs/phone/send");
			// curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
			// curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Accept: */*");
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

			CURLcode ret = curl_easy_perform(curl);

			long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	    		if (response_code != 200) {
        			curl_easy_cleanup(curl);
					
        			QMessageBox errorMessage;
        			errorMessage.setWindowTitle("Error");
        			errorMessage.setText("This Phone Number is Not Registerd On Streamway!");
        			errorMessage.setIcon(QMessageBox::Critical);

					// Set informative text with a clickable link
					errorMessage.setInformativeText("<a href='https://app.streamway.in/signup'>SignUp Here</a>");

					// Make the text clickable
					errorMessage.setTextInteractionFlags(Qt::TextBrowserInteraction);

        			// Show the message box and wait for user interaction
					int result = errorMessage.exec();
					
					if (result == QMessageBox::Ok) {
    					// User clicked OK, redirect to the specified URL
    					QUrl url("https://app.streamway.in/signup");
    					QDesktopServices::openUrl(url);
					}
					
    			}else{
					phoneLineEdit_->setReadOnly(true);
					prePhoneLineEdit_->setReadOnly(true);
					
					verifyOTPButton_->setVisible(true);
					errorL->setVisible(false);
					getOTPButton_->setVisible(false);
					verifyOTPButton_->setVisible(true);
					otpLineEdit_->setVisible(true);
					otpLineEdit_->setReadOnly(false);
				}

	  		curl_easy_cleanup(curl);
			curl = NULL;
			
		});

		QObject::connect(verifyOTPButton_ , &QPushButton::clicked, [this , scrollLayout , scrollWidget  ,phone_ , phoneLineEdit_ , getOTPButton_ , prePhoneLineEdit_ , otpLineEdit_ , verifyOTPButton_ , errorL , tabWidget ,&uidQString , &keyQString](){
			phone_->setText("Enter OTP");
			
			QString fullPhoneNumber_ = prePhoneLineEdit_->text() + phoneLineEdit_->text();
			QString otp = otpLineEdit_->text();
			bool ok;
			int otpAsNumber = otp.toInt(&ok);
			Json jsonObject;
			jsonObject["phoneNumber"] = fullPhoneNumber_.toStdString();
			jsonObject["otp"] = otpAsNumber;

			// Convert JSON object to string
    		std::string jsonData = jsonObject.dump(2);

			CURL *curl;
    		
			std::string readBuffer;
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_URL, "https://api.streamway.in/v1/otp/obs/phone/verify");
			// curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
			// curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Accept: */*");
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
			curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);
			curl_easy_setopt(curl , CURLOPT_WRITEDATA , &readBuffer);

			CURLcode ret = curl_easy_perform(curl);

			long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	    		if (response_code != 200) {
        			QMessageBox errorMessage;
        			errorMessage.setWindowTitle("Error");
        			errorMessage.setText("Invalid OTP!");
        			errorMessage.setIcon(QMessageBox::Critical);
        			errorMessage.exec();
    			}else{
					auto j3 = Json::parse(readBuffer);

					if (j3.contains("msg") && j3["msg"].is_object()) {
            			auto msgObj = j3["msg"];

            		// Read 'uid' and 'key' from the 'msg' object
            		if (msgObj.contains("uid") && msgObj.contains("key")) {
                		
					std::string uid = msgObj["uid"];
                	std::string key = msgObj["key"];
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
            			configJson["uid_key"]["uid"] = uid;
            			configJson["uid_key"]["key"] =  key;
			 

           				 // Convert the updated JSON to a string
            			std::string content = configJson.dump();

			

           				 // Write the updated content back to the file
           				os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
        			}
        			bfree(profiledir);
					
					 isAuthenticated();
					emit authenticationSuccess();
					
            	}
        }

					}

	  		curl_easy_cleanup(curl);
			curl = NULL;

			

		});

	 auto SLabel = new QLabel(u8"<p>Need Accound ? <a href=\"https://app.streamway.in/signup/\">Signup Now Free</a></p>");
        SLabel->setTextFormat(Qt::RichText);
        SLabel->setWordWrap(true);
        SLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        SLabel->setOpenExternalLinks(true);
        LoginLayout->addWidget(SLabel);	

    return loginWithPhoneWidget;
};


QWidget* AuthManager::LoginWithEmailWidget(QTabWidget *tabWidget) {
	 QWidget* loginWithEmailWidget = new QWidget;
    QVBoxLayout* LoginLayout = new QVBoxLayout(loginWithEmailWidget);
	LoginLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 

	QWidget* scrollWidget = new QWidget;
		// scrollArea->setWidgetResizable(true);
		
		// Create a layout for the widget inside the scroll area
		QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
		scrollLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
		LoginLayout->addWidget(scrollWidget);
		scrollWidget->setVisible(false);

		// Add a label and text box for entering the uid
		QLabel* emailLabel_ = new QLabel("Enter Email");
		LoginLayout->addWidget(emailLabel_);

		QLineEdit* emailLineEdit_ = new QLineEdit();
		LoginLayout->addWidget(emailLineEdit_);
	
		// Add a button for getting OTP
		QPushButton* getOTPButton_ = new QPushButton("Get OTP");
		LoginLayout->addWidget(getOTPButton_);

		// OTP input and Verify button
		QLineEdit* otpLineEdit_ = new QLineEdit();
		otpLineEdit_->setValidator(new QIntValidator(0, 1000));
		otpLineEdit_->setPlaceholderText("Enter OTP here");
		LoginLayout->addWidget(otpLineEdit_);
		otpLineEdit_->setReadOnly(true);

		// Add a button for verification
		QPushButton* verifyButton_ = new QPushButton("Verify");
		verifyButton_->setVisible(false);
		LoginLayout->addWidget(verifyButton_);



		

		QObject::connect(getOTPButton_, &QPushButton::clicked, [scrollLayout , scrollWidget , emailLineEdit_ , getOTPButton_ ,verifyButton_, emailLabel_  , otpLineEdit_]() {

			QString email = emailLineEdit_->text();
			
			Json jsonObject;
			jsonObject["email"] = email.toStdString();
			// Convert JSON object to string
    		std::string jsonData = jsonObject.dump(2);

			CURL *curl;
    		
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_URL, "https://api.streamway.in/v1/otp/obs/email/send");
			// curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
			// curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Accept: */*");
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

			CURLcode ret = curl_easy_perform(curl);

			long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	    		if (response_code != 200) {
        			curl_easy_cleanup(curl);
					
        			QMessageBox errorMessage;
        			errorMessage.setWindowTitle("Error");
        			errorMessage.setText("This Email address is Not Registerd On Streamway!");
        			errorMessage.setIcon(QMessageBox::Critical);

					// Set informative text with a clickable link
					errorMessage.setInformativeText("<a href='https://app.streamway.in/signup'>SignUp Here</a>");

					// Make the text clickable
					errorMessage.setTextInteractionFlags(Qt::TextBrowserInteraction);

					// Show the message box and wait for user interaction
					int result = errorMessage.exec();

					if (result == QMessageBox::Ok) {
    					// User clicked OK, redirect to the specified URL
    					QUrl url("https://app.streamway.in/signup");
    					QDesktopServices::openUrl(url);
					}

    			}else{
					emailLineEdit_->setReadOnly(true);
					verifyButton_->setVisible(true);
					getOTPButton_->setVisible(false);
					otpLineEdit_->setVisible(true);
					otpLineEdit_->setReadOnly(false);
				}

	  		curl_easy_cleanup(curl);
			curl = NULL;
			
		
		});

		QObject::connect(verifyButton_, &QPushButton::clicked, [this , scrollLayout , scrollWidget , emailLineEdit_ , verifyButton_ , emailLabel_ ,otpLineEdit_ ]() {

			emailLabel_->setText("Enter OTP");
			
			QString email = emailLineEdit_->text();
			QString otp = otpLineEdit_->text();
			bool ok;
			int otpAsNumber = otp.toInt(&ok);
			Json jsonObject;
			jsonObject["email"] = email.toStdString();
			jsonObject["otp"] = otpAsNumber;

			// Convert JSON object to string
    		std::string jsonData = jsonObject.dump(2);

			CURL *curl;
    		
			std::string readBuffer;
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_URL, "https://api.streamway.in/v1/otp/obs/email/verify");
			// curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
			// curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Accept: */*");
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
			curl_easy_setopt(curl , CURLOPT_WRITEFUNCTION , writeCallback);
			curl_easy_setopt(curl , CURLOPT_WRITEDATA , &readBuffer);

			CURLcode ret = curl_easy_perform(curl);

			long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	    		if (response_code != 200) {
        			QMessageBox errorMessage;
        			errorMessage.setWindowTitle("Error");
        			errorMessage.setText("Invalid OTP!");
        			errorMessage.setIcon(QMessageBox::Critical);
        			errorMessage.exec();
    			}else{
					auto j3 = Json::parse(readBuffer);

					if (j3.contains("msg") && j3["msg"].is_object()) {
            			auto msgObj = j3["msg"];

            		// Read 'uid' and 'key' from the 'msg' object
            		if (msgObj.contains("uid") && msgObj.contains("key")) {
                		
					std::string uid = msgObj["uid"];
                	std::string key = msgObj["key"];
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
            			configJson["uid_key"]["uid"] = uid;
            			configJson["uid_key"]["key"] =  key;
			 

           				 // Convert the updated JSON to a string
            			std::string content = configJson.dump();

			

           				 // Write the updated content back to the file
           				os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
        			}
        			bfree(profiledir);
					
					 isAuthenticated();
					emit authenticationSuccess();
					
            	}
        }

					}

	  		curl_easy_cleanup(curl);
			curl = NULL;

			
		
		});



		 auto SLabel = new QLabel(u8"<p>Need Accound ? <a href=\"https://app.streamway.in/signup/\">Signup Now Free</a></p>");
        SLabel->setTextFormat(Qt::RichText);
        SLabel->setWordWrap(true);
        SLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        SLabel->setOpenExternalLinks(true);
        LoginLayout->addWidget(SLabel);

    return loginWithEmailWidget;
}


QWidget* AuthManager::LoginWithAPIKeyWidget(QTabWidget *tabWidget) {
	 QWidget* loginWithAPIKeyWidget = new QWidget;
    QVBoxLayout* LoginLayout = new QVBoxLayout(loginWithAPIKeyWidget);
	LoginLayout->setAlignment(Qt::AlignmentFlag::AlignTop); 
	
	QWidget* scrollWidget = new QWidget;
		// scrollArea->setWidgetResizable(true);
		
		// Create a layout for the widget inside the scroll area
		QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
		LoginLayout->addWidget(scrollWidget);
		scrollWidget->setVisible(false);

		// Add a label and text box for entering the uid
QHBoxLayout* uidLayout = new QHBoxLayout;
QLabel* uidLabel_ = new QLabel("Uid      ");
uidLayout->addWidget(uidLabel_);
QLineEdit* uidLineEdit_ = new QLineEdit();
uidLayout->addWidget(uidLineEdit_);
LoginLayout->addLayout(uidLayout);

// Add a label and text box for entering the API key
QHBoxLayout* keyLayout = new QHBoxLayout;
QLabel* codeLabel_ = new QLabel("API Key");
keyLayout->addWidget(codeLabel_);
QLineEdit* keyLineEdit_ = new QLineEdit();
keyLayout->addWidget(keyLineEdit_);
LoginLayout->addLayout(keyLayout);

	
		// Add a button for verification
		QPushButton* verifyButton_ = new QPushButton("Verify");
		LoginLayout->addWidget(verifyButton_);

		QLabel* errorL = new QLabel("Invalid Uid or Api Key");
		errorL->setStyleSheet("QLabel{font-size: 14px;font-family: Arial;}");
		scrollLayout->addWidget(errorL);
		errorL->setWordWrap(true);
		errorL->setStyleSheet("QLabel{font-size: 15px;font-family: Arial;color:red;}"); 
		errorL->setVisible(false);


		QObject::connect(verifyButton_, &QPushButton::clicked, [this , errorL,scrollLayout , scrollWidget , uidLineEdit_ , keyLineEdit_ ,verifyButton_ , codeLabel_  , uidLabel_ ,tabWidget]() {
			// Get the code entered by the user
			QString uid = uidLineEdit_->text();
			QString key = keyLineEdit_->text();

			QString combined = uid + ":" + key;
		

			QByteArray combinedData = combined.toUtf8().toBase64();
			QString base64AuthHeader = "Basic " + QString(combinedData);


 

   // Use a try-catch block to catch any potential exceptions
    try {
       // Send the HTTP request
  if(sendHttpRequest(uid , key)){
	try{

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
            configJson["uid_key"]["uid"] = uid.toStdString();
            configJson["uid_key"]["key"] =  key.toStdString();
			 

            // Convert the updated JSON to a string
            std::string content = configJson.dump();

			

            // Write the updated content back to the file
            os_quick_write_utf8_file_safe(filename.c_str(), content.c_str(), content.size(), true, "tmp", "bak");
        }
        bfree(profiledir);

		isAuthenticated();
		emit authenticationSuccess();
		
	
	}catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
		scrollWidget->setVisible(true);
       	errorL->setVisible(true);
		
	}
						
  }else{
	scrollWidget->setVisible(true);
	errorL->setVisible(true);
  }
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
		scrollWidget->setVisible(true);
        errorL->setVisible(true);
    }			
		});

 auto SLabel = new QLabel(u8"<p>Need Accound ? <a href=\"https://app.streamway.in/signup/\">Signup Now Free</a></p>");
        SLabel->setTextFormat(Qt::RichText);
        SLabel->setWordWrap(true);
        SLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        SLabel->setOpenExternalLinks(true);
        LoginLayout->addWidget(SLabel);

	auto label = new QLabel(
				u8"<p>Get Your <a href=\"https://app.streamway.in/account/obs\">Uid and Api Key</a></p>");
			label->setTextFormat(Qt::RichText);
			label->setTextInteractionFlags(
				Qt::TextBrowserInteraction);
			label->setOpenExternalLinks(true);
			LoginLayout->addWidget(label);

    return loginWithAPIKeyWidget;
}


// Function to handle Authentication Tabs
QWidget* AuthManager::handleAuthTab() {

    // Create a container widget to hold the login label and tab widget
    QWidget* containerWidget = new QWidget;

    // Create a layout for the container widget
    QVBoxLayout* containerLayout = new QVBoxLayout(containerWidget);
	containerLayout->setContentsMargins(0, 0, 0, 0); // Set margins to 0 to reduce extra space


    // Show authentication screen if not authenticated
    QLabel* loginLabel_ = new QLabel("LOGIN WITH", containerWidget);
    loginLabel_->setAlignment(Qt::AlignCenter); // Align the text to the center
    containerLayout->addWidget(loginLabel_, 0, Qt::AlignCenter); // Add the label with center alignment

    // Create a QTabWidget to hold the tabs
    QTabWidget* tabWidget = new QTabWidget(containerWidget);

    // Create widgets for each tab content
    QWidget* phoneWidget = LoginWithPhoneWidget(tabWidget);
    QWidget* emailWidget = LoginWithEmailWidget(tabWidget);
    QWidget* apiKeyWidget = LoginWithAPIKeyWidget(tabWidget);

    // Add tabs to the QTabWidget
    tabWidget->addTab(phoneWidget, "Phone");
    tabWidget->addTab(emailWidget, "Email");
    tabWidget->addTab(apiKeyWidget, "API Key");

    // Add the tab widget to the container layout
    containerLayout->addWidget(tabWidget);

    return containerWidget;
}

