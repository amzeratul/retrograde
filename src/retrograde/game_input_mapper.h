#pragma once

#include <halley.hpp>

#include "src/config/mapping_config.h"

class LibretroCore;
class MappingConfig;
class InputMapper;
class RetrogradeEnvironment;
class SystemConfig;
class Settings;
using namespace Halley;

class GameInputMapper {
public:

	GameInputMapper(RetrogradeEnvironment& environment, InputMapper& inputMapper, const SystemConfig& systemConfig);

	void update();
	void chooseBestAssignments();
	void setAssignmentsFixed(bool fixed);
	void bindCore(LibretroCore& core);

private:
	struct Assignment {
		std::shared_ptr<InputDevice> assignedDevice;
		std::shared_ptr<InputDevice> boundDevice;
		bool present = false;
		int priority = 0;
		InputType type = InputType::None;

		bool operator<(const Assignment& other) const;
		void clear();
	};

	RetrogradeEnvironment& retrogradeEnvironment;
	InputMapper& inputMapper;
	const SystemConfig& systemConfig;

	Vector<std::shared_ptr<InputVirtual>> gameInput;
	Vector<Assignment> assignments;

	bool assignmentsChanged = false;
	bool assignmentsFixed = false;

	const MappingConfig* mapping = nullptr;

	void assignJoysticks();
	void assignDevice(std::shared_ptr<InputDevice> device, InputType type);
	void bindInputJoystick(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> joy);
	void bindInputJoystickWithMapping(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy, const MappingConfig::Mapping& mapping);
	void bindInputKeyboard(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> kb);
	void bindDevices();
	std::optional<int> findAssignment(std::shared_ptr<InputDevice> device, bool tryNew);
	String getControllerType(const String& name);
};
