#include "RankingManager.h"

RankingManager::RankingManager(const std::wstring& serverUrl) : network(serverUrl), score(0), isRequestInProgress(false), isLoggedIn(false) {}

void RankingManager::Login(const std::wstring& username, const std::wstring& password) {
	network.Login(username, password).wait();
	isLoggedIn = true;
}

void RankingManager::SendScore(int newScore) {
	if (!isLoggedIn)
		return;

	score = newScore;
	isRequestInProgress = true;

	network.PostScore(score)
		.then([this](int statusCode) {
		if (statusCode == 1) {
			network.GetRanking()
				.then([this](std::vector<int> scores) {
				ranking = scores;
				isRequestInProgress = false;
					})
				.wait();
		} else {
			isRequestInProgress = false;
		}
			})
		.wait();
}

void RankingManager::UpdateRanking() {
	if (!isLoggedIn || isRequestInProgress)
		return;

	isRequestInProgress = true;

	network.GetRanking()
		.then([this](std::vector<int> scores) {
		ranking = scores;
		isRequestInProgress = false;
			})
		.wait();
}

void RankingManager::Render() {
	ImGui::Begin("Ranking Board");

	ImGui::Text("Top 5 Scores:");
	for (int i = 0; i < ranking.size(); i++) {
		ImGui::Text("%d: %d", i + 1, ranking[i]);
	}

	if (ImGui::Button("Update Ranking")) {
		UpdateRanking();
	}

	ImGui::End();
}