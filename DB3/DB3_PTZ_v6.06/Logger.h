#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

class Logger {
public:
    // Constructor: optional maxLines for ring buffer (default 10, capped at MAX_LINES)
    Logger(uint32_t maxLines = 10) 
        : maxLines_(maxLines > 0 ? (maxLines <= MAX_LINES ? maxLines : MAX_LINES) : 1),
          head_(0), 
          count_(0), 
          isReady_(false), 
          waitTime_(0) {
        // Initialize buffer to empty strings
        for (uint32_t i = 0; i < maxLines_; i++) {
            buffer_[i][0] = '\0';
        }
    }

    // Destructor: no dynamic memory to clean up
    ~Logger() {
    }

    // Initialize the serial port and wait for it to be ready
    void begin() {
        Serial.begin(BAUD_RATE);
        unsigned long startTime = millis();
        while (millis() - startTime < TIMEOUT_MS) {
            delay(10); // Allow interrupts and background tasks
            yield();   // Yield for ESP32 background tasks
            Serial.println("Waiting for serial port...");
        }
        waitTime_ = millis() - startTime;

        char waitMessage[MAX_LINE_LENGTH];
        snprintf(waitMessage, sizeof(waitMessage), "Waited for serial port: %lu ms", waitTime_);
        log(waitMessage);
    }

    // Log a message to serial and ring buffer (legacy method)
    void log(const String& message) {
        char cstr[MAX_LINE_LENGTH];
        // Safely copy String to C-string, ensuring null-termination
        strncpy(cstr, message.c_str(), sizeof(cstr) - 1);
        cstr[sizeof(cstr) - 1] = '\0';
        log(cstr);
    }

    void log(const char* message) {
        char timestamped[MAX_LINE_LENGTH];
        createTimestampedMessage(timestamped, sizeof(timestamped), message);
        addToBuffer(timestamped);
        Serial.println(timestamped);
    }

    // Print methods (mimic Serial.print)
    void print(const String& message) {
        char cstr[MAX_LINE_LENGTH];
        strncpy(cstr, message.c_str(), sizeof(cstr) - 1);
        cstr[sizeof(cstr) - 1] = '\0';
        print(cstr);
    }

    void print(const char* message) {
        char timestamped[MAX_LINE_LENGTH];
        createTimestampedMessage(timestamped, sizeof(timestamped), message);
        addToBuffer(timestamped);
        Serial.print(timestamped);
    }

    void print(int value, int format = DEC) {
        char str[16]; // Sufficient for int in any format (e.g., -2147483648)
        formatNumber(str, sizeof(str), value, format);
        print(str);
    }

    void print(long value, int format = DEC) {
        char str[16]; // Sufficient for long in any format
        formatNumber(str, sizeof(str), value, format);
        print(str);
    }

    void print(float value, int decimals = 2) {
        char str[16]; // Sufficient for float with reasonable decimals
        dtostrf(value, 0, decimals, str);
        print(str);
    }

    // Println methods (mimic Serial.println)
    void println(const String& message) {
        char cstr[MAX_LINE_LENGTH];
        strncpy(cstr, message.c_str(), sizeof(cstr) - 1);
        cstr[sizeof(cstr) - 1] = '\0';
        println(cstr);
    }

    void println(const char* message) {
        char timestamped[MAX_LINE_LENGTH];
        createTimestampedMessage(timestamped, sizeof(timestamped), message);
        addToBuffer(timestamped);
        Serial.println(timestamped);
    }

    void println(int value, int format = DEC) {
        char str[16];
        formatNumber(str, sizeof(str), value, format);
        println(str);
    }

    void println(long value, int format = DEC) {
        char str[16];
        formatNumber(str, sizeof(str), value, format);
        println(str);
    }

    void println(float value, int decimals = 2) {
        char str[16];
        dtostrf(value, 0, decimals, str);
        println(str);
    }

    void println() {
        char timestamped[MAX_LINE_LENGTH];
        createTimestampedMessage(timestamped, sizeof(timestamped), "");
        addToBuffer(timestamped);
        Serial.println(timestamped);
    }

    // Printf method (mimic Serial.printf)
    void printf(const char* format, ...) {
        char buffer[MAX_LINE_LENGTH];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        print(buffer);
    }

    // Get the wait time (in milliseconds) spent waiting for the serial port
    uint32_t getWaitTime() const {
        return waitTime_;
    }

    // Get the ring buffer contents as a String (for HTTP serving)
    String getLogBuffer() const {
        char result[MAX_LINES * (MAX_LINE_LENGTH + 1)]; // +1 for '\n'
        result[0] = '\0';
        if (count_ == 0) {
            return String(result);
        }
        // Start from oldest entry (tail)
        uint32_t tail = (head_ >= count_) ? (head_ - count_) : (maxLines_ + head_ - count_);
        for (uint32_t i = 0; i < count_; i++) {
            uint32_t index = (tail + i) % maxLines_;
            strncat(result, buffer_[index], sizeof(buffer_[index]));
            strncat(result, "\n", 2);
        }
        return String(result);
    }

private:
    static const uint32_t BAUD_RATE = 115200; // Fixed baud rate
    static const uint32_t TIMEOUT_MS = 1000;  // Fixed timeout
    static const uint32_t MAX_LINES = 10;     // Maximum number of lines in buffer
    static const uint32_t MAX_LINE_LENGTH = 128; // Maximum length of a single log message

    const uint32_t maxLines_;                 // Number of lines in ring buffer
    char buffer_[MAX_LINES][MAX_LINE_LENGTH]; // Static ring buffer
    uint32_t head_;                           // Index for next write
    uint32_t count_;                          // Number of entries in buffer
    bool isReady_;                            // Flag for serial port readiness
    uint32_t waitTime_;                       // Time spent waiting for serial

    // Create a timestamped message
    void createTimestampedMessage(char* output, size_t bufferSize, const char* message) const {
        unsigned long ms = millis();
        snprintf(output, bufferSize, "[%lu ms] %s", ms, message);
    }

    // Add a message to the ring buffer
    void addToBuffer(const char* message) {
        strncpy(buffer_[head_], message, sizeof(buffer_[head_]) - 1);
        buffer_[head_][sizeof(buffer_[head_]) - 1] = '\0';
        head_ = (head_ + 1) % maxLines_;
        if (count_ < maxLines_) {
            count_++;
        }
    }

    // Format a number into a string based on the format specifier
    void formatNumber(char* output, size_t size, long value, int format) const {
        switch (format) {
            case BIN:
                formatBinary(output, size, value);
                break;
            case OCT:
                snprintf(output, size, "%lo", value);
                break;
            case HEX:
                snprintf(output, size, "%lX", value);
                break;
            case DEC:
            default:
                snprintf(output, size, "%ld", value);
                break;
        }
    }

    // Format a number in binary (not natively supported by snprintf)
    void formatBinary(char* output, size_t size, long value) const {
        if (size < 2) {
            output[0] = '\0';
            return;
        }
        char temp[33]; // Enough for 32-bit number + null
        uint32_t index = 0;
        unsigned long uvalue = (unsigned long)value;
        if (uvalue == 0) {
            temp[index++] = '0';
        } else {
            while (uvalue > 0 && index < sizeof(temp) - 1) {
                temp[index++] = (uvalue & 1) ? '1' : '0';
                uvalue >>= 1;
            }
        }
        // Reverse the string
        size_t maxLen = size - 1;
        size_t outIndex = 0;
        for (int i = index - 1; i >= 0 && outIndex < maxLen; i--) {
            output[outIndex++] = temp[i];
        }
        output[outIndex] = '\0';
    }
};

#endif // LOGGER_H