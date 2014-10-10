#include "Motion.h"

using namespace VMD;

Motion::Motion()
{
}


Motion::~Motion()
{
}

bool Motion::Load(const std::wstring &filename)
{
	std::ifstream fs;

	fs.open(filename, std::ios::binary);

	if (!fs.good())
		return false;

	char magic[30];

	fs.read(magic, 30);

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
	fs.read((char*)&count, sizeof(uint32_t));
	boneKeyFrames.resize(count);

	for (uint32_t i = 0; i < count; ++i) {
		fs.read(boneKeyFrames[i].boneName, 15);
		fs.read((char*)&boneKeyFrames[i].frameCount, sizeof(uint32_t));
		fs.read((char*)boneKeyFrames[i].translation, sizeof(float) * 3);
		fs.read((char*)boneKeyFrames[i].rotation, sizeof(float) * 4);
		fs.read((char*)boneKeyFrames[i].interpolationData, 64);
	}

	fs.read((char*)&count, sizeof(uint32_t));
	faceKeyFrames.resize(count);

	for (uint32_t i = 0; i < count; ++i) {
		fs.read(faceKeyFrames[i].name, 15);
		fs.read((char*)&faceKeyFrames[i].frameCount, sizeof(uint32_t));
		fs.read((char*)&faceKeyFrames[i].weight, sizeof(float));
	}

	// Check for camera data
	if (!fs.eof()) {
		fs.read((char*)&count, sizeof(uint32_t));
		cameraKeyFrames.resize(count);

		for (uint32_t i = 0; i < count; ++i) {
			fs.read((char*)&cameraKeyFrames[i].frameCount, sizeof(uint32_t));
			fs.read((char*)&cameraKeyFrames[i].distance, sizeof(float));
			fs.read((char*)cameraKeyFrames[i].position, sizeof(float) * 3);
			fs.read((char*)cameraKeyFrames[i].angles, sizeof(float) * 3);
			fs.read((char*)cameraKeyFrames[i].interpolationData, 24);
			fs.read((char*)&cameraKeyFrames[i].fovAngle, sizeof(uint32_t));
			fs.read((char*)&cameraKeyFrames[i].noPerspective, 1);
		}
	}

	fs.close();

	return true;
}
