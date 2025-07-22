#include "Network.h"
// #include <stdexcept>

//Network::Network(const std::wstring& serverUrl) : serverUrl(serverUrl) {}x
//
//pplx::task<void> Network::Login(const std::wstring& username, const std::wstring& password) {
//	return pplx::create_task([=] {
//		       json::value postData;
//		       postData[L"name"] = json::value::string(username);
//		       postData[L"password"] = json::value::string(password);
//
//		       http_client client(serverUrl + L"/users/login");
//		       return client.request(methods::POST, L"", postData.serialize(), L"application/json");
//	       })
//	    .then([this](http_response response) {
//		    if (response.status_code() == status_codes::OK) {
//			    return response.extract_json();
//		    } else {
//			    throw std::runtime_error("Login failed");
//		    }
//	    })
//	    .then([this](json::value json) {
//		    if (json.has_field(L"token")) {
//			    authToken = json[L"token"].as_string();
//		    } else {
//			    throw std::runtime_error("Invalid login response");
//		    }
//	    });
//}
//
//pplx::task<int> Network::PostScore(int score) {
//	return pplx::create_task([=] {
//		       json::value postData;
//		       postData[L"score"] = score;
//
//		       http_client client(serverUrl + L"/scores");
//		       http_request request(methods::POST);
//		       request.headers().add(L"Authorization", L"Bearer " + authToken);
//		       request.headers().set_content_type(L"application/json");
//		       request.set_body(postData.serialize());
//
//		       return client.request(request);
//	       })
//	    .then([](http_response response) {
//		    if (response.status_code() == status_codes::OK) {
//			    return response.extract_json();
//		    } else {
//			    throw std::runtime_error("Post failed");
//		    }
//	    })
//	    .then([](json::value json) {
//		    if (json.has_field(L"status_code")) {
//			    return json[L"status_code"].as_integer();
//		    } else {
//			    throw std::runtime_error("Invalid JSON response");
//		    }
//	    });
//}
//
//pplx::task<std::vector<int>> Network::GetRanking() {
//	return pplx::create_task([=] {
//		       http_client client(serverUrl + L"/scores");
//		       http_request request(methods::GET);
//		       request.headers().add(L"Authorization", L"Bearer " + authToken);
//
//		       return client.request(request);
//	       })
//	    .then([](http_response response) {
//		    if (response.status_code() == status_codes::OK) {
//			    return response.extract_json();
//		    } else {
//			    throw std::runtime_error("Get failed");
//		    }
//	    })
//	    .then([](json::value json) {
//		    std::vector<int> ranking;
//		    auto array = json.as_array();
//		    for (const auto& item : array) {
//			    ranking.push_back(item.at(U("score")).as_integer());
//		    }
//		    return ranking;
//	    });
//}