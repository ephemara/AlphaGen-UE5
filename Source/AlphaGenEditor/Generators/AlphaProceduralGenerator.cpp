// Copyright 2026 K-Studio. All Rights Reserved.

#include "Generators/AlphaProceduralGenerator.h"
#include "Compute/AlphaGenComputeShaders.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

// Helper to safely get a param with a default value
static float GetParam(const TMap<FString, float>& Params, const FString& Key, float Default)
{
	const float* Value = Params.Find(Key);
	return Value ? *Value : Default;
}

UTexture2D* FAlphaProceduralGenerator::Generate(
	EProceduralType Type,
	int32 Size,
	const TMap<FString, float>& Params)
{
	// Clamp size like Rust: params.size.max(16).min(2048)
	// We allow up to 4096 for higher quality
	Size = FMath::Clamp(Size, 16, 4096);
	
	// Map widget enum to compute shader enum
	EAlphaGenType GPUType;
	switch (Type)
	{
		case EProceduralType::Radial:  GPUType = EAlphaGenType::Radial; break;
		case EProceduralType::Circle:  GPUType = EAlphaGenType::Circle; break;
		case EProceduralType::Square:  GPUType = EAlphaGenType::Square; break;
		case EProceduralType::Diamond: GPUType = EAlphaGenType::Diamond; break;
		case EProceduralType::Perlin:  GPUType = EAlphaGenType::Perlin; break;
		case EProceduralType::Voronoi: GPUType = EAlphaGenType::Voronoi; break;
		case EProceduralType::Bricks:  GPUType = EAlphaGenType::Bricks; break;
		case EProceduralType::Dots:    GPUType = EAlphaGenType::Dots; break;
		case EProceduralType::SeamlessNoise: GPUType = EAlphaGenType::SeamlessNoise; break;
		case EProceduralType::Crosshatch: GPUType = EAlphaGenType::Crosshatch; break;
		case EProceduralType::Waves: GPUType = EAlphaGenType::Waves; break;
		case EProceduralType::Checkerboard: GPUType = EAlphaGenType::Checkerboard; break;
		case EProceduralType::Hexagon: GPUType = EAlphaGenType::Hexagon; break;
		case EProceduralType::Tears: GPUType = EAlphaGenType::Tears; break;
		case EProceduralType::Scratches: GPUType = EAlphaGenType::Scratches; break;
		case EProceduralType::Splatter: GPUType = EAlphaGenType::Splatter; break;
		case EProceduralType::Cracks: GPUType = EAlphaGenType::Cracks; break;
		case EProceduralType::Cells: GPUType = EAlphaGenType::Cells; break;
		case EProceduralType::Grunge: GPUType = EAlphaGenType::Grunge; break;
		case EProceduralType::Fibers: GPUType = EAlphaGenType::Fibers; break;
		case EProceduralType::Caustics: GPUType = EAlphaGenType::Caustics; break;
		default: GPUType = EAlphaGenType::Radial;
	}
	
	// GPU expects these params in ProceduralParams (x,y,z,w) and ProceduralParams2 (x,y,z,w)
	// Each generator uses params differently, so we map explicitly per-type:
	// 
	// ProceduralParams:
	//   .x = Primary param (falloff for shapes, scale for noise)
	//   .y = Secondary param (softness for shapes, octaves/edge_width for noise)
	//   .z = Seed
	//   .w = Unused
	//
	// ProceduralParams2:
	//   .x = Pattern width / dot size
	//   .y = Pattern height / spacing  
	//   .z = Mortar width
	//   .w = Noise type (0-4) for Perlin: perlin/simplex/ridged/billowy/worley
	
	float Param1, Param2, Param3;
	float PatternX, PatternY, PatternZ, PatternW;
	
	switch (Type)
	{
		case EProceduralType::Radial:
			// Radial uses: falloff (power curve for gradient), softness (unused but available), seed
			Param1 = GetParam(Params, TEXT("falloff"), 2.0f);
			Param2 = GetParam(Params, TEXT("softness"), 0.1f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Circle:
		case EProceduralType::Square:
		case EProceduralType::Diamond:
			// Shapes use: scale (0.1-1.0 for shape size in UV), softness (edge blur), seed
			// Default 0.8 gives good visible shape with some margin
			Param1 = GetParam(Params, TEXT("scale"), 0.8f);
			Param2 = GetParam(Params, TEXT("softness"), 0.02f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Perlin:
			// Perlin uses: scale (frequency 1-64), octaves (1-8), seed
			// noise_type: 0=perlin, 1=simplex, 2=ridged, 3=billowy, 4=worley
			Param1 = GetParam(Params, TEXT("scale"), 4.0f);
			Param2 = GetParam(Params, TEXT("octaves"), 4.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = 0.0f;
			PatternW = GetParam(Params, TEXT("noise_type"), 0.0f);
			break;
			
		case EProceduralType::Voronoi:
			// Voronoi uses: scale (cell density 2-32), edge_width (line thickness 0.01-0.2), seed
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = GetParam(Params, TEXT("edge_width"), 0.05f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Bricks:
			// Bricks uses: ProceduralParams2 for brick dimensions
			Param1 = GetParam(Params, TEXT("scale"), 1.0f);
			Param2 = GetParam(Params, TEXT("softness"), 0.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = GetParam(Params, TEXT("width"), 0.25f);
			PatternY = GetParam(Params, TEXT("height"), 0.1f);
			PatternZ = GetParam(Params, TEXT("mortar"), 0.02f);
			PatternW = 0.0f;
			break;
			
		case EProceduralType::Dots:
			// Dots uses: ProceduralParams2 for dot size and spacing
			Param1 = GetParam(Params, TEXT("scale"), 1.0f);
			Param2 = GetParam(Params, TEXT("softness"), 0.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = GetParam(Params, TEXT("dot_size"), 0.08f);
			PatternY = GetParam(Params, TEXT("spacing"), 0.15f);
			PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::SeamlessNoise:
			// SeamlessNoise uses: scale (frequency 1-32), octaves (1-8), seed
			// seamless_noise_type: 0=standard, 1=ridged, 2=billowy
			Param1 = GetParam(Params, TEXT("scale"), 4.0f);
			Param2 = GetParam(Params, TEXT("octaves"), 4.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = 0.0f;
			PatternW = GetParam(Params, TEXT("seamless_noise_type"), 0.0f);
			break;
			
		case EProceduralType::Crosshatch:
			// Crosshatch: scale, thickness, angle
			Param1 = GetParam(Params, TEXT("scale"), 16.0f);
			Param2 = GetParam(Params, TEXT("thickness"), 0.1f);
			Param3 = GetParam(Params, TEXT("angle"), 0.785f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Waves:
			// Waves: frequency, amplitude, type
			Param1 = GetParam(Params, TEXT("frequency"), 8.0f);
			Param2 = GetParam(Params, TEXT("amplitude"), 0.5f);
			Param3 = GetParam(Params, TEXT("wave_type"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Checkerboard:
			// Checkerboard: scale
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = Param3 = 0.0f;
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Hexagon:
			// Hexagon: scale, edge_thickness, seed
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = GetParam(Params, TEXT("edge_thickness"), 0.1f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Tears:
			// Tears: scale, length, seed
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = GetParam(Params, TEXT("length"), 0.8f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Scratches:
			// Scratches: density, length, seed
			Param1 = GetParam(Params, TEXT("density"), 20.0f);
			Param2 = GetParam(Params, TEXT("length"), 0.5f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Splatter:
			// Splatter: scale, size, seed
			Param1 = GetParam(Params, TEXT("scale"), 4.0f);
			Param2 = GetParam(Params, TEXT("size"), 0.3f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Cracks:
			// Cracks: scale, width, seed
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = GetParam(Params, TEXT("width"), 0.1f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Cells:
			// Cells: scale, contrast, seed
			Param1 = GetParam(Params, TEXT("scale"), 8.0f);
			Param2 = GetParam(Params, TEXT("contrast"), 2.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Grunge:
			// Grunge: scale, detail, seed
			Param1 = GetParam(Params, TEXT("scale"), 4.0f);
			Param2 = GetParam(Params, TEXT("detail"), 1.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Fibers:
			// Fibers: scale, angle, seed
			Param1 = GetParam(Params, TEXT("scale"), 16.0f);
			Param2 = GetParam(Params, TEXT("angle"), 0.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		case EProceduralType::Caustics:
			// Caustics: scale, time, seed
			Param1 = GetParam(Params, TEXT("scale"), 4.0f);
			Param2 = GetParam(Params, TEXT("time"), 0.0f);
			Param3 = GetParam(Params, TEXT("seed"), 0.0f);
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
			break;
			
		default:
			Param1 = Param2 = Param3 = 0.0f;
			PatternX = PatternY = PatternZ = PatternW = 0.0f;
	}
	
	// Dispatch GPU compute
	UTexture2D* Result = FAlphaGenComputeEngine::GenerateGPU(
		GPUType,
		Size,
		Param1,
		Param2,
		Param3,
		PatternX,
		PatternY,
		PatternZ,
		PatternW
	);
	
	if (Result)
	{
		UE_LOG(LogTemp, Log, TEXT("AlphaGen: GPU generated %dx%d [Type=%d] P1=%.2f P2=%.2f P3=%.2f PX=%.2f PY=%.2f PZ=%.2f PW=%.0f"), 
			Size, Size, (int32)Type, Param1, Param2, Param3, PatternX, PatternY, PatternZ, PatternW);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AlphaGen: GPU generation failed"));
	}
	
	return Result;
}

// Direct port from Rust procedural.rs: generate_radial
void FAlphaProceduralGenerator::GenerateRadial(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float Falloff = Params.Contains(TEXT("falloff")) ? Params[TEXT("falloff")] : 2.0f;
	const float Center = Size / 2.0f;
	const float MaxDist = Center;
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			float DX = X - Center;
			float DY = Y - Center;
			float Dist = FMath::Sqrt(DX * DX + DY * DY);
			
			float T = FMath::Min(Dist / MaxDist, 1.0f);
			float Value = FMath::Max(1.0f - FMath::Pow(T, Falloff), 0.0f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
		}
	}
}

// Direct port from Rust procedural.rs: generate_circle
void FAlphaProceduralGenerator::GenerateCircle(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float EdgeSoftness = Params.Contains(TEXT("softness")) ? Params[TEXT("softness")] : 0.02f;
	const float Center = Size / 2.0f;
	const float Radius = Center * 0.95f;
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			float DX = X - Center;
			float DY = Y - Center;
			float Dist = FMath::Sqrt(DX * DX + DY * DY);
			
			float Edge = (Radius - Dist) / (Radius * EdgeSoftness);
			float Value = FMath::Clamp(Edge, 0.0f, 1.0f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
		}
	}
}

// Direct port from Rust procedural.rs: generate_square
void FAlphaProceduralGenerator::GenerateSquare(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float EdgeSoftness = Params.Contains(TEXT("softness")) ? Params[TEXT("softness")] : 0.02f;
	const int32 Padding = static_cast<int32>(Size * 0.05f);
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			bool bInBounds = X >= Padding && X < Size - Padding 
			              && Y >= Padding && Y < Size - Padding;
			
			if (bInBounds)
			{
				int32 DX = FMath::Min(X - Padding, Size - Padding - 1 - X);
				int32 DY = FMath::Min(Y - Padding, Size - Padding - 1 - Y);
				float EdgeDist = static_cast<float>(FMath::Min(DX, DY));
				float SoftnessPixels = FMath::Max(Size * EdgeSoftness, 1.0f);
				float Value = FMath::Min(EdgeDist / SoftnessPixels, 1.0f);
				
				Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
			}
		}
	}
}

// Direct port from Rust procedural.rs: generate_diamond
void FAlphaProceduralGenerator::GenerateDiamond(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float EdgeSoftness = Params.Contains(TEXT("softness")) ? Params[TEXT("softness")] : 0.05f;
	const float Center = Size / 2.0f;
	const float Radius = Center * 0.95f;
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			float DX = FMath::Abs(X - Center);
			float DY = FMath::Abs(Y - Center);
			float ManhattanDist = DX + DY;
			
			float Edge = (Radius - ManhattanDist) / (Radius * EdgeSoftness);
			float Value = FMath::Clamp(Edge, 0.0f, 1.0f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
		}
	}
}

// Noise helper functions - ported from proceduralcommon.ush

float FAlphaProceduralGenerator::Hash21(FVector2f P)
{
	FVector3f P3 = FVector3f(
		FMath::Frac(P.X * 0.1031f),
		FMath::Frac(P.Y * 0.1031f),
		FMath::Frac(P.X * 0.1031f)
	);
	float Dot = P3.X * (P3.Y + 33.33f) + P3.Y * (P3.Z + 33.33f) + P3.Z * (P3.X + 33.33f);
	P3.X += Dot; P3.Y += Dot; P3.Z += Dot;
	return FMath::Frac((P3.X + P3.Y) * P3.Z);
}

FVector2f FAlphaProceduralGenerator::Hash22(FVector2f P)
{
	FVector3f P3 = FVector3f(
		FMath::Frac(P.X * 0.1031f),
		FMath::Frac(P.Y * 0.1030f),
		FMath::Frac(P.X * 0.0973f)
	);
	float Dot = P3.X * (P3.Y + 33.33f) + P3.Y * (P3.Z + 33.33f) + P3.Z * (P3.X + 33.33f);
	P3.X += Dot; P3.Y += Dot; P3.Z += Dot;
	return FVector2f(
		FMath::Frac((P3.X + P3.Y) * P3.Z),
		FMath::Frac((P3.X + P3.Z) * P3.Y)
	);
}

float FAlphaProceduralGenerator::Grad2(FVector2f P)
{
	FVector2f I = FVector2f(FMath::FloorToFloat(P.X), FMath::FloorToFloat(P.Y));
	FVector2f F = FVector2f(FMath::Frac(P.X), FMath::Frac(P.Y));
	
	// Quintic interpolation curve
	FVector2f U = F * F * F * (F * (F * 6.0f - 15.0f) + 10.0f);
	
	// Four corner gradients
	float A = Hash21(I + FVector2f(0.0f, 0.0f));
	float B = Hash21(I + FVector2f(1.0f, 0.0f));
	float C = Hash21(I + FVector2f(0.0f, 1.0f));
	float D = Hash21(I + FVector2f(1.0f, 1.0f));
	
	// Bilinear interpolation
	return FMath::Lerp(FMath::Lerp(A, B, U.X), FMath::Lerp(C, D, U.X), U.Y);
}

float FAlphaProceduralGenerator::FBM(FVector2f P, int32 Octaves, float Lacunarity, float Persistence)
{
	float Value = 0.0f;
	float Amplitude = 0.5f;
	float Frequency = 1.0f;
	float MaxValue = 0.0f;
	
	for (int32 I = 0; I < Octaves; I++)
	{
		Value += Amplitude * (Grad2(P * Frequency) * 2.0f - 1.0f);
		MaxValue += Amplitude;
		Amplitude *= Persistence;
		Frequency *= Lacunarity;
	}
	
	return (Value / MaxValue) * 0.5f + 0.5f; // Normalize to 0-1
}

// Direct port from Rust procedural.rs: generate_perlin
void FAlphaProceduralGenerator::GeneratePerlin(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float Scale = Params.Contains(TEXT("scale")) ? Params[TEXT("scale")] : 4.0f;
	const int32 Octaves = static_cast<int32>(Params.Contains(TEXT("octaves")) ? Params[TEXT("octaves")] : 4.0f);
	const float Seed = Params.Contains(TEXT("seed")) ? Params[TEXT("seed")] : 0.0f;
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			FVector2f UV = FVector2f(
				static_cast<float>(X) / Size,
				static_cast<float>(Y) / Size
			);
			
			FVector2f P = UV * Scale + Seed;
			float Value = FBM(P, Octaves, 2.0f, 0.5f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(FMath::Clamp(Value, 0.0f, 1.0f) * 255.0f);
		}
	}
}

// Direct port from Rust procedural.rs: generate_voronoi
void FAlphaProceduralGenerator::GenerateVoronoi(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const int32 CellCount = static_cast<int32>(Params.Contains(TEXT("cells")) ? Params[TEXT("cells")] : 16.0f);
	const float EdgeWidth = Params.Contains(TEXT("edge_width")) ? Params[TEXT("edge_width")] : 0.1f;
	const uint64 Seed = static_cast<uint64>(Params.Contains(TEXT("seed")) ? Params[TEXT("seed")] : 42.0f);
	
	// Generate cell centers using simple LCG random
	TArray<FVector2f> Centers;
	Centers.Reserve(CellCount);
	uint64 RandState = Seed;
	
	auto NextRandom = [&RandState]() -> float
	{
		RandState = RandState * 6364136223846793005ULL + 1442695040888963407ULL;
		return static_cast<float>((RandState >> 33) & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
	};
	
	for (int32 I = 0; I < CellCount; I++)
	{
		Centers.Add(FVector2f(NextRandom() * Size, NextRandom() * Size));
	}
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			FVector2f P = FVector2f(static_cast<float>(X), static_cast<float>(Y));
			
			// Find two closest centers
			TArray<float> Dists;
			Dists.Reserve(CellCount);
			
			for (const FVector2f& Center : Centers)
			{
				FVector2f Delta = P - Center;
				Dists.Add(FMath::Sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y));
			}
			
			Dists.Sort();
			
			float D1 = Dists[0];
			float D2 = Dists.Num() > 1 ? Dists[1] : D1 + 10.0f;
			
			// Edge detection based on distance difference
			float Edge = (D2 - D1) / (Size * EdgeWidth);
			float Value = FMath::Clamp(Edge, 0.0f, 1.0f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
		}
	}
}

// Direct port from Rust procedural.rs: generate_bricks
void FAlphaProceduralGenerator::GenerateBricks(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float BrickWidthRatio = Params.Contains(TEXT("width")) ? Params[TEXT("width")] : 0.25f;
	const float BrickHeightRatio = Params.Contains(TEXT("height")) ? Params[TEXT("height")] : 0.1f;
	const float MortarRatio = Params.Contains(TEXT("mortar")) ? Params[TEXT("mortar")] : 0.02f;
	
	const int32 BW = static_cast<int32>(Size * BrickWidthRatio);
	const int32 BH = static_cast<int32>(Size * BrickHeightRatio);
	const int32 MW = static_cast<int32>(Size * MortarRatio);
	
	// Prevent division by zero
	if (BW <= 0 || BH <= 0 || MW <= 0)
	{
		return;
	}
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			int32 Row = Y / (BH + MW);
			int32 Offset = (Row % 2 == 1) ? BW / 2 : 0;
			
			int32 BX = (X + Offset) % (BW + MW);
			int32 BY = Y % (BH + MW);
			
			bool bInBrick = BX < BW && BY < BH;
			
			if (bInBrick)
			{
				// Slight variation within brick
				float EdgeX = static_cast<float>(FMath::Min(BX, BW - 1 - BX)) / MW;
				float EdgeY = static_cast<float>(FMath::Min(BY, BH - 1 - BY)) / MW;
				float Edge = FMath::Min(FMath::Min(EdgeX, EdgeY), 1.0f);
				
				Pixels[Y * Size + X] = static_cast<uint8>(Edge * 255.0f);
			}
		}
	}
}

// Direct port from Rust procedural.rs: generate_dots
void FAlphaProceduralGenerator::GenerateDots(TArray<uint8>& Pixels, int32 Size, const TMap<FString, float>& Params)
{
	const float DotSizeRatio = Params.Contains(TEXT("dot_size")) ? Params[TEXT("dot_size")] : 0.08f;
	const float SpacingRatio = Params.Contains(TEXT("spacing")) ? Params[TEXT("spacing")] : 0.15f;
	
	const float DotRadius = Size * DotSizeRatio / 2.0f;
	const float CellSize = Size * SpacingRatio;
	
	// Prevent issues with tiny cell sizes
	if (CellSize < 1.0f)
	{
		return;
	}
	
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			// Find nearest dot center
			float CellX = (FMath::FloorToFloat(X / CellSize) + 0.5f) * CellSize;
			float CellY = (FMath::FloorToFloat(Y / CellSize) + 0.5f) * CellSize;
			
			float DX = X - CellX;
			float DY = Y - CellY;
			float Dist = FMath::Sqrt(DX * DX + DY * DY);
			
			float Value = FMath::Max(1.0f - Dist / DotRadius, 0.0f);
			
			Pixels[Y * Size + X] = static_cast<uint8>(Value * 255.0f);
		}
	}
}

UTexture2D* FAlphaProceduralGenerator::CreateTextureFromPixels(const TArray<uint8>& Pixels, int32 Size)
{
	// Create transient texture - bypass streaming pool to prevent eviction
	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_G8);
	
	if (!Texture)
	{
		UE_LOG(LogTemp, Error, TEXT("AlphaGen: Failed to create transient texture"));
		return nullptr;
	}
	
	// Critical: Disable streaming to prevent pool eviction (gray background fix)
	Texture->NeverStream = true;
	
	// Configure texture settings for grayscale alpha usage
	Texture->CompressionSettings = TC_Grayscale;
	Texture->SRGB = false;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->LODGroup = TEXTUREGROUP_Pixels2D; // Non-streaming group
	
	// Lock and write pixel data
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	
	// Update the texture resource
	Texture->UpdateResource();
	
	return Texture;
}
