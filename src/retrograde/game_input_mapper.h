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

	InputMapper& getInputMapper() const;

	std::shared_ptr<InputDevice> getDeviceAt(int port) const;
	const Vector<std::shared_ptr<InputDevice>>& getUnassignedDevices() const;
	const Vector<std::shared_ptr<InputDevice>>& getAllDevices() const;
	void assignDevice(std::shared_ptr<InputDevice> device, std::optional<int> port);
	std::optional<int> moveDevice(const std::shared_ptr<InputDevice>& device, int dx, int maxPorts);

	Colour4f getDeviceColour(const InputDevice& device) const;

private:
	constexpr static int numAssignments = 10;

	struct Assignment {
		std::shared_ptr<InputDevice> assignedDevice;
		std::shared_ptr<InputDevice> boundDevice;
		bool present = false;
		int priority = 0;

		bool operator<(const Assignment& other) const;
		void clear();
	};

	RetrogradeEnvironment& retrogradeEnvironment;
	InputMapper& inputMapper;
	const SystemConfig& systemConfig;

	Vector<std::shared_ptr<InputVirtual>> gameInput;
	Vector<Assignment> assignments;
	Vector<std::shared_ptr<InputDevice>> unassignedDevices;
	Vector<std::shared_ptr<InputDevice>> allDevices;
	Vector<bool> allDevicesPresence;

	bool assignmentsChanged = false;
	bool assignmentsFixed = false;

	const MappingConfig* mapping = nullptr;

	void assignJoysticks();
	void assignDevice(std::shared_ptr<InputDevice> device);
	void bindInputJoystick(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> joy);
	void bindInputJoystickWithMapping(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy, const MappingConfig::Mapping& mapping);
	void bindInputKeyboard(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> kb);
	void bindDevices();
	std::optional<int> findAssignment(std::shared_ptr<InputDevice> device, bool tryNew);
	String getControllerType(const String& name);
};
