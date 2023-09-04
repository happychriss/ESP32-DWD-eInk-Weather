#include "support.h"
void waitAndPrint(const char* message) {
    Serial.print(message); // Write the message to the serial port
    Serial.println(" ...press key!");

    // Wait for any key press
    while (!Serial.available()) {
        delay(100);
        // Wait until at least one byte is available in the input buffer
    }

    // Read the pressed key
    char keyPressed = Serial.read(); // Read the first available byte


    // Clear the input buffer
    while (Serial.available()) {
        Serial.read(); // Clear any remaining bytes in the input buffer
    }
}

void Serial_printTime(std::tm *time) {
    char timeStr[50];
    strftime(timeStr, sizeof(timeStr), "%c", &*time);
    DP(timeStr);
}

//
// Created by development on 27.08.23.
//
