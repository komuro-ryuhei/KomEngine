#pragma once
#include "Engine/Base/3d/Object3d/Object3d.h"
#include "Engine/Base/2d/Sprite/Sprite.h"

class Player;
class Camera;

class BossEnemy {

private:

	enum class AttackPhase {
		SingleLeft,   // 左腕のみ
		SingleRight,  // 右腕のみ
		BothHands,    // 両手同時
		WaitMeteor,   // 流星攻撃中（腕は攻撃しない）
	};

	AttackPhase attackPhase_ = AttackPhase::SingleLeft;
	bool meteorRequest_ = false; // 両手攻撃完了後にtrue

public:

	void Init(Camera* camera);

	void Update();

	void Draw();

	void ImGuiDebug();

public:

	// getter・setter
	float GetRadius() const;
	Transform GetTransform() const;
	bool GetIsmoveRight() const { return isMoveRight_; }

	void SetRotate(Vector3& rotate);
	void SetTranslate(Vector3 translate);

	void SetPlayer(Player* player) { player_ = player; }

	void SetAttack(bool isAttack) { isAttack_ = isAttack; }
	void SetAttackSpeed(float speed) { attackSpeed_ = speed; }
	float GetAttackSpeed() const { return attackSpeed_; }

	void SetInTitleScene(bool isTitleScene) { isInTitleScene_ = isTitleScene; }

	// ----- 部位 ----- //
	Object3d* GetBody() const { return object3d_.get(); }
	Object3d* GetLeftArm() const { return leftArm_.get(); }
	Object3d* GetRightArm() const { return rightArm_.get(); }

	// ----- 右手 ----- //
	void    SetRightHandScale(const Vector3& s);
	Vector3 GetRightHandWorldPos() const;
	float   GetRightHandRadius() const;

	// ----- 左手 ----- //
	void    SetLeftHandScale(const Vector3& s);
	Vector3 GetLeftHandWorldPos() const;
	float   GetLeftHandRadius() const;

	// HP
	void   Damage(int v);
	int    GetHP() const { return hp_; }
	bool   IsDead() const { return hp_ <= 0; }

	// 着地(墜落)したかのフラグ
	bool   HasLanded() const { return hasLanded_; }

	// ----- 攻撃 ----- //
	// 攻撃中かどうか（腕が伸びているフェーズか）を外からチェック用
	bool IsExtending() const { return isExtending_; }

	// 現在攻撃に使っている腕が左かどうか
	bool IsLeftArmAttacking() const { return attackPhase_ == AttackPhase::SingleLeft; }

	// 今攻撃に使っている腕のワールド座標
	Vector3 GetCurrentArmWorldPos() const;

	// 腕のヒット数
	int GetLeftHitCount() const { return leftArmHitCount_; }
	int GetRightHitCount() const { return rightArmHitCount_; }

public:

	void AddHitLeftArm() { ++leftArmHitCount_; }
	void AddHitRightArm() { ++rightArmHitCount_; }

	// AttackPhase 取得系（UpdateArmTargetMarker用）
	bool IsRightArmAttacking() const { return attackPhase_ == AttackPhase::SingleRight; }
	bool IsBothHandsAttacking() const { return attackPhase_ == AttackPhase::BothHands; }

	// 最大ヒット数（ターゲット消す条件に使う）
	int GetMaxHitCount() const { return maxHitCount_; }

private:

	void Attack();
	void Move();
	void TitleSceneMove();

public:
	void AddHitToAttackingArm();
	void InitTitleScenePos();

	// 流星攻撃との連携
	bool ConsumeMeteorRequest(); // trueを返したタイミングでフラグを消費
	void OnMeteorFinished();     // メテオ終了後に呼ぶ

	void HPDraw();


private:
	// カメラ
	Camera* camera_ = nullptr;
	// SRT
	Transform transform_;
	// 半径
	float radius_ = 4.0f;

	Player* player_ = nullptr;

	// モデル
	std::unique_ptr<Object3d> object3d_ = nullptr;
	// 部位
	std::unique_ptr<Object3d> leftArm_;
	std::unique_ptr<Object3d> rightArm_;

	// 
	std::unique_ptr<Sprite> hpSprite_;
 
	// 攻撃用のタイマーと状態
	float attackTimer_ = 0.0f;
	float attackInterval_ = 2.0f; // 2秒周期
	bool isExtending_ = true;     // 腕を伸ばしているか
	float attackCooldown_ = 0.0f;
	bool isAttacking_ = false;
	bool attackLeftArm_ = true;
	float attackSpeed_ = 0.1f; // 腕の伸縮速度
	int leftArmHitCount_ = 0;
	int rightArmHitCount_ = 0;
	const int maxHitCount_ = 5;

	// 攻撃用フラグ
	bool isAttack_ = true;

	// TitleScene用のフラグ
	bool isInTitleScene_ = false;

	// 
	Vector3 rightArmPos_;
	Vector3 leftArmPos_;
	Vector3 rightArmRot_;
	Vector3 leftArmRot_;

	bool isMoveRight_ = false;
	bool pushEnter_ = true;

	// HP
	int maxHp_ = 10;
	int hp_ = maxHp_;

	// 両手攻撃用：左右個別に伸縮管理
	bool leftExtending_ = true;
	bool rightExtending_ = true;

	// 撃破後の墜落制御
	bool  fallStarted_ = false;   // 落下開始したか
	bool  hasLanded_ = false;   // 地面に着いたか
	float fallVelY_ = 0.0f;    // 落下速度
	float gravityY_ = -0.006f;  // 重力加速度（毎フレーム加算）
	float groundY_ = -5.0f;   // 地面のY（glassObject_ と合わせた）
	// 回転しながら落下用
	float fallRotateStart_ = 0.0f;    // 開始角度（今の回転を保存）
	float fallRotateEnd_ = -1.2f;   // 最終角度（ラジアン）＝約 -70 度前に倒す

	// ★ 落下中シェイク用
	float fallShakeTime_ = 0.0f;
	float fallShakeAmplitude_ = 0.25f; // 揺れ幅（XZ方向）

	// 落下後のカメラシェイクフラグ
	bool landingShakeDone_ = false;
};