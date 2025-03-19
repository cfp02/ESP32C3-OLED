#pragma once

#include "OLEDScreen.h"
#include "TaskManager.h"

class DisplayManager : public OLEDScreen {
public:
    DisplayManager();
    DisplayManager(uint8_t clockPin, uint8_t dataPin);
    
    // Task list display
    void displayTaskList(const TaskManager& taskManager);
    void updateSelection(const TaskManager& taskManager);
    
    // Layout constants
    static const uint8_t TASK_START_Y = 9;      // Moved down 1 pixel (from 8 to 9)
    static const uint8_t TASK_SPACING = 10;     // Space between tasks
    static const uint8_t BULLET_X = 1;          // Bullet points at leftmost edge
    static const uint8_t TEXT_X = 10;           // Increased spacing for text
    static const uint8_t SELECTION_BOX_PADDING = 1;
    
private:
    // Display helpers
    void drawTask(const Task& task, uint8_t yPos, bool isSelected);
    void drawSelectionBox(uint8_t yPos);
    void drawBullet(bool completed);
}; 