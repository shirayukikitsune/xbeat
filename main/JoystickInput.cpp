#include "JoystickInput.h"

JoystickInput::JoystickInput(void)
{
}


JoystickInput::~JoystickInput(void)
{
	for (int i = 0; i < MAX_JOYSTICKS; i++) {
		delete[] axes[i];
		delete[] buttons[i];
	}
}


void JoystickInput::detect(MMDAgent *agent) {
	for (int i = 0; i < MAX_JOYSTICKS; i++) {
		bool isPresent = glfwGetJoystickParam(i, GLFW_PRESENT) == GL_TRUE;

		if (isPresent) {
			enabled[i] = true;

			axesCount[i] = glfwGetJoystickParam(i, GLFW_AXES);
			axes[i] = new float[axesCount[i]];

			buttonsCount[i] = glfwGetJoystickParam(i, GLFW_BUTTONS);
			buttons[i] = new bool[buttonsCount[i]];

			memset(axes[i], 0, sizeof(float) * axesCount[i]);
			memset(buttons[i], 0, sizeof(bool) * buttonsCount[i]);

			agent->showLogMessage("Gamepad %i enabled with %i buttons and %i axes\n", i + 1, buttonsCount[i], axesCount[i]);
		}
		else {
			axes[i] = nullptr;
			buttons[i] = nullptr;
			agent->showLogMessage("Gamepad %i disabled\n", i + 1);
		}
	}
}


void JoystickInput::update()
{
	unsigned char *buttonsState;

	for (int i = 0; i < MAX_JOYSTICKS; i++) {
		if (enabled[i] == false) continue;

		buttonsState = new unsigned char[buttonsCount[i]];

		glfwGetJoystickButtons(i, buttonsState, buttonsCount[i]);
		glfwGetJoystickPos(i, axes[i], axesCount[i]);

		for (int button = 0; button < buttonsCount[i]; button++) {
			JoystickButtonEvent evt;
			evt.type = JOYBTNEVENT_COUNT;
			evt.button = button;
			evt.joystickNumber = i;
			if (buttons[i][button] && buttonsState[button] == GLFW_RELEASE) {
				// Throw onButtonUp events
				evt.type = JOYBTNEVENT_UP;
				buttons[i][button] = false;
			}
			else if (!buttons[i][button] && buttonsState[button] == GLFW_PRESS) {
				// Throw onButtonDown events
				evt.type = JOYBTNEVENT_DOWN;
				buttons[i][button] = true;
			}
			/*else if (buttons[i][button] && buttonsState[button] == GLFW_PRESS) {
				// Throw onButtonPress events
				evt.type = JOYBTNEVENT_PRESS;
			}*/
			else continue;

			buttonbindingmap_t::iterator eventIterator = buttonBindings[i][evt.type].find(button);
			if (eventIterator != buttonBindings[i][evt.type].end())
				eventIterator->second(evt);
		}
	}
}

void JoystickInput::setButtonBinding(int joyNumber, int button, int type, std::function<void (JoystickButtonEvent&)> function)
{
	if (function == false) removeButtonBinding(joyNumber, button, type);

	if (type == -1) {
		for (int i = 0; i < JOYBTNEVENT_COUNT; i++) {
			setButtonBinding(joyNumber, button, i, function);
		}
	}
	else {
		if (joyNumber == -1) {
			for (int i = 0; i < MAX_JOYSTICKS; i++) {
				if (enabled[i])
					setButtonBinding(i, button, type, function);
			}
		}
		else if (joyNumber >= 0 && joyNumber < MAX_JOYSTICKS) {
			if (button == -1) {
				for (int b = 0; b < buttonsCount[joyNumber]; b++) {
					setButtonBinding(joyNumber, b, type, function);
				}
			}
			else buttonBindings[joyNumber][type][button] = function;
		}
	}
}

void JoystickInput::removeButtonBinding(int joyNumber, int button, int type)
{
	if (joyNumber == -1) {
		for (int i = 0; i < MAX_JOYSTICKS; i++) {
			buttonBindings[i][type].erase(button);
		}
	}
	else if (joyNumber >= 0 && joyNumber < MAX_JOYSTICKS) {
		buttonBindings[joyNumber][type].erase(button);
	}
}

void JoystickInput::setAxisBinding(int joyNumber, int axisNumber, std::function<void (JoystickAxisEvent&)> function)
{
	if (function == false) removeAxisBinding(joyNumber, axisNumber);

	if (joyNumber == -1) {
		for (int i = 0; i < MAX_JOYSTICKS; i++) {
			axisBindings[i][axisNumber] = function;
		}
	}
	else if (joyNumber >= 0 && joyNumber < MAX_JOYSTICKS) {
		axisBindings[joyNumber][axisNumber] = function;
	}
}

void JoystickInput::removeAxisBinding(int joyNumber, int axisNumber)
{
	if (joyNumber == -1) {
		for (int i = 0; i < MAX_JOYSTICKS; i++) {
			axisBindings[i].erase(axisNumber);
		}
	}
	else if (joyNumber >= 0 && joyNumber < MAX_JOYSTICKS) {
		axisBindings[joyNumber].erase(axisNumber);
	}
}

