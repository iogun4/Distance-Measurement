#include "ranger.h"

typedef struct {
    Event *callback_event;
    enum {
        IDLE,                   // ranger is idle
        EXPECT_START_EVENT_1,   // expecting the rising edge of the start pulse
        EXPECT_START_EVENT_2,   // expecting the falling edge of the start pulse
        EXPECT_DATA_EVENT_1,    // expecting the rising edge of the data pulse
        EXPECT_DATA_EVENT_2     // expecting the falling edge of the data pulse
    } state;
    bool new_data_ready;

    // This is for part 2:
    uint32_t rising_edge_time;
    uint32_t falling_edge_time;
    uint32_t pulse_width;


} RangerState;

volatile RangerState ranger;

void RangerISR();

// Ranger call:
void RangerInit()
{
    // Initial ranger state in memory
    ranger.callback_event = NULL;
    ranger.new_data_ready = false;
    ranger.pulse_width = 0;
    ranger.rising_edge_time = 0;
    ranger.falling_edge_time = 0;

    // Enable GPIO Port D and Wide Timer 2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER2);

    // Configuring:
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_1);
    GPIOPinConfigure(GPIO_PD1_WT2CCP1);

    // Count both edges as event
    TimerControlEvent(WTIMER2_BASE, TIMER_B, TIMER_EVENT_BOTH_EDGES);

    // Register an ISR to deal with the timeout event
    TimerIntRegister(WTIMER2_BASE, TIMER_B, RangerISR);
    TimerIntEnable(WTIMER2_BASE, TIMER_CAPB_EVENT);
}

void RangerTriggerReading()
{
    // Clean data ready flag and set the ranger state
    ranger.new_data_ready = false;
    ranger.state = EXPECT_START_EVENT_1;

    // It is safe to disable a peripheral during configuration
    TimerDisable(WTIMER2_BASE, TIMER_B);

    // Configure WT2CCP1 for PWM to generate a pulse
    TimerConfigure(WTIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PWM);
    TimerLoadSet(WTIMER2_BASE, TIMER_B, 1000);

    // Set match to 500. This creates a 10us (500 cycle) pulse.
    TimerMatchSet(WTIMER2_BASE, TIMER_B, 500);

    TimerIntClear(WTIMER2_BASE, TIMER_CAPB_EVENT);

    // Enable the timer, and start PWM waveform
    TimerEnable(WTIMER2_BASE, TIMER_B);
}


bool RangerDataReady()
{
    return ranger.new_data_ready;
}


void RangerISR()
{

    TimerIntClear(WTIMER2_BASE, TIMER_CAPB_EVENT);

    switch (ranger.state)
    {
    case EXPECT_START_EVENT_1:
        // Rising edge of start pulse detected
        ranger.state = EXPECT_START_EVENT_2;
        break;

    case EXPECT_START_EVENT_2:

        TimerDisable(WTIMER2_BASE, TIMER_B);
        TimerConfigure(WTIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_TIME);
        TimerLoadSet(WTIMER2_BASE, TIMER_B, 0xFFFFFFFF);
        TimerControlEvent(WTIMER2_BASE, TIMER_B, TIMER_EVENT_BOTH_EDGES);
        TimerEnable(WTIMER2_BASE, TIMER_B);
        ranger.state = EXPECT_DATA_EVENT_1;
        break;

    case EXPECT_DATA_EVENT_1:
        ranger.rising_edge_time = TimerValueGet(WTIMER2_BASE, TIMER_B);
        ranger.state = EXPECT_DATA_EVENT_2;
        break;

    case EXPECT_DATA_EVENT_2:
        ranger.falling_edge_time = TimerValueGet(WTIMER2_BASE, TIMER_B);

        if (ranger.falling_edge_time < ranger.rising_edge_time) {

            // Timer overflow
            ranger.pulse_width = (0xFFFFFFFF - ranger.rising_edge_time) + ranger.falling_edge_time;
        } else {
            // No overflow
            ranger.pulse_width = ranger.falling_edge_time - ranger.rising_edge_time;
        }

        // Set data ready flag
        ranger.new_data_ready = true;

        // Schedule the callback event, if registered
        if (ranger.callback_event) {
            EventSchedule(ranger.callback_event, EventGetCurrentTime());
        }

        ranger.state = IDLE;
        break;

    case IDLE:
        // Should not happen, but good to clear flag
        TimerIntClear(WTIMER2_BASE, TIMER_CAPB_EVENT);
        break;
    }
}


uint32_t RangerGetData()
{
    return ranger.pulse_width;
}


void RangerEventRegister(Event *event)
{
    ranger.callback_event = event;
}
