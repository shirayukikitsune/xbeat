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
	MaxFrame = 0.0f;
}


VMD::Motion::~Motion()
{
}

void VMD::Motion::reset()
{
	CurrentFrame = 0.0f;
	Finished = false;
}

bool VMD::Motion::advanceFrame(float Frames)
{
	CurrentFrame += Frames;

	if (CurrentFrame >= MaxFrame) {
		Finished = true;
		return true;
	}

	if (!AttachedCameras.empty())
		updateCamera(CurrentFrame);

	if (!AttachedModels.empty()) {
		for (auto &Model : AttachedModels)
			Model->Reset();

		updateBones(CurrentFrame);
		updateMorphs(CurrentFrame);
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
		std::wstring Output(OutputBuffer);

		// Delete the buffers
		delete[] ReadBuffer;
		delete[] OutputBuffer;

		return Output;
	};

#if 1
	std::wstring ModelName = readSJISString(InputStream, Version * 10);
#else
	// Skips the model name for the animation
	InputStream.seekg(Version * 10, std::ios::cur);
#endif

	uint32_t FrameCount;
	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		BoneKeyFrame Frame;
		int8_t InterpolationData[64];
		float TempVector[4];

		Frame.BoneName = readSJISString(InputStream, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)TempVector, sizeof(float) * 3);
		Frame.Translation = btVector3(TempVector[0], TempVector[1], TempVector[2]);
		InputStream.read((char*)TempVector, sizeof(float) * 4);
		Frame.Rotation = btQuaternion(TempVector[0], TempVector[1], TempVector[2], TempVector[3]);
		InputStream.read((char*)InterpolationData, 64);

		parseBoneInterpolationData(Frame, InterpolationData);

		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		auto BoneMotion = BoneKeyFrames.find(Frame.BoneName);
		if (BoneMotion == BoneKeyFrames.end())
			BoneKeyFrames[Frame.BoneName] = { Frame };
		else BoneKeyFrames[Frame.BoneName].emplace_back(Frame);
	}

	// Sort the bone motion by the key frames
	for (auto &BoneMotion : BoneKeyFrames) {
		std::sort(BoneMotion.second.begin(), BoneMotion.second.end(), [](BoneKeyFrame &a, BoneKeyFrame &b) {
			return a.FrameCount < b.FrameCount;
		});
	}

	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		MorphKeyFrame Frame;

		Frame.MorphName = readSJISString(InputStream, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)&Frame.Weight, sizeof(float));

		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		auto MorphFrame = MorphKeyFrames.find(Frame.MorphName);
		if (MorphFrame == MorphKeyFrames.end())
			MorphKeyFrames[Frame.MorphName] = { Frame };
		else MorphKeyFrames[Frame.MorphName].emplace_back(Frame);
	}

	// Check if the camera data is present
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
		Frame.Rotation.setEulerZYX(TempVector[0], TempVector[1], TempVector[2]);

		InputStream.read((char*)InterpolationData, 24);

		InputStream.read((char*)&DegreesAngle, sizeof(uint32_t));
		Frame.FovAngle = DirectX::XMConvertToRadians((float)DegreesAngle);

		InputStream.read((char*)&Frame.NoPerspective, 1);

		parseCameraInterpolationData(Frame, InterpolationData);

		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		CameraKeyFrames.push_back(Frame);
	}

	InputStream.close();

	return true;
}

void VMD::Motion::setCameraParameters(float FieldOfView, float Distance, btVector3 &Position, btQuaternion &Rotation)
{
	for (auto &Camera : AttachedCameras) {
		Camera->SetRotation(Rotation.get128());
		Camera->SetPosition(Position.get128());
		Camera->setFieldOfView(FieldOfView);
		Camera->setFocalDistance(Distance);
	}
}

void VMD::Motion::setBoneParameters(std::wstring BoneName, btVector3 &Translation, btQuaternion &Rotation)
{
	for (auto &Model : AttachedModels) {
		auto bone = Model->GetBoneByName(BoneName);
		if (bone) {
			bone->Transform(btTransform(Rotation, Translation), PMX::DeformationOrigin::Internal);
		}
	}
}

void VMD::Motion::setMorphParameters(std::wstring MorphName, float MorphWeight)
{
	for (auto &Model : AttachedModels) {
		Model->ApplyMorph(MorphName, MorphWeight);
	}
}

// The following functions were extracted from MMDAgent project
void VMD::Motion::updateCamera(float Frame)
{
	if (CameraKeyFrames.empty()) return;

	// Clamp frame to the last frame of the animation
	if (Frame > (float)CameraKeyFrames.back().FrameCount) {
		Frame = (float)CameraKeyFrames.back().FrameCount;
	}

	// Find the next key frame
	size_t NextKeyFrame = 0, CurrentKeyFrame = 0;
	for (size_t i = 0; i < CameraKeyFrames.size(); ++i) {
		if (Frame <= CameraKeyFrames[i].FrameCount) {
			NextKeyFrame = i;
			break;
		}
	}

	// Value clamping
	if (NextKeyFrame >= CameraKeyFrames.size()) NextKeyFrame = CameraKeyFrames.size() - 1;

	if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
	else CurrentKeyFrame = NextKeyFrame - 1;

	float Frame1Time = (float)CameraKeyFrames[CurrentKeyFrame].FrameCount;
	float Frame2Time = (float)CameraKeyFrames[NextKeyFrame].FrameCount;
	CameraKeyFrame& Frame1 = CameraKeyFrames[CurrentKeyFrame];
	CameraKeyFrame& Frame2 = CameraKeyFrames[NextKeyFrame];

	// Do not use interpolation if this is the first key frame, the last keyframe or if the time difference is one frame (camera switch)
	if (Frame1Time == Frame2Time || Frame <= Frame1Time || Frame2Time - Frame1Time <= 1.0f) {
		setCameraParameters(Frame1.FovAngle, Frame1.Distance, Frame1.Position, Frame1.Rotation);
		return;
	}

	btVector3 Position;
	float Ratio = (Frame - Frame1Time) / (Frame2Time - Frame1Time);
	uint32_t InterpolationIndex = (uint32_t)(Ratio * InterpolationTableSize);

	auto findRatio = [Ratio, InterpolationIndex](std::vector<float> &Table) {
		if (Table.empty()) return Ratio;

		return Table[InterpolationIndex] + (Table[InterpolationIndex + 1] - Table[InterpolationIndex]) * (Ratio * InterpolationTableSize - InterpolationIndex);
	};

	// Calculate the camera parameters
	Position.setX(doLinearInterpolation(findRatio(Frame2.InterpolationData[0]), Frame1.Position.getX(), Frame2.Position.getX()));
	Position.setY(doLinearInterpolation(findRatio(Frame2.InterpolationData[1]), Frame1.Position.getY(), Frame2.Position.getY()));
	Position.setZ(doLinearInterpolation(findRatio(Frame2.InterpolationData[2]), Frame1.Position.getZ(), Frame2.Position.getZ()));
	btQuaternion Rotation = Frame1.Rotation.slerp(Frame2.Rotation, findRatio(Frame2.InterpolationData[3]));
	float Distance = doLinearInterpolation(findRatio(Frame2.InterpolationData[4]), Frame1.Distance, Frame2.Distance);
	float FieldOfView = doLinearInterpolation(findRatio(Frame2.InterpolationData[5]), Frame1.FovAngle, Frame2.FovAngle);

	setCameraParameters(FieldOfView, Distance, Position, Rotation);
}

void VMD::Motion::updateBones(float Frame)
{
	for (auto BoneMotion = BoneKeyFrames.begin(); BoneMotion != BoneKeyFrames.end(); ) {
		if (BoneMotion->second.size() == 1) {
			if (BoneMotion->second.front().FrameCount > Frame) continue;

			setBoneParameters(BoneMotion->first, BoneMotion->second.front().Translation, BoneMotion->second.front().Rotation);
			std::wstring key = BoneMotion->first;
			BoneMotion++;
			std::wstring nextKey;
			if (BoneMotion != BoneKeyFrames.end()) {
				nextKey = BoneMotion->first;
			}
			BoneMotion = BoneKeyFrames.find(nextKey);
			BoneKeyFrames.erase(key);
			continue;
		}

		auto &BoneKeyFrames = BoneMotion->second;

		// Clamp frame to the last frame of the animation
		if (Frame > (float)BoneKeyFrames.back().FrameCount) {
			Frame = (float)BoneKeyFrames.back().FrameCount;
		}

		// Find the next key frame
		size_t NextKeyFrame = 0, CurrentKeyFrame = 0;
		for (size_t i = 0; i < BoneKeyFrames.size(); ++i) {
			if (Frame <= BoneKeyFrames[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}

		// Value clamping
		if (NextKeyFrame >= BoneKeyFrames.size()) NextKeyFrame = BoneKeyFrames.size() - 1;

		if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
		else CurrentKeyFrame = NextKeyFrame - 1;

		float Frame1Time = (float)BoneKeyFrames[CurrentKeyFrame].FrameCount;
		float Frame2Time = (float)BoneKeyFrames[NextKeyFrame].FrameCount;
		BoneKeyFrame& Frame1 = BoneKeyFrames[CurrentKeyFrame];
		BoneKeyFrame& Frame2 = BoneKeyFrames[NextKeyFrame];

		if (Frame1Time == Frame2Time || Frame <= Frame1Time) {
			setBoneParameters(Frame1.BoneName, Frame1.Translation, Frame1.Rotation);
			return;
		}
		else if (Frame >= Frame2Time) {
			setBoneParameters(Frame2.BoneName, Frame2.Translation, Frame2.Rotation);
			return;
		}

		float Ratio = (Frame - Frame1Time) / (Frame2Time - Frame1Time);
		uint32_t InterpolationIndex = (uint32_t)(Ratio * InterpolationTableSize);

		auto findRatio = [Ratio, InterpolationIndex](std::vector<float> &Table) {
			if (Table.empty()) return Ratio;

			return Table[InterpolationIndex] + (Table[InterpolationIndex + 1] - Table[InterpolationIndex]) * (Ratio * InterpolationTableSize - InterpolationIndex);
		};

		btVector3 Translation;
		Translation.setX(doLinearInterpolation(findRatio(Frame2.InterpolationData[0]), Frame1.Translation.getX(), Frame2.Translation.getX()));
		Translation.setY(doLinearInterpolation(findRatio(Frame2.InterpolationData[1]), Frame1.Translation.getY(), Frame2.Translation.getY()));
		Translation.setZ(doLinearInterpolation(findRatio(Frame2.InterpolationData[2]), Frame1.Translation.getZ(), Frame2.Translation.getZ()));
		btQuaternion Rotation;
		Rotation = Frame1.Rotation.slerp(Frame2.Rotation, findRatio(Frame2.InterpolationData[3]));

		setBoneParameters(Frame1.BoneName, Translation, Rotation);

		++BoneMotion;
	}
}

void VMD::Motion::updateMorphs(float CurrentFrame)
{
	for (auto MorphFrame = MorphKeyFrames.begin(); MorphFrame != MorphKeyFrames.end(); ) {
		if (MorphFrame->second.size() == 1) {
			if (MorphFrame->second.front().FrameCount > CurrentFrame) continue;

			setMorphParameters(MorphFrame->second.front().MorphName, MorphFrame->second.front().Weight);
			std::wstring key = MorphFrame->first;
			++MorphFrame;
			std::wstring nextKey;
			if (MorphFrame != MorphKeyFrames.end()) {
				nextKey = MorphFrame->first;
			}
			MorphFrame = MorphKeyFrames.find(nextKey);
			MorphKeyFrames.erase(key);
			continue;
		}

		auto &Frame = MorphFrame->second;

		if (CurrentFrame > Frame.back().FrameCount)
			CurrentFrame = (float)Frame.back().FrameCount;

		// Find the next key frame
		size_t NextKeyFrame = 0, CurrentKeyFrame = 0;
		for (size_t i = 0; i < Frame.size(); ++i) {
			if (CurrentFrame <= Frame[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}

		// Value clamping
		if (NextKeyFrame >= Frame.size()) NextKeyFrame = Frame.size() - 1;

		if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
		else CurrentKeyFrame = NextKeyFrame - 1;

		float Frame1Time = (float)Frame[CurrentKeyFrame].FrameCount;
		float Frame2Time = (float)Frame[NextKeyFrame].FrameCount;
		auto& Frame1 = Frame[CurrentKeyFrame];
		auto& Frame2 = Frame[NextKeyFrame];

		if (Frame1Time == Frame2Time || CurrentFrame <= Frame1Time) {
			setMorphParameters(Frame1.MorphName, Frame1.Weight);
			return;
		}
		else if (CurrentFrame >= Frame2Time) {
			setMorphParameters(Frame2.MorphName, Frame2.Weight);
			return;
		}

		float Ratio = (CurrentFrame - Frame1Time) / (Frame2Time - Frame1Time);

		setMorphParameters(Frame1.MorphName, doLinearInterpolation(Ratio, Frame1.Weight, Frame2.Weight));

		++MorphFrame;
	}
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

		generateInterpolationTable(Frame.InterpolationData[i], X1, X2, Y1, Y2);
	}
}

void VMD::Motion::parseBoneInterpolationData(BoneKeyFrame &Frame, int8_t *InterpolationData)
{
	float X1, Y1, X2, Y2;

	for (int i = 0; i < 4; ++i) {
		if (InterpolationData[i] == InterpolationData[i + 4] && InterpolationData[i + 8] == InterpolationData[i + 12]) {
			Frame.InterpolationData[i].clear();
			continue;
		}

		Frame.InterpolationData[i].resize(InterpolationTableSize + 1);
		X1 = InterpolationData[i] / 127.0f;
		Y1 = InterpolationData[i + 4] / 127.0f;
		X2 = InterpolationData[i + 8] / 127.0f;
		Y2 = InterpolationData[i + 12] / 127.0f;

		generateInterpolationTable(Frame.InterpolationData[i], X1, X2, Y1, Y2);
	}
}

void VMD::Motion::generateInterpolationTable(std::vector<float> &Table, float X1, float X2, float Y1, float Y2)
{
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

		Table[k] = InterpolationFunction(Param, Y1, Y2);
	}

	Table[InterpolationTableSize] = 1.0f;
}

float VMD::Motion::InterpolationFunction(float T, float P1, float P2)
{
	return ((1 + 3 * P1 - 3 * P2) * T * T * T + (3 * P2 - 6 * P1) * T * T + 3 * P1 * T);
}

float VMD::Motion::InterpolationFunctionDerivative(float T, float P1, float P2)
{
	return ((3 + 9 * P1 - 9 * P2) * T * T + (6 * P2 - 12 * P1) * T + 3 * P1);
}
