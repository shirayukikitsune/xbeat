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

VMD::Motion::Motion()
{
}


VMD::Motion::~Motion()
{
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

#if 0
	char * ModelName = new char[Version * 10];
	InputStream.read(ModelName, Version * 10);
#endif
	// Skips the model name for the animation
	InputStream.seekg(Version * 10, std::ios::cur);

	uint32_t FrameCount;
	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		BoneKeyFrame Frame;

		InputStream.read(Frame.BoneName, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)Frame.Translation, sizeof(float) * 3);
		InputStream.read((char*)Frame.Rotation, sizeof(float) * 4);
		InputStream.read((char*)Frame.InterpolationData, 64);

		BoneKeyFrames.push(Frame);
	}

	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		MorphKeyFrame Frame;

		InputStream.read(Frame.MorphName, 15);
		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)&Frame.Weight, sizeof(float));

		MorphKeyFrames.push(Frame);
	}

	// Check for camera data
	if (InputStream.eof()) {
		InputStream.close();
		return true;
	}

	InputStream.read((char*)&FrameCount, sizeof(uint32_t));

	while (FrameCount --> 0) {
		CameraKeyFrame Frame;

		InputStream.read((char*)&Frame.FrameCount, sizeof(uint32_t));
		InputStream.read((char*)&Frame.Distance, sizeof(float));
		InputStream.read((char*)Frame.Position, sizeof(float) * 3);
		InputStream.read((char*)Frame.Angles, sizeof(float) * 3);
		InputStream.read((char*)Frame.InterpolationData, 24);
		InputStream.read((char*)&Frame.FovAngle, sizeof(uint32_t));
		InputStream.read((char*)&Frame.NoPerspective, 1);

		CameraKeyFrames.push(Frame);
	}

	InputStream.close();

	return true;
}
