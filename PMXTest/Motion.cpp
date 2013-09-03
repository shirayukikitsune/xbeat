#include "stdafx.h"
#include "Motion.h"

#include <fstream>

using VMD::Motion;

Motion::Motion(void)
{
}


Motion::~Motion(void)
{
}

bool Motion::Load(const std::wstring &filename)
{
	uint32_t i;
	std::fstream fs;

	fs.open(filename, std::ios::binary);

	if (!fs.good())
		return false;

	char magic[30];

	fs.read(magic, sizeof (magic));

	int version;

	if (!strcmp("Vocaloid Motion Data file", magic))
		version = 1;
	else if (!strcmp("Vocaloid Motion Data 0002", magic))
		version = 2;
	else {
		fs.close();
		return false;
	}

	char * model = new char[version * 10];
	fs.read(model, version * 10);

	uint32_t count;
	fs.read((char*)&count, sizeof (uint32_t));
	keyFrames.resize(count);
	
	//for (i = 0; i < count; i++) {
		fs.read((char*)&keyFrames[0], sizeof (KeyFrame) * count);
	//}

	fs.read((char*)&count, sizeof (uint32_t));
	faceKeyFrames.resize(count);
	
	//for (i = 0; i < count; i++) {
	fs.read((char*)&faceKeyFrames[0], sizeof (FaceKeyFrame) * count);

	fs.close();

	return true;
}
