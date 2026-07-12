#pragma once

class BossEnemy;
class Camera;
class ResultImage;

class ClearSequenceController {

public:

	void Start(Camera* camera);

	void Update(
		float dt,
		BossEnemy* boss,
		Camera* camera,
		ResultImage* result
	);

	bool IsStarted() const { return started_; }
	bool IsResultStarted() const { return resultStarted_; }

	// result開始をGameScene側に通知する用
	bool ConsumeResultStartedRequest();

private:

	void TriggerExplosionStep(
		int step,
		BossEnemy* boss,
		Camera* camera
	);

private:

	bool started_ = false;
	bool resultStarted_ = false;
	bool resultStartedRequest_ = false;

	float sequenceTimer_ = 0.0f;

	int explosionStep_ = 0;
	float explosionTimer_ = 0.0f;
	float explosionInterval_ = 0.18f;

	float resultStartTime_ = 2.8f;
};