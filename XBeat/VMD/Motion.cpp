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
#include "../PMX/PMXModel.h"

#include <algorithm>
#include <Camera.h>
#include <Context.h>
#include <RigidBody.h>
#include <Sort.h>
#include <Windows.h>

VMD::Motion::Motion(Urho3D::Context *context)
	: Resource(context)
{
	reset();
	MaxFrame = 0.0f;
}


VMD::Motion::~Motion()
{
}

void VMD::Motion::RegisterObject(Urho3D::Context *context)
{
	context->RegisterFactory<VMD::Motion>();
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

	if (!AttachedCameras.Empty())
		updateCamera(CurrentFrame);

	if (!AttachedModels.Empty()) {
		for (auto ModelIt = AttachedModels.Begin(); ModelIt != AttachedModels.End(); ++ModelIt) {
			auto pmx = (*ModelIt)->GetComponent<PMXAnimatedModel>();
#if 0
			auto model = static_cast<PMXModel*>(pmx->GetModel());
			auto bones = pmx->GetSkeleton().GetModifiableBones();
			auto pmxit = model->GetBones().Begin(), pmxend = model->GetBones().End();
			for (auto boneit = bones.Begin(), boneend = bones.End(); boneit != boneend; ++boneit) {
				if (boneit->animated_ && boneit->node_) {
					boneit->node_->SetTransform(boneit->initialPosition_, boneit->initialRotation_, boneit->initialScale_);
				}
			}
#else
			pmx->GetSkeleton().Reset();
#endif
		}

		updateBones(CurrentFrame);
		updateMorphs(CurrentFrame);
	}

	return false;
}

void VMD::Motion::attachCamera(Urho3D::Node* CameraNode)
{
	if (CameraNode->GetComponent<Urho3D::Camera>() != nullptr)
		AttachedCameras.Push(CameraNode);
}

void VMD::Motion::attachModel(Urho3D::Node* ModelNode)
{
	if (ModelNode->GetComponent<PMXAnimatedModel>() != nullptr)
		AttachedModels.Push(ModelNode);
}

bool VMD::Motion::BeginLoad(Urho3D::Deserializer &source)
{
	char Magic[30];

	if (source.Read(Magic, 30) != 30)
		return false;

	int Version;

	if (!strcmp("Vocaloid Motion Data file", Magic))
		Version = 1;
	else if (!strcmp("Vocaloid Motion Data 0002", Magic))
		Version = 2;
	else
		return false;

	// Function used to read a Shift-JIS string from an input stream and returns its counterfeit in a std::wstring
	auto readSJISString = [](Urho3D::Deserializer &source, unsigned int Length) {
		char *ReadBuffer = new char[Length];
		assert(ReadBuffer != nullptr);

		source.Read(ReadBuffer, Length);
		ReadBuffer[Length - 1] = '\0';

		// Gets the required output buffer size
		auto RequiredLength = MultiByteToWideChar(932, 0, ReadBuffer, -1, nullptr, 0);

		// Creates the output buffer
		wchar_t *OutputBuffer = new wchar_t[RequiredLength];
		assert(OutputBuffer != nullptr);

		// Convert the sequence
		auto WrittenCharacters = MultiByteToWideChar(932, 0, ReadBuffer, -1, OutputBuffer, RequiredLength);

		// Create the output string
		Urho3D::String Output(OutputBuffer);

		// Delete the buffers
		delete[] ReadBuffer;
		delete[] OutputBuffer;

		return Output;
	};

	Urho3D::String ModelName = readSJISString(source, Version * 10);

	unsigned int FrameCount = source.ReadUInt();

	float data[4];

	while (FrameCount --> 0) {
		BoneKeyFrame Frame;
		char InterpolationData[64];

		Frame.BoneName = readSJISString(source, 15);
		Frame.FrameCount = source.ReadUInt();
		Frame.Translation = source.ReadVector3();
		source.Read(data, sizeof(float) * 4);
		Frame.Rotation = Urho3D::Quaternion(data[3], data[0], data[1], data[2]);
		source.Read(InterpolationData, 64);

		parseBoneInterpolationData(Frame, InterpolationData);

		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		auto BoneMotion = BoneKeyFrames.Find(Frame.BoneName);
		if (BoneMotion == BoneKeyFrames.End()) {
			Urho3D::Vector<BoneKeyFrame> frameList;
			frameList.Push(Frame);
			BoneKeyFrames.Insert(Urho3D::MakePair(Urho3D::StringHash(Frame.BoneName), frameList));
		}
		else
			BoneKeyFrames[Frame.BoneName].Push(Frame);
	}

	// Sort the bone motion by the key frames
	for (auto BoneIt = BoneKeyFrames.Begin(); BoneIt != BoneKeyFrames.End(); ++BoneIt) {
		Urho3D::Sort(BoneIt->second_.Begin(), BoneIt->second_.End(), [](BoneKeyFrame &a, BoneKeyFrame &b) {
			return a.FrameCount < b.FrameCount;
		});
	}

	FrameCount = source.ReadUInt();

	while (FrameCount --> 0) {
		MorphKeyFrame Frame;

		Frame.MorphName = readSJISString(source, 15);
		Frame.FrameCount = source.ReadUInt();
		Frame.Weight = source.ReadFloat();
		
		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		auto MorphFrame = MorphKeyFrames.Find(Frame.MorphName);
		if (MorphFrame == MorphKeyFrames.End()) {
			Urho3D::Vector<MorphKeyFrame> frameList;
			frameList.Push(Frame);
			MorphKeyFrames.Insert(Urho3D::MakePair(Urho3D::StringHash(Frame.MorphName), frameList));
		}
		else MorphKeyFrames[Frame.MorphName].Push(Frame);
	}

	// Check if the camera data is present
	if (source.IsEof()) {
		return true;
	}

	FrameCount = source.ReadUInt();

	while (FrameCount --> 0) {
		CameraKeyFrame Frame;
		char InterpolationData[24];

		Frame.FrameCount = source.ReadUInt();
		Frame.Distance = source.ReadFloat();
		Frame.Position = source.ReadVector3();

		Urho3D::Vector3 rotationEuler = source.ReadVector3();
		Frame.Rotation.FromEulerAngles(rotationEuler.x_, rotationEuler.y_, rotationEuler.z_);

		source.Read(InterpolationData, 24);

		Frame.FovAngle = source.ReadUInt() * Urho3D::M_DEGTORAD;
		Frame.NoPerspective = source.ReadUByte();

		parseCameraInterpolationData(Frame, InterpolationData);

		MaxFrame = std::max(MaxFrame, (float)Frame.FrameCount);

		CameraKeyFrames.Push(Frame);
	}

	return true;
}

void VMD::Motion::setCameraParameters(float FieldOfView, float Distance, Urho3D::Vector3 &Position, Urho3D::Quaternion &Rotation)
{
	for (auto it = AttachedCameras.Begin(); it != AttachedCameras.End(); ++it) {
		auto cameraNode = *it;
		cameraNode->SetRotation(Rotation);
		cameraNode->SetPosition(Position);
		auto cameraComponent = cameraNode->GetComponent<Urho3D::Camera>();
		cameraComponent->SetFov(FieldOfView);
		cameraComponent->SetLodBias(Distance);
	}
}

void VMD::Motion::setBoneParameters(Urho3D::String BoneName, Urho3D::Vector3 &Position, Urho3D::Quaternion &Rotation)
{
	for (auto it = AttachedModels.Begin(); it != AttachedModels.End(); ++it) {
		auto modelNode = *it;
		auto bone = modelNode->GetComponent<PMXAnimatedModel>()->GetSkeleton().GetBone(BoneName);
		if (bone) {
			auto boneNode = bone->node_;
			boneNode->SetPosition(Position + bone->initialPosition_);
			boneNode->SetRotation(Rotation);
		}
	}
}

void VMD::Motion::setMorphParameters(Urho3D::String MorphName, float MorphWeight)
{
	// TODO: Port this to Urho
	/*for (auto it = AttachedModels.Begin(); it != AttachedModels.End(); ++it) {
		auto modelNode = it->Lock();
		if (modelNode) {
			modelNode->GetComponent<PMXModel>()->ApplyMorph(MorphName, MorphWeight);
		}
	}*/
}

// The following functions were extracted from MMDAgent project
void VMD::Motion::updateCamera(float Frame)
{
	if (CameraKeyFrames.Empty()) return;

	// Clamp frame to the last frame of the animation
	if (Frame > (float)CameraKeyFrames.Back().FrameCount) {
		Frame = (float)CameraKeyFrames.Back().FrameCount;
	}

	// Find the next key frame
	unsigned int NextKeyFrame = 0, CurrentKeyFrame = 0;
	for (unsigned int i = 0; i < CameraKeyFrames.Size(); ++i) {
		if (Frame <= CameraKeyFrames[i].FrameCount) {
			NextKeyFrame = i;
			break;
		}
	}

	// Value clamping
	if (NextKeyFrame >= CameraKeyFrames.Size()) NextKeyFrame = CameraKeyFrames.Size() - 1;

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

	Urho3D::Vector3 Position;
	float Ratio = (Frame - Frame1Time) / (Frame2Time - Frame1Time);
	unsigned int InterpolationIndex = (unsigned int)(Ratio * InterpolationTableSize);

	auto findRatio = [Ratio, InterpolationIndex](Urho3D::PODVector<float> &Table) {
		if (Table.Empty()) return Ratio;

		return Table[InterpolationIndex] + (Table[InterpolationIndex + 1] - Table[InterpolationIndex]) * (Ratio * InterpolationTableSize - InterpolationIndex);
	};

	// Calculate the camera parameters
	Position.x_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[0]), Frame1.Position.x_, Frame2.Position.x_);
	Position.y_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[1]), Frame1.Position.y_, Frame2.Position.y_);
	Position.z_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[2]), Frame1.Position.z_, Frame2.Position.z_);
	Urho3D::Quaternion Rotation = Frame1.Rotation.Slerp(Frame2.Rotation, findRatio(Frame2.InterpolationData[3]));
	float Distance = doLinearInterpolation(findRatio(Frame2.InterpolationData[4]), Frame1.Distance, Frame2.Distance);
	float FieldOfView = doLinearInterpolation(findRatio(Frame2.InterpolationData[5]), Frame1.FovAngle, Frame2.FovAngle);

	setCameraParameters(FieldOfView, Distance, Position, Rotation);
}

void VMD::Motion::updateBones(float Frame)
{
	for (auto it = BoneKeyFrames.Begin(); it != BoneKeyFrames.End(); ++it) {
		float animationFrame = Frame;
		if (it->second_.Size() == 1) {
			if (it->second_.Front().FrameCount > Frame) continue;

			setBoneParameters(it->second_.Front().BoneName, it->second_.Front().Translation, it->second_.Front().Rotation);
			continue;
		}

		auto &BoneKeyFrames = it->second_;

		// Clamp frame to the last frame of the animation
		if (animationFrame > (float)BoneKeyFrames.Back().FrameCount) {
			animationFrame = (float)BoneKeyFrames.Back().FrameCount;
		}

		// Find the next key frame
		unsigned int NextKeyFrame = 0, CurrentKeyFrame = 0;
		for (unsigned int i = 0; i < BoneKeyFrames.Size(); ++i) {
			if (animationFrame <= BoneKeyFrames[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}

		// Value clamping
		if (NextKeyFrame >= BoneKeyFrames.Size()) NextKeyFrame = BoneKeyFrames.Size() - 1;

		if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
		else CurrentKeyFrame = NextKeyFrame - 1;

		float Frame1Time = (float)BoneKeyFrames[CurrentKeyFrame].FrameCount;
		float Frame2Time = (float)BoneKeyFrames[NextKeyFrame].FrameCount;
		BoneKeyFrame& Frame1 = BoneKeyFrames[CurrentKeyFrame];
		BoneKeyFrame& Frame2 = BoneKeyFrames[NextKeyFrame];

		if (Frame1Time == Frame2Time || animationFrame <= Frame1Time) {
			setBoneParameters(Frame1.BoneName, Frame1.Translation, Frame1.Rotation);
			return;
		}
		else if (animationFrame >= Frame2Time) {
			setBoneParameters(Frame2.BoneName, Frame2.Translation, Frame2.Rotation);
			return;
		}

		float Ratio = (animationFrame - Frame1Time) / (Frame2Time - Frame1Time);
		unsigned int InterpolationIndex = (unsigned int)(Ratio * InterpolationTableSize);

		auto findRatio = [Ratio, InterpolationIndex](Urho3D::PODVector<float> &Table) {
			if (Table.Empty()) return Ratio;

			return Table[InterpolationIndex] + (Table[InterpolationIndex + 1] - Table[InterpolationIndex]) * (Ratio * InterpolationTableSize - InterpolationIndex);
		};

		Urho3D::Vector3 Translation;
		Translation.x_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[0]), Frame1.Translation.x_, Frame2.Translation.x_);
		Translation.y_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[1]), Frame1.Translation.y_, Frame2.Translation.y_);
		Translation.z_ = doLinearInterpolation(findRatio(Frame2.InterpolationData[2]), Frame1.Translation.z_, Frame2.Translation.z_);
		Urho3D::Quaternion Rotation;
		Rotation = Frame1.Rotation.Slerp(Frame2.Rotation, findRatio(Frame2.InterpolationData[3]));

		setBoneParameters(Frame1.BoneName, Translation, Rotation);
	}
}

void VMD::Motion::updateMorphs(float currentFrame)
{
	for (auto it = MorphKeyFrames.Begin(); it != MorphKeyFrames.End(); ++it) {
		float animationFrame = currentFrame;
		auto &Frame = it->second_;

		if (Frame.Size() == 1) {
			if (Frame.Front().FrameCount > animationFrame) continue;

			setMorphParameters(Frame.Front().MorphName, Frame.Front().Weight);
			continue;
		}

		if (animationFrame > Frame.Back().FrameCount)
			animationFrame = (float)Frame.Back().FrameCount;

		// Find the next key frame
		unsigned int NextKeyFrame = 0, CurrentKeyFrame = 0;
		for (unsigned int i = 0; i < Frame.Size(); ++i) {
			if (animationFrame <= Frame[i].FrameCount) {
				NextKeyFrame = i;
				break;
			}
		}

		// Value clamping
		if (NextKeyFrame >= Frame.Size()) NextKeyFrame = Frame.Size() - 1;

		if (NextKeyFrame <= 1) CurrentKeyFrame = 0;
		else CurrentKeyFrame = NextKeyFrame - 1;

		float Frame1Time = (float)Frame[CurrentKeyFrame].FrameCount;
		float Frame2Time = (float)Frame[NextKeyFrame].FrameCount;
		auto& Frame1 = Frame[CurrentKeyFrame];
		auto& Frame2 = Frame[NextKeyFrame];

		if (Frame1Time == Frame2Time || animationFrame <= Frame1Time) {
			setMorphParameters(Frame1.MorphName, Frame1.Weight);
			return;
		}
		else if (animationFrame >= Frame2Time) {
			setMorphParameters(Frame2.MorphName, Frame2.Weight);
			return;
		}

		float Ratio = (animationFrame - Frame1Time) / (Frame2Time - Frame1Time);

		setMorphParameters(Frame1.MorphName, doLinearInterpolation(Ratio, Frame1.Weight, Frame2.Weight));
	}
}

void VMD::Motion::parseCameraInterpolationData(CameraKeyFrame &Frame, char *InterpolationData)
{
	float X1, Y1, X2, Y2;

	for (int i = 0; i < 6; ++i) {
		if (InterpolationData[i * 4] == InterpolationData[i * 4 + 2] && InterpolationData[i * 4 + 1] == InterpolationData[i * 4 + 3]) {
			Frame.InterpolationData[i].Clear();
			continue;
		}

		Frame.InterpolationData[i].Resize(InterpolationTableSize + 1);
		X1 = InterpolationData[i * 4] / 127.0f;
		Y1 = InterpolationData[i * 4 + 2] / 127.0f;
		X2 = InterpolationData[i * 4 + 1] / 127.0f;
		Y2 = InterpolationData[i * 4 + 3] / 127.0f;

		generateInterpolationTable(Frame.InterpolationData[i], X1, X2, Y1, Y2);
	}
}

void VMD::Motion::parseBoneInterpolationData(BoneKeyFrame &Frame, char *InterpolationData)
{
	float X1, Y1, X2, Y2;

	for (int i = 0; i < 4; ++i) {
		if (InterpolationData[i] == InterpolationData[i + 4] && InterpolationData[i + 8] == InterpolationData[i + 12]) {
			Frame.InterpolationData[i].Clear();
			continue;
		}

		Frame.InterpolationData[i].Resize(InterpolationTableSize + 1);
		X1 = InterpolationData[i] / 127.0f;
		Y1 = InterpolationData[i + 4] / 127.0f;
		X2 = InterpolationData[i + 8] / 127.0f;
		Y2 = InterpolationData[i + 12] / 127.0f;

		generateInterpolationTable(Frame.InterpolationData[i], X1, X2, Y1, Y2);
	}
}

void VMD::Motion::generateInterpolationTable(Urho3D::PODVector<float> &Table, float X1, float X2, float Y1, float Y2)
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
