//===-- VMD/Motion.cpp - Defines the VMD animation class ------------*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines the VMD::Motion class
///
//===-------------------------------------------------------------------------===//

#include "Motion.h"

#include <Windows.h>

VMD::Motion::Motion()
{
	reset();
}


VMD::Motion::~Motion()
{
}

void VMD::Motion::reset()
{
	CurrentFrame = 0.0f;
	LastKeyFrame = 0;
	Finished = false;
}

bool VMD::Motion::advanceFrame(float Frames)
{
	CurrentFrame += Frames;

	updateCamera(CurrentFrame);

	if (CurrentFrame >= (float)CameraKeyFrames.back().FrameCount) {
		CurrentFrame = (float)CameraKeyFrames.back().FrameCount;

		return true;
	}

	return false;
}

void VMD::Motion::attachCamera(std::shared_ptr<Renderer::Camera> Camera)
{
	AttachedCameras.push_back(Camera);
}

void VMD::Motion::attachModel(std::shared_ptr<PMX::Model> Model)
{
	AttachedModels.push_back(Model);
}

bool VMD::Motion::loadFromFile(const std::wstring &FileName)
{
	std::ifstream InputStream;

	InputStream.open(FileName, std::ios::binary);

	if (!InputStream.good())
		return false;

	char Magic[30];

	InputStream.read(Magic, 30);

	int Version;

	if (!strcmp("Vocaloid Motion Data file", Magic))
		Version = 1;
	else if (!strcmp("Vocaloid Motion Data 0002", Magic))
		Version = 2;
	else {
		InputStream.close();
		return false;
	}

	// Function used to read a Shift-JIS string from an input stream and returns its counterfeit in a std::wstring
	auto readSJISString = [](std::istream &Input, size_t Length) {
		char *ReadBuffer = new char[Length];
		assert(ReadBuffer != nullptr);

		Input.read(ReadBuffer, Length);
		ReadBuffer[Length - 1] = '\0';

		// Gets the required output buffer size
		auto RequiredLength = MultiByteToWideChar(932, 0, ReadBuffer, -1, nullptr, 0);

		// Creates the output buffer
		wchar_t *OutputBuffer = new wchar_t[RequiredLength];
		assert(OutputBuffer != nullptr);

		// Convert the sequence
		auto WrittenCharacters = MultiByteToWideChar(932, 0, ReadBuffer, -1, OutputBuffer, RequiredLength);

		// Create the output string
		std::wstring Output(OutputBuffer, WrittenCharacters);

		// Delete the buffers
		delete[] ReadBuffer;
		delete[] OutputBuffer;

		return Output;
	};

#if 0
	char * ModelName = new char[Version * 10];
	InputStream.read(ModelName, Version * 10);
#else
	// Skips the model name for the animation
	InputStream.seekg(Version * 10, std::ios::cur);
#endif

	uint32_t FrameCount;
	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		BoneKeyFrame Frame;

		Frame.BoneName = readSJISString(InputStream, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)Frame.Translation, sizeof(float) * 3);
		InputStream.read((char*)Frame.Rotation, sizeof(float) * 4);
		InputStream.read((char*)Frame.InterpolationData, 64);

		BoneKeyFrames.push_back(Frame);
	}

	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		MorphKeyFrame Frame;

		Frame.MorphName = readSJISString(InputStream, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)&Frame.Weight, sizeof(float));

		MorphKeyFrames.push_back(Frame);
	}

	// Check for camera data
	if (InputStream.eof()) {
		InputStream.close();
		return true;
	}

	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		CameraKeyFrame Frame;
		int8_t InterpolationData[24];
		float TempVector[3];
		uint32_t DegreesAngle;

		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)&Frame.Distance, sizeof(float));

		InputStream.read((char*)TempVector, sizeof(float) * 3);
		Frame.Position = btVector3(TempVector[0], TempVector[1], TempVector[2]);

		InputStream.read((char*)TempVector, sizeof(float) * 3);
		Frame.Angles = btVector3(TempVector[0], TempVector[1], TempVector[2]);

		InputStream.read((char*)InterpolationData, 24);

		InputStream.read((char*)&DegreesAngle, sizeof(uint32_t));
		Frame.FovAngle = DirectX::XMConvertToRadians((float)DegreesAngle);

		InputStream.read((char*)&Frame.NoPerspective, 1);

		parseCameraInterpolationData(Frame, InterpolationData);

		CameraKeyFrames.push_back(Frame);
	}

	InputStream.close();

	return true;
}

void VMD::Motion::setCameraParameters(float FieldOfView, float Distance, btVector3 &Position, btVector3 &Angles)
{
	for (auto &Camera : AttachedCameras) {
		Camera->SetPosition(Position.get128());
		Camera->SetRotation(Angles.x(), Angles.y(), Angles.z(), true);
		Camera->setFieldOfView(FieldOfView);
		Camera->setFocalDistance(Distance);
	}
}

// The following functions were extracted from MMDAgent project
void VMD::Motion::updateCamera(float Frame)
{
	assert(CameraKeyFrames.size() >= 2);

	// Clamp frame to the last frame of the animation
	if (Frame > (float)CameraKeyFrames.at(CameraKeyFrames.size() - 2).FrameCount) {
		Frame = (float)CameraKeyFrames.at(CameraKeyFrames.size() - 2).FrameCount;
	}

	// Find the next key frame
	uint32_t NextKeyFrame = 0, CurrentKeyFrame = 0;
	if (Frame >= CameraKeyFrames[LastKeyFrame].FrameCount) {
		for (uint32_t i = LastKeyFrame; i < CameraKeyFrames.size(); ++i) {
			if (Frame <= CameraKeyFrames[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}
	}
	else {
		for (uint32_t i = 0; i <= LastKeyFrame && i < CameraKeyFrames.size(); ++i) {
			if (Frame <= CameraKeyFrames[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}
	}

	// Value clamping
	if (NextKeyFrame >= CameraKeyFrames.size()) NextKeyFrame = CameraKeyFrames.size() - 1;

	if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
	else CurrentKeyFrame = NextKeyFrame - 1;

	LastKeyFrame = CurrentKeyFrame;

	float Frame1Time = (float)CameraKeyFrames[CurrentKeyFrame].FrameCount;
	float Frame2Time = (float)CameraKeyFrames[NextKeyFrame].FrameCount;
	CameraKeyFrame& Frame1 = CameraKeyFrames[CurrentKeyFrame];
	CameraKeyFrame& Frame2 = CameraKeyFrames[NextKeyFrame];

	// Do not use interpolation if this is the first key frame, the last keyframe or if the time difference is one frame (camera switch)
	if (Frame1Time == Frame2Time || Frame <= Frame1Time || Frame2Time - Frame1Time <= 1.0f) {
		setCameraParameters(Frame1.FovAngle, Frame1.Distance, Frame1.Position, Frame1.Angles);
		return;
	}

	btVector3 Position;
	float Ratio = (Frame - Frame1Time) / (Frame2Time - Frame1Time);
	uint32_t InterpolationIndex = (uint32_t)(Ratio * InterpolationTableSize);

	auto doLinearInterpolation = [](float Ratio, float V1, float V2) {
		return V1 * (1.0f - Ratio) + V2 * Ratio;
	};
	auto findRatio = [Ratio, InterpolationIndex](std::vector<float> &Table) {
		if (Table.empty()) return Ratio;

		return Table[InterpolationIndex] + (Table[InterpolationIndex + 1] - Table[InterpolationIndex]) * (Ratio * InterpolationTableSize - InterpolationIndex);
	};

	// Calculate the camera parameters
	Position.setX(doLinearInterpolation(findRatio(Frame2.InterpolationData[0]), Frame1.Position.getX(), Frame2.Position.getX()));
	Position.setY(doLinearInterpolation(findRatio(Frame2.InterpolationData[1]), Frame1.Position.getY(), Frame2.Position.getY()));
	Position.setZ(doLinearInterpolation(findRatio(Frame2.InterpolationData[2]), Frame1.Position.getZ(), Frame2.Position.getZ()));
	btVector3 Angles = Frame1.Angles.lerp(Frame2.Angles, findRatio(Frame2.InterpolationData[3]));
	float Distance = doLinearInterpolation(findRatio(Frame2.InterpolationData[4]), Frame1.Distance, Frame2.Distance);
	float FieldOfView = doLinearInterpolation(findRatio(Frame2.InterpolationData[5]), Frame1.FovAngle, Frame2.FovAngle);

	setCameraParameters(FieldOfView, Distance, Position, Angles);
}

void VMD::Motion::parseCameraInterpolationData(CameraKeyFrame &Frame, int8_t *InterpolationData)
{
	float X1, Y1, X2, Y2;

	for (int i = 0; i < 6; ++i) {
		if (InterpolationData[i * 4] == InterpolationData[i * 4 + 2] && InterpolationData[i * 4 + 1] == InterpolationData[i * 4 + 3]) {
			Frame.InterpolationData[i].clear();
			continue;
		}

		Frame.InterpolationData[i].resize(InterpolationTableSize + 1);
		X1 = InterpolationData[i * 4] / 127.0f;
		Y1 = InterpolationData[i * 4 + 2] / 127.0f;
		X2 = InterpolationData[i * 4 + 1] / 127.0f;
		Y2 = InterpolationData[i * 4 + 3] / 127.0f;

		for (int k = 0; k < InterpolationTableSize; ++k) {
			float CurrentFrame = (float)k / (float)InterpolationTableSize;
			float Param = CurrentFrame;

			while (true) {
				float Value = InterpolationFunction(Param, X1, X2) - CurrentFrame;
				if (fabsf(Value) <= 0.0001f) break;

				float ParamDT = InterpolationFunctionDerivative(Param, X1, X2);
				if (ParamDT == 0.0f) break;

				Param -= Value / ParamDT;
			}

			Frame.InterpolationData[i][k] = InterpolationFunction(Param, Y1, Y2);
		}

		Frame.InterpolationData[i][InterpolationTableSize] = 1.0f;
	}
}

float VMD::Motion::InterpolationFunction(float T, float P1, float P2)
{
	return ((1 + 3 * P1 - 3 * P2) * T * T * T + (3 * P2 - 6 * P1) * T * T + 3 * P1 * T);
}

float VMD::Motion::InterpolationFunctionDerivative(float T, float P1, float P2)
{
	return ((3 + 9 * P1 - 9 * P2) * T * T + (6 * P2 - 12 * P1) * T + 3 * P1);
}
