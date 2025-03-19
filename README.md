# ESP32-C3 Bathroom Task Tracker

A battery-powered task tracking system using an ESP32-C3 microcontroller, OLED display, and joystick input. This device helps track daily bathroom routines and personal care tasks, with the ability to sync data via Bluetooth Low Energy (BLE).

## Features

### Core Functionality
- Track multiple personal care tasks with completion status
- Visual display of tasks with completion indicators
- Intuitive joystick navigation and selection
- Persistent storage of task data
- BLE connectivity for mobile app synchronization
- Ultra-low power operation for extended battery life

### Task Management
- Support for various task types:
  - Daily tasks (e.g., brushing teeth)
  - Periodic tasks (e.g., shaving every 2-3 days)
  - Custom frequency tasks
- Visual indicators:
  - Bullet point (•) for incomplete tasks
  - Empty space for completed tasks
- Selection box for current task
- Task completion tracking with timestamps

### User Interface
- 90-degree rotated OLED display for optimal vertical space
- Clear visual hierarchy:
  - Task name
  - Completion status
  - Selection indicator
- Smooth scrolling for long task lists
- Immediate visual feedback for user actions

### Input Controls
- Joystick navigation:
  - Up/Down: Navigate through tasks
  - Left/Right or Button: Toggle task completion
- Debounced inputs for reliable operation
- Configurable sensitivity settings

### Power Management
- Ultra-low power operation:
  - Deep sleep mode when inactive
  - Wake on button press (interrupt)
  - Automatic return to sleep after inactivity
- Power optimization features:
  - Display backlight control
  - BLE advertising intervals
  - CPU frequency scaling
  - Peripheral power management

### BLE Connectivity
- BLE server for mobile app connection
- Task status synchronization
- Configuration updates
- Battery status reporting
- Connection state management

## Hardware Requirements

### Core Components
- ESP32-C3 microcontroller
- SSD1306 OLED display (72x40)
- Analog joystick
- Tactile button (optional, for wake-up)
- CR2032 coin cell battery
- Battery holder
- PCB or breadboard for assembly

### Pin Assignments
- GPIO0: Joystick X-axis
- GPIO1: Joystick Y-axis
- GPIO2: Joystick button
- GPIO6: OLED clock
- GPIO5: OLED data
- GPIO8: LED indicator
- GPIO9: Optional wake-up button

## Software Architecture

### Core Modules

#### TaskManager
- Manages task list and states
- Handles task persistence
- Tracks completion timestamps
- Manages task frequencies
- Provides task status updates

#### DisplayManager
- Extends OLEDScreen class
- Handles display layout
- Manages selection indicators
- Controls display updates
- Handles screen rotation

#### InputHandler
- Manages joystick and button inputs
- Provides debounced input handling
- Controls navigation logic
- Manages selection events

#### BLEServer
- Handles BLE connections
- Manages data synchronization
- Controls advertising intervals
- Handles connection states

#### PowerManager
- Controls sleep/wake cycles
- Manages power states
- Handles wake-up interrupts
- Optimizes power usage

### State Management
- Clear state machine for UI
- Persistent storage for tasks
- Atomic operations for updates
- Error recovery mechanisms

### Power States
1. Active Mode
   - Full functionality
   - Display on
   - BLE active
   - Regular update intervals

2. Light Sleep Mode
   - Reduced power consumption
   - Maintains some functionality
   - Quick wake-up time

3. Deep Sleep Mode
   - Minimal power consumption
   - Wake on interrupt only
   - State preservation

## Future Enhancements

### Planned Features
- Mobile app for remote management
- Task statistics and trends
- Custom task scheduling
- Multiple user profiles
- Battery level monitoring
- Task reminders

### Potential Improvements
- Enhanced power optimization
- Additional input methods
- Extended BLE functionality
- More display modes
- Advanced task types

## Development Guidelines

### Code Organization
- Modular design
- Clear interfaces
- Comprehensive error handling
- Efficient resource usage
- Clean code practices

### Testing
- Unit tests for modules
- Integration testing
- Power consumption testing
- BLE connectivity testing
- User interface testing

### Documentation
- Code documentation
- API documentation
- Power management guidelines
- BLE protocol documentation
- User guides

## Power Consumption Optimization

### Active Mode
- Display updates optimized
- BLE advertising intervals
- CPU frequency scaling
- Peripheral power management

### Sleep Mode
- Deep sleep configuration
- Wake-up source configuration
- State preservation
- Timer-based wake-up

### Battery Life Considerations
- CR2032 capacity: ~220mAh
- Target battery life: 6+ months
- Power consumption monitoring
- Battery level tracking

## Getting Started

### Prerequisites
- ESP32-C3 development environment
- Required libraries:
  - U8g2lib
  - ESP32 BLE libraries
  - ESP32 power management libraries

### Installation
1. Clone the repository
2. Install required libraries
3. Configure pin assignments
4. Upload to ESP32-C3

### Configuration
- Adjust display settings
- Configure BLE parameters
- Set power management options
- Customize task list

## Contributing
- Fork the repository
- Create feature branch
- Submit pull request
- Follow coding standards
- Update documentation

## License
MIT License - See LICENSE file for details 