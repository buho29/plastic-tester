/**
 * @file MotorController.h
 * @brief This class provides an interface for controlling a stepper motor with limit switch detection and callback functionality.
 */
#ifndef MotorController_h
#define MotorController_h

#include <ESP_FlexyStepper.h>
#include <functional>

/**
 * @class MotorController
 * @brief Controls a stepper motor using the ESP_FlexyStepper library.
 *
 * This class abstracts the motion control of a stepper motor, providing methods to move 
 * the motor to target positions in millimeters (both absolute and relative) and to handle 
 * limit switches for safety. It also allows consumers to set callbacks that are triggered 
 * on motor events such as reaching a limit or completing a motion.
 *
 * The motor can be configured with steps per millimeter, speed, acceleration/deceleration, 
 * and an inversion flag for proper direction handling. Limit switch debouncing is also implemented 
 * to ensure reliable state detection.
 *
 * Usage:
 *   1. Instantiate the MotorController with the step, direction, and endstop pin numbers.
 *   2. Call begin() to initialize the motor and pins.
 *   3. Optionally assign a callback using setOnMotorEvent() to handle state changes.
 *   4. Command the motor using moveAbsolute(), moveRelative(), goHome(), etc.
 */

class MotorController
{
public:
    /**
     * @brief Constructor for the MotorController class.
     * @param step_pin The pin connected to the step signal of the stepper motor.
     * @param dir_pin The pin connected to the direction signal of the stepper motor.
     * @param end_pin The pin connected to the limit switch.
     */
    MotorController(uint8_t step_pin, uint8_t dir_pin, uint8_t end_pin)
    {
        stepPin = step_pin;
        direPin = dir_pin;
        endPin = end_pin;
    }

    /**
     * @brief Callback function type for motor events.
     * @param MotorController* The motor controller instance that triggered the event.
     */
    typedef std::function<void(MotorController *)> ChangeCallback;

    /**
     * @brief Initializes the motor controller, setting up the limit switch pin and connecting to the stepper motor driver.
     */
    void begin()
    {
        pinMode(endPin, INPUT_PULLUP);
        currentStateSwitch = digitalRead(endPin);
        lastStateSwitch = currentStateSwitch;

        stepper.connectToPins(stepPin, direPin);

        // update
        stepper.startAsService(1);
    }

    /**
     * @brief Enum representing the possible states of the motor controller.
     */
    enum State
    {
        /**
         * @brief The motor is not at a limit.
         */
        NOT_LIMIT,
        /**
         * @brief The motor is at the right limit.
         */
        LIMIT_RIGHT,
        /**
         * @brief The motor is at the left limit.
         */
        LIMIT_LEFT,
        /**
         * @brief The motor has completed its motion.
         */
        MOTION_END
    };
    // Asignar callback
    // se dispara cuando alcanza los limites y cuando se acaba la animation
    /**
     * @brief Sets the callback function to be called when a motor event occurs (limit switch activation or motion end).
     * @param callback The callback function to be called.
     */
    void setOnMotorEvent(ChangeCallback callback)
    {
        motorCallBack = callback;
    }

    /**
     * @brief Moves the motor a relative distance in millimeters.
     * @param dist The distance to move relative to the current position, in millimeters.
     */
    void moveRelative(float dist)
    {
        stepper.setTargetPositionRelativeInMillimeters(dist * ccwMotor);
        callMotion = true;
    }
    /**
     * @brief Moves the motor to an absolute position in millimeters.
     * @param dist The absolute position to move to, in millimeters.
     */
    void moveAbsolute(float dist)
    {
        stepper.setTargetPositionInMillimeters(dist * ccwMotor);
        callMotion = true;
    }
    /**
     * @brief Sets the current position of the motor in millimeters.
     * @param position The new current position, in millimeters.
     */
    void setHome(float position)
    {
        stepper.setCurrentPositionInMillimeters(position * ccwMotor);
    }
    /**
     * @brief Moves the motor to the home position (0 mm).
     */
    void goHome()
    {
        stepper.setTargetPositionInMillimeters(0);
        callMotion = true;
    }
    /**
     * @brief Gets the current position of the motor in millimeters.
     * @return The current position of the motor, in millimeters.
     */
    float getPosition()
    {
        return stepper.getCurrentPositionInMillimeters() * ccwMotor;
    }

    /**
     * @brief Checks if the motor is currently running.
     * @return True if the motor is running, false otherwise.
     */
    bool isRunning()
    {
        return stepper.getDirectionOfMotion() != 0;
    }

    /**
     * @brief Immediately stops the motor.
     */
    void emergencyStop()
    {

        stepper.emergencyStop();
    }

    /**
     * @brief Stops the motor smoothly.
     */
    void stop()
    {
        stepper.setTargetPositionToStop();
        callMotion = false;
    }

    /**
     * @brief Gets the current direction of the motor.
     * @return 1 if the motor is moving forward, -1 if moving backward, 0 if stopped.
     */
    int8_t getDirection()
    {
        return stepper.getDirectionOfMotion() * ccwMotor;
    }

    /**
     * @brief Sets the speed of the motor in millimeters per second.
     * @param value The desired speed in millimeters per second.
     */
    void setSpeed(float value){
        stepper.setSpeedInMillimetersPerSecond(value);
    }

    /**
     * @brief Configures the motor parameters such as steps per millimeter, speed, acceleration, and motor direction.
     * @param steps_mm The number of steps per millimeter.
     * @param speed The desired speed in millimeters per second.
     * @param acc_desc The desired acceleration and deceleration in millimeters per second squared.
     * @param invert_motor True to invert the motor direction, false otherwise.
     */
    void setConfigMotor(float steps_mm, float speed, float acc_desc, bool invert_motor)
    {
        stepper.setStepsPerMillimeter(steps_mm);
        stepper.setSpeedInMillimetersPerSecond(speed);
        stepper.setAccelerationInMillimetersPerSecondPerSecond(acc_desc);
        stepper.setDecelerationInMillimetersPerSecondPerSecond(acc_desc);
        if (invert_motor)
            ccwMotor = -1;
        else
            ccwMotor = 1;
    }

    /**
     * @brief Configures the home position and maximum travel distance of the motor.
     * @param home_mm The home position in millimeters.
     * @param max_travel The maximum travel distance in millimeters.
     */
    void setConfigHome(float home_mm, float max_travel)
    {
        homePosition = home_mm;
        maxTravel = max_travel;
    }

    /**
     * @brief Starts jogging the motor until a limit switch is hit.
     */
    void seekLimitSwitch()
    {
        stepper.startJogging(ccwMotor);
        callMotion = true;
    }

    /**
     * @brief Checks if the current motion is complete.
     * @return True if the motion is complete, false otherwise.
     */
    bool isMotionEnd(){
        return stepper.motionComplete();
    }

    /**
     * @brief Checks the state of the limit switch and updates the motor state accordingly.
     */
    void checkLimit()
    {
        static bool call = false;
        // RIGTH check
        int reading = digitalRead(endPin);

        if (reading != currentStateSwitch)
        {
            lastChange = millis();
            currentStateSwitch = reading;
        }

        if ((millis() - lastChange) > debounceTime)
        {
            if (lastStateSwitch != currentStateSwitch)
            {
                lastStateSwitch = currentStateSwitch;
                if (currentStateSwitch == LOW)
                {
                    if (getDirection() > 0)
                    {
                        stepper.setLimitSwitchActive(
                            stepper.LIMIT_SWITCH_COMBINED_BEGIN_AND_END);
                        callMotion = false;
                    }

                    limit = LIMIT_RIGHT;
                    call = true;
                }
                else
                {
                    limit = NOT_LIMIT;
                }
            }
        }
        // esperar un bucle antes de hacer el callback para q se actualice stepper
        if (limit == LIMIT_RIGHT && !isRunning() && call)
        {
            dispach();
            call = false;
        }
        else if (limit == LIMIT_RIGHT && getDirection() < 0)
            stepper.clearLimitSwitchActive();

        // LEFT check
        float pos = getPosition();
        bool isLimitLeft = pos <= homePosition - maxTravel;

        if (isLimitLeft && getDirection() < 0)
        {
            stepper.setLimitSwitchActive(stepper.LIMIT_SWITCH_COMBINED_BEGIN_AND_END);
            limit = LIMIT_LEFT;
            call = true;
        }
        else if (isLimitLeft && !isRunning() && call)
        {
            callMotion = false;
            dispach();
            call = false;
        }
        else if (isLimitLeft && getDirection() > 0)
        {
            stepper.clearLimitSwitchActive();
            limit = NOT_LIMIT;
        }

        // check animation end
        if (callMotion && stepper.motionComplete())
        {
            limit = MOTION_END;
            callMotion = false;
            dispach();
            limit = NOT_LIMIT;
        }

        if (isRunning())
            lastDir = getDirection();
    }

    /**
     * @brief Gets the current state of the motor controller.
     * @return The current state of the motor controller.
     */
    State getState()
    {
        return limit;
    }

    /**
     * @brief Gets the last direction of the motor.
     * @return The last direction of the motor.
     */
    int8_t getLastDirection()
    {
        return lastDir;
    }

private:
    /**
     * @brief Instance of the ESP_FlexyStepper library for controlling the stepper motor.
     */
    ESP_FlexyStepper stepper;
    /**
     * @brief Callback function to be called when a motor event occurs.
     */
    ChangeCallback motorCallBack;
    /**
     * @brief The pin connected to the limit switch.
     */
    uint8_t endPin;
    /**
     * @brief The pin connected to the step signal of the stepper motor.
     */
    uint8_t stepPin;
    /**
     * @brief The pin connected to the direction signal of the stepper motor.
     */
    uint8_t direPin;
    /**
     * @brief The debounce time for the limit switch in milliseconds.
     */
    unsigned long debounceTime = 50;
    // Último tiempo de cambio
    /**
     * @brief The last time the limit switch state changed.
     */
    unsigned long lastChange = 0;
    // Último estado estable
    /**
     * @brief The last stable state of the limit switch.
     */
    int lastStateSwitch = HIGH;
    /**
     * @brief The current state of the limit switch.
     */
    int currentStateSwitch = HIGH;
    /**
     * @brief The current state of the motor controller.
     */
    State limit = NOT_LIMIT;

    /**
     * @brief Flag indicating whether a motion is in progress.
     */
    bool callMotion = false;
    /**
     * @brief The last direction of the motor.
     */
    int8_t lastDir = 0;

    /**
     * @brief Multiplier for inverting the motor direction.
     */
    int8_t ccwMotor = -1;
    /**
     * @brief The maximum travel distance of the motor in millimeters.
     */
    float maxTravel = 20.0;
    /**
     * @brief The home position of the motor in millimeters.
     */
    float homePosition = 5; 

    /**
     * @brief Dispatches the motor event callback.
     */
    void dispach()
    {
        if (motorCallBack)
                motorCallBack(this);
    }
};

#endif
/*
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;

MotorController motor(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN, ENDSTOP_PIN); // Pin 4 con debounce de 50ms (default)

// Función callback
void handleStateChange(MotorController *m)
{
    if (m->getState() == MotorController::LIMIT_LEFT)
    {
        Serial.println("Limit Left");
        motor.moveAbsolute(50);
        // Acciones cuando se activa
    }
    else if (m->getState() == MotorController::LIMIT_RIGHT)
    {
        Serial.println("Limit RIGTH");
        Serial.println(motor.getDirection());
        motor.moveAbsolute(-50);
        Serial.println(motor.getDirection());
    }
    else if (m->getState() == MotorController::MOTION_END)
    {
        Serial.println("motion end");
        motor.moveAbsolute(-50);
    }
}

void setup()
{
    Serial.begin(115200);

    // 200steps/revolution   stepping 1/8 reduction 60:10 screw pitch 2mm
    const float steps_mm = 200 * 8 * 6 / 2;
    motor.setConfigMotor(steps_mm, 2, 4, true);

    motor.setConfigHome(20, 40);
    motor.setHome();

    motor.begin();
    motor.setOnMotorEvent(handleStateChange); // Asignar callback
    // motor.seekLimitSwitch();
    motor.moveAbsolute(50);
}

void loop()
{
    motor.checkLimit(); // Actualizar en cada iteración del loop
    static uint32_t c = 3000;
    if (millis() - c > 200)
    {
        c = millis();
        Serial.printf("distance %.2fmm %d %d\n", motor.getPosition(), motor.getDirection(), motor.getState());
    }
}*/