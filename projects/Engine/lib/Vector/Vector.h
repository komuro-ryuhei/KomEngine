#pragma once

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 final {
	float x;
	float y;

	// + 演算子のオーバーロード
	Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }

	// - 演算子のオーバーロード
	Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }

	// + オペレーターのオーバーロード
	Vector2& operator+(const float& other) {
		x = x + other;
		y = y + other;
		return *this;
	}

	// + オペレーターのオーバーロード
	Vector2& operator+=(const float& other) {
		x += other;
		y += other;
		return *this;
	}

	// += オペレーターのオーバーロード
	Vector2& operator+=(const Vector2& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	// *= オペレーターのオーバーロード (スカラ倍)
	Vector2& operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		return *this;
	}

	// Vector2 と float の掛け算のオペレーター
	Vector2 operator*(float scalar) const { return Vector2{x * scalar, y * scalar}; }
};

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;

	// + 演算子のオーバーロード
	Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }

	// - 演算子のオーバーロード
	Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }

	// + オペレーターのオーバーロード
	Vector3& operator+(const float& other) {
		x = x + other;
		y = y + other;
		z = z + other;
		return *this;
	}

	// + オペレーターのオーバーロード
	Vector3& operator+=(const float& other) {
		x = x += other;
		y = y += other;
		z = z += other;
		return *this;
	}

	// += オペレーターのオーバーロード
	Vector3& operator+=(const Vector3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	// *= オペレーターのオーバーロード (スカラ倍)
	Vector3& operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	// Vector3 と float の掛け算のオペレーター
	Vector3 operator*(float scalar) { return Vector3{x * scalar, y * scalar, z * scalar}; }
};

/// <summary>
/// 4次元ベクトル
/// </summary>
struct Vector4 final {
	float x;
	float y;
	float z;
	float w;
};