#pragma once

#include "MMDAgent.h"

#define MAX_JOYSTICKS GLFW_JOYSTICK_LAST + 1

#include <map>
#include <functional>

enum {
	POV_X,
	POV_Y
};

enum JoyButtonEventType {
	JOYBTNEVENT_UP,
	JOYBTNEVENT_DOWN,
	JOYBTNEVENT_PRESS,
	JOYBTNEVENT_COUNT
};

struct JoystickButtonEvent {
	JoyButtonEventType type;
	int button;
	int joystickNumber;
};
struct JoystickAxisEvent {
	int axis;
	float value;
	int joystickNumber;
};

typedef std::map<int, std::function<void (JoystickButtonEvent&)>> buttonbindingmap_t;
typedef std::map<int, std::function<void (JoystickAxisEvent&)>> axisbindingmap_t;

class JoystickInput
{
public:
	JoystickInput(void);
	~JoystickInput(void);

	// detects the useable controllers
	void detect(MMDAgent *agent);
	// Updates the controllers states
	void update();

	void setButtonBinding(int joyNumber, int button, int type, std::function<void (JoystickButtonEvent&)> function);
	void removeButtonBinding(int joyNumber, int button, int type);
	void setAxisBinding(int joyNumber, int axisNumber, std::function<void (JoystickAxisEvent&)> function);
	void removeAxisBinding(int joyNumber, int axisNumber);

private:
	bool enabled[MAX_JOYSTICKS];

	int axesCount[MAX_JOYSTICKS], buttonsCount[MAX_JOYSTICKS];

	float* axes[MAX_JOYSTICKS];
	bool* buttons[MAX_JOYSTICKS];

	buttonbindingmap_t buttonBindings[MAX_JOYSTICKS][JOYBTNEVENT_COUNT];
	axisbindingmap_t axisBindings[MAX_JOYSTICKS];
};

