#include "DisplayManager.h"

DisplayManager::DisplayManager() : OLEDScreen() {
    setFont(u8g2_font_6x10_tr);
}

DisplayManager::DisplayManager(uint8_t clockPin, uint8_t dataPin) 
    : OLEDScreen(clockPin, dataPin) {
    setFont(u8g2_font_6x10_tr);
}

void DisplayManager::displayTaskList(const TaskManager& taskManager) {
    clear();
    
    uint8_t yPos = TASK_START_Y;
    uint8_t selectedTask = taskManager.getSelectedTask();
    
    Serial.println("\nDisplaying Task List:");
    for (uint8_t i = 0; i < taskManager.getTaskCount(); i++) {
        const Task* task = taskManager.getTask(i);
        if (task) {
            Serial.print("Task ");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(task->name);
            Serial.print(" (");
            Serial.print(task->completed ? "completed" : "incomplete");
            Serial.println(")");
            
            drawTask(*task, yPos, i == selectedTask);
            yPos += TASK_SPACING;
        }
    }
    
    update();
}

void DisplayManager::updateSelection(const TaskManager& taskManager) {
    displayTaskList(taskManager);
}

void DisplayManager::drawTask(const Task& task, uint8_t yPos, bool isSelected) {
    // Draw selection box if selected
    if (isSelected) {
        drawSelectionBox(yPos);
    }
    
    // Draw bullet point with correct vertical position
    setCursor(BULLET_X, yPos);
    if (task.completed) {
        print("  ");  // Two spaces for completed tasks (same width as asterisk)
    } else {
        print("*");   // Asterisk for incomplete tasks
    }
    
    // Draw task name
    setCursor(TEXT_X, yPos);
    print(task.name.c_str());
}

void DisplayManager::drawSelectionBox(uint8_t yPos) {
    // Draw box around the task
    uint8_t boxY = yPos - 9;  // Keeps the same vertical position
    uint8_t boxHeight = 11;   // Height remains 11 pixels
    uint8_t boxWidth = getScreenWidth() - (2 * SELECTION_BOX_PADDING);
    
    // Moved left 2 more pixels (now total of 4 pixels left from original)
    drawFrame(SELECTION_BOX_PADDING - 2, boxY, boxWidth, boxHeight);
}

// This method is no longer used but kept for reference
void DisplayManager::drawBullet(bool completed) {
    setCursor(BULLET_X, TASK_START_Y);
    if (completed) {
        print(" ");  // Empty space for completed tasks
    } else {
        print("•");  // Bullet point for incomplete tasks
    }
} 