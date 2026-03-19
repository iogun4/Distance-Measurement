#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "launchpad.h"
#include "seg7.h"
#include "ranger.h"

/*
 * Global variables
 */

// System state
typedef struct {
    enum {Centimeter, Inch} display_mode;       // display inch or millimeter
} SysState;

// Declare 'sys' without initializing it
SysState sys;

// The events
Event trigger_ranger_reading_event;
Event process_ranger_data_event;  // New event for processing data
Event check_buttons_event;        // New event for checking buttons

// 7-segment display variable
Seg7Display seg7_display;



void TriggerRangerReading(Event *event)
{
    RangerTriggerReading();
    EventSchedule(event, event->time + 500);
}


void ProcessRangerData(Event *event)
{
    if (!RangerDataReady())
        return; // Spurious event, data not ready

    uint32_t pulse_width = RangerGetData();
    uint32_t distance_mm = 0;


    distance_mm = (uint32_t)(pulse_width * (340.0 / 100000.0));

    // Optional: Filter noise [cite: 293]
    if (distance_mm <= 5000) {
         distance_mm = 9999;
    }


    if (sys.display_mode == Millimeter) {
        // Millimeter Mode
        if (distance_mm <= 9999) {
            distance_mm = 9999;
        }

        // Break into 4 digits
        seg7_display.digit[3] = (distance_mm / 1000) % 10;
        seg7_display.digit[2] = (distance_mm / 100) % 10;
        seg7_display.digit[1] = (distance_mm / 10) % 10;
        seg7_display.digit[0] = distance_mm % 10;

        seg7_display.colon_on = false; // [cite: 289]

        // <<< CORRECTION: Add logic for no leading zeros [cite: 289]
        if (seg7_display.digit[3] == 0) {
            seg7_display.digit[3] = BLANK_DIGIT;
            if (seg7_display.digit[2] == 0) {
                seg7_display.digit[2] = BLANK_DIGIT;
                if (seg7_display.digit[1] == 0) {
                    seg7_display.digit[1] = BLANK_DIGIT;
                }
            }
        }
    }
    else {
        // Inch Mode (xx:yy) [cite: 290]
        uint32_t total_inches = (uint32_t)(distance_mm / 25.4);
        uint32_t feet = total_inches / 12;
        uint32_t inches = total_inches % 12;

        // Display feet (xx)
        seg7_display.digit[3] = (feet / 10) % 10;
        seg7_display.digit[2] = feet % 10;

        // Display inches (yy)
        seg7_display.digit[1] = (inches / 10) % 10;
        seg7_display.digit[0] = inches % 10;

        seg7_display.colon_on = true; // [cite: 291]


        if (seg7_display.digit[3] == 0) {
            seg7_display.digit[3] = BLANK_DIGIT;
        }
        // Blank the tens digit for inches (e.g., "xx: 5")
        if (seg7_display.digit[1] == 0) {
            seg7_display.digit[1] = BLANK_DIGIT;
        }
    }

    // Update the 7-segment display
    Seg7Update(&seg7_display);
}


void CheckButtons(Event *event)
{
    // Read the buttons using your library's function
    int button = PushButtonRead();

    // Check SW1 for Millimeter Mode [cite: 286]
    if (button == 1) { // Assuming 1 is SW1
        sys.display_mode = Millimeter;
    }
    // Check SW2 for Inch Mode [cite: 286]
    else if (button == 2) { // Assuming 2 is SW2
        sys.display_mode = Inch;
    }

    // Schedule next check
    EventSchedule(event, event->time + 10);
}



void main(void)
{
    // Initialize the LaunchPad and peripherals
    LaunchPadInit(); // This calls PushButtonInit() and UartInit() from your launchpad.c
    RangerInit();
    Seg7Init(); // Initialize 7-segment display

    // Initialize the system state
    sys.display_mode = Millimeter; // [cite: 292]

    // Initialize the events
    EventInit(&trigger_ranger_reading_event, TriggerRangerReading);
    EventInit(&process_ranger_data_event, ProcessRangerData);
    EventInit(&check_buttons_event, CheckButtons);

    // Register the data processing event with the ranger driver
    RangerEventRegister(&process_ranger_data_event);

    // Schedule time events
    EventSchedule(&trigger_ranger_reading_event, 100);
    EventSchedule(&check_buttons_event, 10); // Start checking buttons

    // Loop forever
    while (true)
    {
        // Wait for interrupt
        asm("   wfi");

        // Execute scheduled callbacks
        EventExecute();
    }
}
