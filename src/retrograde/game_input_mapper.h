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

	Colour4f getDeviceColour(const InputDevice* device) const;
	String getDeviceName(const InputDevice* device) const;

private:
	constexpr static int numAssignments = 10;

	struct Assignment {
		Vector<std::shared_ptr<InputDevice>> assignedDevices;
		bool present = false;
		int priority = 0;

		bool operator<(const Assignment& other) const;
		void clear();

		bool hasAssignedDevice(const std::shared_ptr<InputDevice>& device) const;
	};

	struct Binding {
		Vector<std::shared_ptr<InputDevice>> boundDevices;
		std::shared_ptr<InputVirtual> gameInput;

		void clear();
	};

	RetrogradeEnvironment& retrogradeEnvironment;
	InputMapper& inputMapper;
	const SystemConfig& systemConfig;

	Vector<Binding> gameInput;
	Vector<Assignment> assignments;
	Vector<std::shared_ptr<InputDevice>> unassignedDevices;
	Vector<std::shared_ptr<InputDevice>> allDevices;
	Vector<bool> allDevicesPresence;
	HashMap<const InputDevice*, std::shared_ptr<InputDevice>> devicePair;

	bool assignmentsChanged = false;
	bool assignmentsFixed = false;

	const MappingConfig* mapping = nullptr;

	void assignJoysticks();
	void assignDevice(std::shared_ptr<InputDevice> device);
	void bindInputJoystick(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> joy);
	void bindInputJoystickWithMapping(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy, const MappingConfig::Mapping& mapping);
	void bindInputKeyboard(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> kb);
	void bindInputMouse(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> mouse);
	void bindDevices();
	std::optional<int> findAssignment(std::shared_ptr<InputDevice> device, bool tryNew);
	String getControllerType(const String& name);
};
