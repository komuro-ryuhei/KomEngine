#pragma once

#include "Network.h"
#include "externals/imgui/imgui.h"
#include <vector>

class RankingManager {
public:
	RankingManager(const std::wstring& serverUrl);

	void Login(const std::wstring& username, const std::wstring& password);
	void SendScore(int newScore);
	void UpdateRanking();
	void Render();

private:
	Network network;
	std::vector<int> ranking;
	int score;
	bool isRequestInProgress;
	bool isLoggedIn;
};