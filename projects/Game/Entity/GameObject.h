#pragma once

class GameObject {

public:

	virtual ~GameObject() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	bool IsActive() const { return isActive_; }
	virtual void Kill() { isActive_ = false; }

protected:

	bool isActive_ = true;
};