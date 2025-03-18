/**
 * @file MotorController.h
 * @brief This class provides an interface for controlling a stepper motor with limit switch detection and callback functionality.
 */
#ifndef MotorController_h
#define MotorController_h

#include <FastAccelStepper.h>
#include <functional>

/**
 * @class MotorController
 * @brief Controls a stepper motor using the FastAccelStepper library.
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
 *   4. Command the motor using moveTo(), move(), goHome(), etc.
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
    typedef std::function<void(MotorController *)> EventCallback;

    /**
     * @brief Initializes the motor controller, setting up the limit switch pin and connecting to the stepper motor driver.
     */
    void begin(bool initEngine = false)
    {
        pinMode(endPin, INPUT_PULLUP);
        currentStateSwitch = digitalRead(endPin);
        lastStateSwitch = currentStateSwitch;

        if(initEngine) engine.init(1);
        stepper = engine.stepperConnectToPin(stepPin);
        Serial.println("motor Starting");

        if (stepper)
        {
            stepper->setDirectionPin(direPin);
            // stepper->setEnablePin(enablePinStepper);
            stepper->setAutoEnable(true);
        }
        else
        {
            Serial.println("Stepper Not initialized!");
            delay(1000);
        }
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
    /**
     * @brief Sets the callback function to be called when a motor event occurs (limit switch activation or motion end).
     * @param callback The callback function to be called.
     */
    void setOnMotorEvent(EventCallback callback)
    {
        eventCallBack = callback;
    }

    /**
     * @brief Moves the motor a relative distance in millimeters.
     * @param dist The distance to move relative to the current position, in millimeters.
     */
    bool move(float dist)
    {
        int32_t steps = dist * stepsPerMm;

        if (isValideDirection(steps, true))
        {
            callMotion = true;
            stepper->move(steps * ccwMotor);
            return true;
        }
        return false;
    }

    /**
     * @brief Moves the motor to an absolute position in millimeters.
     * @param dist The absolute position to move to, in millimeters.
     */
    bool moveTo(float dist)
    {
        int32_t steps = dist * stepsPerMm;

        if (isValideDirection(steps, false))
        {
            callMotion = true;
            stepper->moveTo(steps * ccwMotor);
            return true;
        }
        return false;
    }
    /**
     * @brief Sets the current position of the motor in millimeters.
     * @param position The new current position, in millimeters.
     */
    void setCurrentPosition(float position)
    {
        stepper->setCurrentPosition(position * ccwMotor * stepsPerMm);
    }
    /**
     * @brief Moves the motor to the home position (0 mm).
     */
    void goHome()
    {
        moveTo(0);
    }
    /**
     * @brief Gets the current position of the motor in millimeters.
     * @return The current position of the motor, in millimeters.
     */
    float getPosition()
    {
        return (stepper->getCurrentPosition() / float(stepsPerMm) * ccwMotor);
    }

    /**
     * @brief Checks if the motor is currently running.
     * @return True if the motor is running, false otherwise.
     */
    bool isRunning()
    {
        return stepper->isRunning();
    }

    /**
     * @brief Immediately stops the motor.
     */
    void emergencyStop()
    {

        stepper->forceStop();
        // TODO stepper->forceStopAndNewPosition(homePosition - maxTravel );
    }

    /**
     * @brief Stops the motor smoothly.
     */
    void stop()
    {
        stepper->stopMove();
        callMotion = false;
    }

    /**
     * @brief Gets the current direction of the motor.
     * @return 1 if the motor is moving forward, -1 if moving backward, 0 if stopped.
     */
    int8_t getDirection()
    {
        return lastDir;
    }

    /**
     * @brief Sets the speed and acceleration of the motor in millimeters per second.
     * @param speed The desired speed in millimeters per second.
     * @param acceleration The desired acceleration and deceleration in millimeters per second squared.
     */
    void setSpeedAcceleration(float speed, float acceleration)
    {
        float f = ((speed * speed / (2 * acceleration)) + 0.08);
        brakingDistance = f * stepsPerMm;

        stepper->setSpeedInHz(speed * stepsPerMm);
        stepper->setAcceleration(acceleration * stepsPerMm);
        stepper->applySpeedAcceleration();
    }

    /**
     * @brief Configures the motor parameters such as steps per millimeter, speed, acceleration, and motor direction.
     * @param steps_mm The number of steps per millimeter.
     * @param speed The desired speed in millimeters per second.
     * @param acceleration The desired acceleration and deceleration in millimeters per second squared.
     * @param invert_motor True to invert the motor direction, false otherwise.
     */
    void setConfigMotor(uint32_t steps_mm, float speed, float acceleration, bool invert_motor)
    {
        stepsPerMm = steps_mm;
        setSpeedAcceleration(speed, acceleration);

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

        homePosition = home_mm * stepsPerMm;
        maxTravel = max_travel * stepsPerMm;
    }

    /**
     * @brief Sets the home position of the motor to a new position.
     * @param newPos The new home position in millimeters.
     */
    void setHome(float newPos)
    {
        int32_t steps = newPos * stepsPerMm;
        stepper->setCurrentPosition(0);
        homePosition -= steps;
        Serial.printf("setHome %d %.2f \n", steps, newPos);
    }

    /**
     * @brief Starts jogging the motor until a limit switch is hit.
     */
    void jogging(bool soft_limite = true)
    {
        softLimit = soft_limite;
        if (ccwMotor > 0)
            stepper->runForward();
        else
            stepper->runBackward();
        lastDir = 1;
        callMotion = true;
    }

    /**
     * @brief Checks if the current motion is complete.
     * @return True if the motion is complete, false otherwise.
     */
    bool isMotionEnd()
    {
        return !stepper->isRunning();
    }

    bool softLimit = true;
    /**
     * @brief Checks the state of the limit switch and updates the motor state accordingly.
     */
    void checkLimit()
    {
        static bool call = false;
        static const unsigned long debounceTime = 50;
        // The last time the limit switch state changed.
        static unsigned long lastChange = 0;

        // RIGHT limit check
        int reading = digitalRead(endPin);

        const bool is_run = isRunning();
        const int8_t dir = lastDir; // getDirection();

        // Debounce logic
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
                    if (dir > 0)
                    {
                        emergencyStop();
                        softLimit = true;
                    }

                    limit = LIMIT_RIGHT;
                    call = true;
                }
            }
        }

        // Dispatch callback after check limit and stop
        if (limit != NOT_LIMIT && !is_run && call)
        {
            call = false;
            callMotion = false;
            dispach();
        }

        // LEFT limit check
        const int32_t pos = stepper->getCurrentPosition() * ccwMotor;

        if (pos <= homePosition - maxTravel + brakingDistance)
        {
            if (is_run && dir < 0)
            {
                limit = LIMIT_LEFT;
                call = true;
                callMotion = false;
                stop();
            }
            // RIGHT limit check
        }
        else if (softLimit && pos >= homePosition - brakingDistance - stepsPerMm / 2)
        {
            if (is_run && dir > 0)
            {
                limit = LIMIT_RIGHT;
                call = true;
                callMotion = false;
                stop();
            }
        } /**/

        // Motion end check
        if (callMotion && isMotionEnd())
        {
            limit = MOTION_END;
            callMotion = false;
            dispach();
            limit = NOT_LIMIT;
        }
    }

    /**
     * @brief Gets the current state of the motor controller.
     * @return The current state of the motor controller.
     */
    State getState()
    {
        return limit;
    }

private:
    static FastAccelStepperEngine engine;
    FastAccelStepper *stepper = NULL;

    // Callback function to be called when a motor event occurs.
    EventCallback eventCallBack;
    // The pin connected to the limit switch.
    uint8_t endPin;
    // The pin connected to the step signal of the stepper motor.
    uint8_t stepPin;
    // The pin connected to the direction signal of the stepper motor.
    uint8_t direPin;

    // The current state of the motor controller.
    State limit = NOT_LIMIT;

    // Flag indicating whether a motion is in progress.
    bool callMotion = false;
    // The last direction of the motor.
    int8_t lastDir = 0;
    // Multiplier for inverting the motor direction.
    int8_t ccwMotor = 1;
    // The maximum travel distance of the motor in steps.
    int32_t maxTravel = 0;
    // The home position of the motor in steps.
    int32_t homePosition = 0;
    // Number of steps per millimeter
    int32_t stepsPerMm;
    int32_t brakingDistance;

    // The current state of the limit switch.
    int currentStateSwitch = HIGH;
    // The last stable state of the limit switch.
    int lastStateSwitch = HIGH;

    /**
     * @brief Checks if the motor direction is valid based on the current position and limits.
     * @param posSteps The target position in steps.
     * @param relative True if the target position is relative to the current position, false if absolute.
     * @return True if the direction is valid, false otherwise.
     */
    bool isValideDirection(int32_t posSteps, bool relative)
    {
        int32_t pos = stepper->getCurrentPosition() * ccwMotor;

        if (relative)
            posSteps += pos;

        int32_t delta = posSteps - pos;

        if (delta > 0 && limit != LIMIT_RIGHT)
        {
            lastDir = 1;
            limit = NOT_LIMIT;
            return true;
        }
        else if (delta < 0 && limit != LIMIT_LEFT)
        {
            lastDir = -1;
            limit = NOT_LIMIT;
            return true;
        }

        return false;
    }

    /**
     * @brief Dispatches the motor event callback.
     */
    void dispach()
    {
        if (eventCallBack)
            eventCallBack(this);
    }
};

FastAccelStepperEngine MotorController::engine = FastAccelStepperEngine();

#endif