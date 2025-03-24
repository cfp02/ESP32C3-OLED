#pragma once

#include <Arduino.h>
#include <vector>

// Task frequency types
enum class TaskFrequency {
    DAILY,
    EVERY_2_DAYS,
    EVERY_3_DAYS,
    WEEKLY,
    CUSTOM
};

// Task structure
struct Task {
    String name;
    bool completed;
    TaskFrequency frequency;
    unsigned long lastCompleted;
    unsigned long customInterval;  // For CUSTOM frequency type
};

class TaskManager {
public:
    TaskManager();
    
    // Task management
    bool addTask(const String& name, TaskFrequency frequency, unsigned long customInterval = 0);
    bool removeTask(uint8_t index);
    bool toggleTask(uint8_t index);
    Task* getTask(uint8_t index);
    const Task* getTask(uint8_t index) const;
    uint8_t getTaskCount() const;
    
    // Task status
    bool isTaskCompleted(uint8_t index) const;
    bool isTaskDue(uint8_t index) const;
    unsigned long getLastCompleted(uint8_t index) const;
    
    // Task list navigation
    uint8_t getSelectedTask() const;
    void setSelectedTask(uint8_t index);
    void moveSelection(int8_t direction);
    
private:
    static const uint8_t MAX_TASKS = 10;
    std::vector<Task> tasks;
    uint8_t selectedTask;
}; 