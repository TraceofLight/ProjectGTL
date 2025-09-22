#include "pch.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

IMPLEMENT_CLASS(UObjectPicker, UObject)

FRay UObjectPicker::GetModelRay(const FRay& InRay, const UPrimitiveComponent* InPrimitive)
{
	FMatrix ModelInverse = InPrimitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = InRay.Origin * ModelInverse;
	ModelRay.Direction = InRay.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(const FRay& InWorldRay,
                                                  const TArray<UPrimitiveComponent*>& InCandidate, float* InDistance)
{
	UPrimitiveComponent* ShortestPrimitive = nullptr;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;

	for (UPrimitiveComponent* Primitive : InCandidate)
	{
		if (Primitive->GetPrimitiveType() == EPrimitiveType::BillBoard)
		{
			continue;
		}
		FMatrix ModelMat = Primitive->GetWorldTransformMatrix();
		FRay ModelRay = GetModelRay(InWorldRay, Primitive);
		if (IsRayPrimitiveCollided(ModelRay, Primitive, ModelMat, &PrimitiveDistance))
		// Ray와 Primitive가 충돌했다면 거리 테스트 후 가까운 Actor Picking
		{
			if (PrimitiveDistance < ShortestDistance)
			{
				ShortestPrimitive = Primitive;
				ShortestDistance = PrimitiveDistance;
			}
		}
	}
	*InDistance = ShortestDistance;

	return ShortestPrimitive;
}

void UObjectPicker::PickGizmo(const FRay& InWorldRay, UGizmo& InGizmo, FVector& InCollisionPoint, float InViewportWidth,
                              float InViewportHeight, int32 InViewportIndex)
{
	if (!Camera || !InGizmo.GetSelectedActor())
	{
		InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::None);
		return;
	}

	// 뷰포트 정보가 있으면 올바른 스케일 계산을 위해 기즈모 스케일 업데이트
	if (InViewportWidth > 0.0f && InViewportHeight > 0.0f)
	{
		// 현재 뷰포트에 맞는 스케일 계산 후 기즈모에 적용
		float CorrectScale = InGizmo.CalculateScreenSpaceScale(Camera->GetLocation(), Camera, InViewportWidth,
		                                                       InViewportHeight);
		// 임시로 기즈모의 CurrentRenderScale을 업데이트 (피킹용)
		InGizmo.SetCurrentRenderScaleForPicking(CorrectScale);
	}

	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	FVector GizmoLocation = InGizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = {FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1}};

	if (InGizmo.GetGizmoMode() == EGizmoMode::Scale || !InGizmo.IsWorldMode())
	{
		FVector Rad = FVector::GetDegreeToRadian(InGizmo.GetActorRotation());
		FMatrix R = FMatrix::RotationMatrix(Rad);
		//FQuaternion q = FQuaternion::FromEuler(Rad);

		for (int i = 0; i < 3; i++)
		{
			//GizmoAxises[a] = FQuaternion::RotateVector(q, GizmoAxises[a]); // 쿼터니언으로 축 회전
			//GizmoAxises[a].Normalize();
			const FVector4 a4(GizmoAxises[i].X, GizmoAxises[i].Y, GizmoAxises[i].Z, 0.0f);
			FVector4 rotated4 = a4 * R;
			FVector V(rotated4.X, rotated4.Y, rotated4.Z);
			V.Normalize();
			GizmoAxises[i] = V;
		}
	}

	FVector WorldRayOrigin{InWorldRay.Origin.X, InWorldRay.Origin.Y, InWorldRay.Origin.Z};
	FVector WorldRayDirection(InWorldRay.Direction.X, InWorldRay.Direction.Y, InWorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (InGizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
		{
			FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
			bool bIsCollide = false;

			float GizmoRadius = InGizmo.GetTranslateRadius();
			float GizmoHeight = InGizmo.GetTranslateHeight();
			float A, B, C; //Ax^2 + Bx + C의 ABC
			float X; //해
			float Det; //판별식
			//0 = forward 1 = Right 2 = UP

			for (int a = 0; a < 3; a++)
			{
				FVector GizmoAxis = GizmoAxises[a];
				A = 1 - static_cast<float>(pow(InWorldRay.Direction.Dot3(GizmoAxis), 2));
				B = InWorldRay.Direction.Dot3(GizmoDistanceVector) - InWorldRay.Direction.Dot3(GizmoAxis) *
					GizmoDistanceVector.
					Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
				C = static_cast<float>(GizmoDistanceVector.Dot(GizmoDistanceVector) -
					pow(GizmoDistanceVector.Dot(GizmoAxis), 2)) - GizmoRadius * GizmoRadius;

				Det = B * B - A * C;
				if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
				{
					X = (-B + sqrtf(Det)) / A;
					FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
					float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
					if (Height <= GizmoHeight && Height >= 0) //충돌
					{
						InCollisionPoint = PointOnCylinder;
						bIsCollide = true;
					}
					X = (-B - sqrtf(Det)) / A;
					PointOnCylinder = InWorldRay.Origin + InWorldRay.Direction * X;
					Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
					if (Height <= GizmoHeight && Height >= 0)
					{
						InCollisionPoint = PointOnCylinder;
						bIsCollide = true;
					}
					if (bIsCollide)
					{
						switch (a)
						{
						case 0: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Forward);
							return;
						case 1: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Right);
							return;
						case 2: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Up);
							return;
						}
					}
				}
			}
		}
		break;
	case EGizmoMode::Rotate:
		{
			for (int a = 0; a < 3; a++)
			{
				if (IsRayCollideWithPlane(InWorldRay, GizmoLocation, GizmoAxises[a], InCollisionPoint))
				{
					FVector RadiusVector = InCollisionPoint - GizmoLocation;
					if (InGizmo.IsInRadius(RadiusVector.Length()))
					{
						switch (a)
						{
						case 0: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Forward);
							return;
						case 1: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Right);
							return;
						case 2: InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::Up);
							return;
						}
					}
				}
			}
		}
		break;
	default: break;
	}

	InGizmo.SetGizmoDirectionForViewport(InViewportIndex, EGizmoDirection::None);
}

//개별 primitive와 ray 충돌 검사
bool UObjectPicker::IsRayPrimitiveCollided(const FRay& InModelRay, UPrimitiveComponent* InPrimitive,
                                           const FMatrix& InModelMatrix, float* InShortestDistance)

{
	//FRay ModelRay = GetModelRay(Ray, Primitive);

	const TArray<FVertex>* Vertices = InPrimitive->GetVerticesData();

	float Distance = D3D11_FLOAT32_MAX; //Distance 초기화
	bool bIsHit = false;
	for (int32 a = 0; a + 2 < Vertices->size(); a = a + 3) //삼각형 단위로 Vertex 위치정보 읽음
	{
		const FVector& Vertex1 = (*Vertices)[a].Position;
		const FVector& Vertex2 = (*Vertices)[a + 1].Position;
		const FVector& Vertex3 = (*Vertices)[a + 2].Position;

		if (IsRayTriangleCollided(InModelRay, Vertex1, Vertex2, Vertex3, InModelMatrix, &Distance))
		//Ray와 삼각형이 충돌하면 거리 비교 후 최단거리 갱신

		{
			bIsHit = true;
			if (Distance < *InShortestDistance)
			{
				*InShortestDistance = Distance;
			}
		}
	}

	return bIsHit;
}

bool UObjectPicker::IsRayTriangleCollided(const FRay& InRay, const FVector& InVertex1, const FVector& InVertex2,
                                          const FVector& InVertex3,
                                          const FMatrix& InModelMatrix, float* InDistance)
{
	if (!Camera) { return false; }
	FVector CameraForward = Camera->GetForward(); //카메라 정보 필요
	float NearZ = Camera->GetNearZ();
	float FarZ = Camera->GetFarZ();
	FMatrix ModelTransform; //Primitive로부터 얻어내야함.(카메라가 처리하는게 나을듯)


	//삼각형 내의 점은 E1*V + E2*U + Vertex1.Position으로 표현 가능( 0<= U + V <=1,  Y>=0, V>=0 )
	//Ray.Direction * T + Ray.Origin = E1*V + E2*U + Vertex1.Position을 만족하는 T U V값을 구해야 함.
	//[E1 E2 RayDirection][V U T] = [RayOrigin-Vertex1.Position]에서 cramer's rule을 이용해서 T U V값을 구하고
	//U V값이 저 위의 조건을 만족하고 T값이 카메라의 near값 이상이어야 함.
	FVector RayDirection{InRay.Direction.X, InRay.Direction.Y, InRay.Direction.Z};
	FVector RayOrigin{InRay.Origin.X, InRay.Origin.Y, InRay.Origin.Z};
	FVector E1 = InVertex2 - InVertex1;
	FVector E2 = InVertex3 - InVertex1;
	FVector Result = (RayOrigin - InVertex1); //[E1 E2 -RayDirection]x = [RayOrigin - Vertex1.Position] 의 result임.


	FVector CrossE2Ray = E2.Cross(RayDirection);
	FVector CrossE1Result = E1.Cross(Result);

	float Determinant = E1.Dot(CrossE2Ray);

	float NoInverse = 0.0001f; //0.0001이하면 determinant가 0이라고 판단=>역행렬 존재 X
	if (abs(Determinant) <= NoInverse)
	{
		return false;
	}


	float V = Result.Dot(CrossE2Ray) / Determinant; //cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.

	if (V < 0 || V > 1)
	{
		return false;
	}

	float U = RayDirection.Dot(CrossE1Result) / Determinant;
	if (U < 0 || U + V > 1)
	{
		return false;
	}


	float T = E2.Dot(CrossE1Result) / Determinant;

	FVector HitPoint = RayOrigin + RayDirection * T; //모델 좌표계에서의 충돌점
	FVector4 HitPoint4{HitPoint.X, HitPoint.Y, HitPoint.Z, 1};
	//이제 이것을 월드 좌표계로 변환해서 view Frustum안에 들어가는지 판단할 것임.(near, far plane만 테스트하면 됨)

	FVector4 HitPointWorld = HitPoint4 * InModelMatrix;
	FVector4 RayOriginWorld = InRay.Origin * InModelMatrix;

	FVector4 DistanceVec = HitPointWorld - RayOriginWorld;
	if (DistanceVec.Dot3(CameraForward) >= NearZ && DistanceVec.Dot3(CameraForward) <= FarZ)
	{
		*InDistance = DistanceVec.Length();
		return true;
	}
	return false;
}


bool UObjectPicker::IsRayCollideWithPlane(const FRay& InWorldRay, FVector InPlanePoint, FVector InNormal,
                                          FVector& InPointOnPlane)
{
	FVector WorldRayOrigin{InWorldRay.Origin.X, InWorldRay.Origin.Y, InWorldRay.Origin.Z};

	if (abs(InWorldRay.Direction.Dot3(InNormal)) < 0.01f)
		return false;

	float Distance = (InPlanePoint - WorldRayOrigin).Dot(InNormal) / InWorldRay.Direction.Dot3(InNormal);

	if (Distance < 0)
		return false;
	InPointOnPlane = InWorldRay.Origin + InWorldRay.Direction * Distance;


	return true;
}
