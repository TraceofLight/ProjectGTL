#pragma once

struct FMatrix;

struct FVector
{
	float X;
	float Y;
	float Z;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector();

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector(float InX, float InY, float InZ);

	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector(const FVector& InOther);

	void operator=(const FVector4& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector operator+(const FVector& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector operator-(const FVector& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
	 */
	FVector operator*(float InRatio) const;

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	FVector& operator+=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
	 */
	FVector& operator-=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
	 */
	FVector& operator*=(float InRatio);

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	FVector operator-() const { return {-X, -Y, -Z}; }

	bool operator==(const FVector& InOther) const;

	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	float Length() const { return sqrtf(X * X + Y * Y + Z * Z); }

	/**
	 * @brief 자신의 벡터의 각 성분을 제곱하여 더한 값을 반환하는 함수 (루트 사용 X)
	 */
	float LengthSquared() const { return (X * X) + (Y * Y) + (Z * Z); }

	/**
	 * @brief 두 벡터를 내적하여 결과의 스칼라 값을 반환하는 함수
	 */
	float Dot(const FVector& InOtherVector) const
	{
		return (X * InOtherVector.X) + (Y * InOtherVector.Y) + (Z * InOtherVector.Z);
	}

	/**
	 * @brief 두 벡터를 외적한 결과의 벡터 값을 반환하는 함수
	 */
	inline FVector Cross(const FVector& InOtherVector) const
	{
		return FVector(
			Z * InOtherVector.Y - Y * InOtherVector.Z,
			X * InOtherVector.Z - Z * InOtherVector.X,
			Y * InOtherVector.X - X * InOtherVector.Y
		);
	}

	/**
	 * @brief 단위 벡터로 변경하는 함수
	 */
	void Normalize()
	{
		float Length = sqrt(LengthSquared());
		if (Length > 0.00000001f)
		{
			X /= Length;
			Y /= Length;
			Z /= Length;
		}
	}

	/**
	 * @brief 각도를 라디안으로 변환한 값을 반환하는 함수
	 */
	static float GetDegreeToRadian(const float InDegree) { return (InDegree * PI) / 180.f; }

	static FVector GetDegreeToRadian(const FVector& InRotation)
	{
		return FVector{(InRotation.X * PI) / 180.f, (InRotation.Y * PI) / 180.f, (InRotation.Z * PI) / 180.f};
	}

	/**
	 * @brief 벡터의 절대값을 반환하는 함수
	 */
	static FVector GetAbs(const FVector& InVector)
	{
		return {abs(InVector.X), abs(InVector.Y), abs(InVector.Z)};
	}

	/**
	 * @brief 라디안를 각도로 변환한 값을 반환하는 함수
	 */
	static float GetRadianToDegree(const float Radian) { return (Radian * 180.f) / PI; }

	static FVector GetRadianToDegree(const FVector& Rad)
	{
		return FVector{Rad.X * (180.0f / PI), Rad.Y * (180.0f / PI), Rad.Z * (180.0f / PI)};
	}

	// Constant Vector (definition from UE5)
	static FVector ZeroVector() { return {0.0f, 0.0f, 0.0f}; }
	static FVector OneVector() { return {1.0f, 1.0f, 1.0f}; }
	static FVector ForwardVector() { return {1.0f, 0.0f, 0.0f}; }
	static FVector BackwardVector() { return {-1.0f, 0.0f, 0.0f}; }
	static FVector UpVector() { return {0.0f, 0.0f, 1.0f}; }
	static FVector DownVector() { return {0.0f, 0.0f, -1.0f}; }
	static FVector RightVector() { return {0.0f, 1.0f, 0.0f}; }
	static FVector LeftVector() { return {0.0f, -1.0f, 0.0f}; }
	static FVector XAxisVector() { return {1.0f, 0.0f, 0.0f}; }
	static FVector YAxisVector() { return {0.0f, 1.0f, 0.0f}; }
	static FVector ZAxisVector() { return {0.0f, 0.0f, 1.0f}; }

	[[nodiscard]] static FVector Zero() { return ZeroVector(); }
	[[nodiscard]] static FVector One() { return OneVector(); }
	[[nodiscard]] static FVector UnitX() { return XAxisVector(); }
	[[nodiscard]] static FVector UnitY() { return YAxisVector(); }
	[[nodiscard]] static FVector UnitZ() { return ZAxisVector(); }
};

struct FVector2
{
	float X;
	float Y;

	/**
	 * @brief FVector2 기본 생성자
	 */
	FVector2();

	/**
	 * @brief FVector2의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector2(float InX, float InY);

	/**
	 * @brief FVector2를 Param으로 넘기는 생성자
	 */
	FVector2(const FVector2& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator+(const FVector2& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator-(const FVector2& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백터를 반환하는 함수
	 */
	FVector2 operator*(const float Ratio) const;

	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	inline float Length() const { return sqrtf(X * X + Y * Y); }

	/**
	 * @brief 자신의 벡터의 각 성분을 제곱하여 더한 값을 반환하는 함수 (루트 사용 X)
	 */
	inline float LengthSquared() const { return (X * X) + (Y * Y); }
};


struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector4();

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector4(float InX, float InY, float InZ, float InW);

	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector4& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator+(const FVector4& InOtherVector) const;

	/**
	 * @brief 벡터와 행렬곱
	 */
	FVector4 operator*(const FMatrix& InMatrix) const;
	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator-(const FVector4& InOtherVector) const;

	/**
	 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
	 */
	FVector4 operator*(float InRatio) const;


	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	void operator+=(const FVector4& InOtherVector);

	/**
	 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
	 */
	void operator-=(const FVector4& InOtherVector);

	/**
	 * @brief 자신의 벡터에 배율을 곱하는 함수
	 */
	void operator*=(float Ratio);

	float Length() const
	{
		return sqrtf(X * X + Y * Y + Z * Z + W * W);
	}

	void Normalize()
	{
		float Magnitude = this->Length();
		X /= Magnitude;
		Y /= Magnitude;
		Z /= Magnitude;
		W /= Magnitude;
	}


	/**
	 * @brief W 성분 무시하고 dot product 진행하는 함수
	 */
	float Dot3(const FVector4& InOtherVector) const
	{
		return X * InOtherVector.X + Y * InOtherVector.Y + Z * InOtherVector.Z;
	}

	float Dot3(const FVector& InOtherVector) const
	{
		return X * InOtherVector.X + Y * InOtherVector.Y + Z * InOtherVector.Z;
	}
};
