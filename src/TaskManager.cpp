#include "TaskManager.h"

TaskManager::TaskManager() : selectedTask(0) {
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
    return true;
}

bool TaskManager::removeTask(uint8_t index) {
    if (index >= tasks.size()) return false;
    
    tasks.erase(tasks.begin() + index);
    if (selectedTask >= tasks.size()) {
        selectedTask = tasks.empty() ? 0 : tasks.size() - 1;
    }
    
    return true;
}

bool TaskManager::toggleTask(uint8_t index) {
    if (index >= tasks.size()) return false;
    
    tasks[index].completed = !tasks[index].completed;
    if (tasks[index].completed) {
        tasks[index].lastCompleted = millis();
    }
    
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