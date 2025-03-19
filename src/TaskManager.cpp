#include "TaskManager.h"

TaskManager::TaskManager() : selectedTask(0) {
    EEPROM.begin(EEPROM_SIZE);
    loadFromEEPROM();
}

bool TaskManager::addTask(const String& name, TaskFrequency frequency, unsigned long customInterval) {
    if (tasks.size() >= MAX_TASKS) return false;
    
    Task newTask;
    newTask.name = name;
    newTask.completed = false;
    newTask.frequency = frequency;
    newTask.lastCompleted = 0;
    newTask.customInterval = customInterval;
    
    tasks.push_back(newTask);
    saveToEEPROM();
    return true;
}

bool TaskManager::removeTask(uint8_t index) {
    if (index >= tasks.size()) return false;
    
    tasks.erase(tasks.begin() + index);
    if (selectedTask >= tasks.size()) {
        selectedTask = tasks.empty() ? 0 : tasks.size() - 1;
    }
    
    saveToEEPROM();
    return true;
}

bool TaskManager::toggleTask(uint8_t index) {
    if (index >= tasks.size()) {
        Serial.println("Toggle failed: Invalid index");
        return false;
    }
    
    tasks[index].completed = !tasks[index].completed;
    Serial.print("Task ");
    Serial.print(index);
    Serial.print(" toggled to: ");
    Serial.println(tasks[index].completed ? "completed" : "incomplete");
    
    if (tasks[index].completed) {
        tasks[index].lastCompleted = millis();
    }
    
    saveToEEPROM();
    return true;
}

Task* TaskManager::getTask(uint8_t index) {
    if (index >= tasks.size()) return nullptr;
    return &tasks[index];
}

const Task* TaskManager::getTask(uint8_t index) const {
    if (index >= tasks.size()) return nullptr;
    return &tasks[index];
}

uint8_t TaskManager::getTaskCount() const {
    return tasks.size();
}

bool TaskManager::isTaskCompleted(uint8_t index) const {
    if (index >= tasks.size()) return false;
    return tasks[index].completed;
}

bool TaskManager::isTaskDue(uint8_t index) const {
    if (index >= tasks.size()) return false;
    
    const Task& task = tasks[index];
    if (task.completed) return false;
    
    unsigned long currentTime = millis();
    unsigned long timeSinceLastCompletion = currentTime - task.lastCompleted;
    
    switch (task.frequency) {
        case TaskFrequency::DAILY:
            return timeSinceLastCompletion >= 24 * 60 * 60 * 1000UL;
        case TaskFrequency::EVERY_2_DAYS:
            return timeSinceLastCompletion >= 2 * 24 * 60 * 60 * 1000UL;
        case TaskFrequency::EVERY_3_DAYS:
            return timeSinceLastCompletion >= 3 * 24 * 60 * 60 * 1000UL;
        case TaskFrequency::WEEKLY:
            return timeSinceLastCompletion >= 7 * 24 * 60 * 60 * 1000UL;
        case TaskFrequency::CUSTOM:
            return timeSinceLastCompletion >= task.customInterval;
        default:
            return false;
    }
}

unsigned long TaskManager::getLastCompleted(uint8_t index) const {
    if (index >= tasks.size()) return 0;
    return tasks[index].lastCompleted;
}

uint8_t TaskManager::getSelectedTask() const {
    return selectedTask;
}

void TaskManager::setSelectedTask(uint8_t index) {
    if (index < tasks.size()) {
        selectedTask = index;
    }
}

void TaskManager::moveSelection(int8_t direction) {
    if (tasks.empty()) return;
    
    int8_t newIndex = selectedTask + direction;
    if (newIndex < 0) {
        newIndex = tasks.size() - 1;
    } else if (newIndex >= tasks.size()) {
        newIndex = 0;
    }
    
    selectedTask = newIndex;
}

bool TaskManager::saveToEEPROM() {
    // Write number of tasks
    EEPROM.write(0, tasks.size());
    
    // Write each task
    for (uint8_t i = 0; i < tasks.size(); i++) {
        if (!writeTaskToEEPROM(i, calculateTaskAddress(i))) {
            return false;
        }
    }
    
    EEPROM.commit();
    return true;
}

bool TaskManager::loadFromEEPROM() {
    tasks.clear();
    
    // Read number of tasks
    uint8_t taskCount = EEPROM.read(0);
    if (taskCount > MAX_TASKS) return false;
    
    // Read each task
    for (uint8_t i = 0; i < taskCount; i++) {
        if (!readTaskFromEEPROM(i, calculateTaskAddress(i))) {
            return false;
        }
    }
    
    return true;
}

uint16_t TaskManager::calculateTaskAddress(uint8_t index) {
    // First byte is task count, then each task starts at 1 + (index * sizeof(Task))
    return 1 + (index * sizeof(Task));
}

bool TaskManager::writeTaskToEEPROM(uint8_t index, uint16_t address) {
    const Task& task = tasks[index];
    
    // Write task name length
    uint8_t nameLength = task.name.length();
    EEPROM.write(address++, nameLength);
    
    // Write task name
    for (uint8_t i = 0; i < nameLength; i++) {
        EEPROM.write(address++, task.name[i]);
    }
    
    // Write task data
    EEPROM.write(address++, task.completed);
    EEPROM.write(address++, static_cast<uint8_t>(task.frequency));
    
    // Write timestamps
    for (uint8_t i = 0; i < 4; i++) {
        EEPROM.write(address++, (task.lastCompleted >> (i * 8)) & 0xFF);
    }
    for (uint8_t i = 0; i < 4; i++) {
        EEPROM.write(address++, (task.customInterval >> (i * 8)) & 0xFF);
    }
    
    return true;
}

bool TaskManager::readTaskFromEEPROM(uint8_t index, uint16_t address) {
    Task task;
    
    // Read task name length
    uint8_t nameLength = EEPROM.read(address++);
    
    // Read task name
    char nameBuffer[32];  // Maximum name length
    for (uint8_t i = 0; i < nameLength && i < sizeof(nameBuffer) - 1; i++) {
        nameBuffer[i] = EEPROM.read(address++);
    }
    nameBuffer[nameLength] = '\0';
    task.name = String(nameBuffer);
    
    // Read task data
    task.completed = EEPROM.read(address++);
    task.frequency = static_cast<TaskFrequency>(EEPROM.read(address++));
    
    // Read timestamps
    task.lastCompleted = 0;
    for (uint8_t i = 0; i < 4; i++) {
        task.lastCompleted |= (static_cast<unsigned long>(EEPROM.read(address++)) << (i * 8));
    }
    
    task.customInterval = 0;
    for (uint8_t i = 0; i < 4; i++) {
        task.customInterval |= (static_cast<unsigned long>(EEPROM.read(address++)) << (i * 8));
    }
    
    tasks.push_back(task);
    return true;
} 