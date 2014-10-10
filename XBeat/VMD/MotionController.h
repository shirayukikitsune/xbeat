#pragma once

#include <vector>
#include <string>
#include <boost/filesystem/path.hpp>
#include <memory>

namespace VMD {

	class Motion;

	class MotionController
	{
	public:
		MotionController();
		~MotionController();

		void advanceFrame(float time);

		std::shared_ptr<Motion> loadMotion(std::wstring motionName);

	private:
		std::vector<std::shared_ptr<Motion>> knownMotions;
	};

}
