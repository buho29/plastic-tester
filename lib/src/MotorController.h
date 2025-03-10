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
    typedef std::function<void(MotorController *)> ChangeCallback;

    /**
     * @brief Initializes the motor controller, setting up the limit switch pin and connecting to the stepper motor driver.
     */
    void begin()
    {
        pinMode(endPin, INPUT_PULLUP);
        currentStateSwitch = digitalRead(endPin);
        lastStateSwitch = currentStateSwitch;

        engine.init(1);
        stepper = engine.stepperConnectToPin(stepPin);
        Serial.println("Starting");

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
    void setOnMotorEvent(ChangeCallback callback)
    {
        motorCallBack = callback;
    }

    /**
     * @brief Moves the motor a relative distance in millimeters.
     * @param dist The distance to move relative to the current position, in millimeters.
     */
    void move(float dist)
    {
        int32_t steps = dist * ccwMotor * stepsPerMm;

        if (checkLimit(steps, true))
        {
            callMotion = true;
            stepper->move(steps);
        }
        else
        {

            Serial.printf("Limit dist:%.2fmm pos %.2fmm dire:%d state:%d\n",
                          dist,
                          getPosition(),
                          getDirection(), getState());
        }
    }

    /**
     * @brief Moves the motor to an absolute position in millimeters.
     * @param dist The absolute position to move to, in millimeters.
     */
    void moveTo(float dist)
    {
        int32_t steps = dist * ccwMotor * stepsPerMm;

        if (checkLimit(steps, false))
        {
            callMotion = true;
            stepper->moveTo(steps);
        }
        else
            Serial.println("Limit abs");
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
        callMotion = true;
    }
    /**
     * @brief Gets the current position of the motor in millimeters.
     * @return The current position of the motor, in millimeters.
     */
    float getPosition()
    {
        return (stepper->getCurrentPosition() * 1.0 / stepsPerMm * ccwMotor);
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
        return lastDir * ccwMotor;
    }

    /**
     * @brief Sets the speed and acceleration of the motor in millimeters per second.
     * @param speed The desired speed in millimeters per second.
     * @param acceleration The desired acceleration and deceleration in millimeters per second squared.
     */
    void setSpeedAcceleration(float speed, float acceleration)
    {
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
        stepper->setSpeedInHz(speed * steps_mm);
        stepper->setAcceleration(acceleration * steps_mm);
        stepper->applySpeedAcceleration();

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

    void setHome(float newPos){
        int32_t steps = newPos * stepsPerMm;
        stepper->setCurrentPosition(0);
        homePosition += steps;
        maxTravel += steps;
        Serial.printf("setHome %d %.2f \n",steps,newPos);
    }

    /**
     * @brief Starts jogging the motor until a limit switch is hit.
     */
    void seekLimitSwitch()
    {
        if (ccwMotor > 0)
            stepper->runForward();
        else
            stepper->runBackward();
        lastDir = ccwMotor;
        callMotion = true;
    }

    /**
     * @brief Checks if the current motion is complete.
     * @return True if the motion is complete, false otherwise.
     */
    bool isMotionEnd()
    {
        return !stepper->isRunning() || stepper->isStopping();
    }

        // The current state of the limit switch.
        int currentStateSwitch = HIGH;
        // The last stable state of the limit switch.
        int lastStateSwitch = HIGH;

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
                    if (getDirection() > 0)
                    {
                        emergencyStop();
                        callMotion = false;
                    }

                    limit = LIMIT_RIGHT;
                    call = true;
                }
                else
                    limit = NOT_LIMIT;
            }
        }

        // Dispatch callback after debounce and stop
        if (limit == LIMIT_RIGHT && !isRunning() && call)
        {
            dispach();
            call = false;
        }

        // LEFT limit check
        float pos = stepper->getCurrentPosition() * ccwMotor;

        if (pos <= homePosition - maxTravel)
        {
            if (isRunning() && getDirection() < 0)
            {
                emergencyStop();
                limit = LIMIT_LEFT;
                call = true;
            }
            else if (!isRunning() && call)
            {
                callMotion = false;
                call = false;
                dispach();
            }
            else if (getDirection() > 0)
            {
                limit = NOT_LIMIT;
            }
        }

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
     * @brief The maximum travel distance of the motor in steps.
     */
    int32_t maxTravel = 0;
    /**
     * @brief The home position of the motor in steps.
     */
    int32_t homePosition = 0;
    // Number of steps per millimeter
    uint32_t stepsPerMm;

    bool checkLimit(int32_t posSteps, bool relative)
    {
        int32_t pos = stepper->getCurrentPosition() * ccwMotor;

        if (relative)
            posSteps += pos;

        int32_t delta = posSteps - pos;

        if (delta * ccwMotor > 0 && limit != LIMIT_RIGHT)
        {
            lastDir = ccwMotor;
            return true;
        }
        else if (delta * ccwMotor < 0 && limit != LIMIT_LEFT)
        {
            lastDir = -ccwMotor;
            return true;
        }

        return false;
    }

    /**
     * @brief Dispatches the motor event callback.
     */
    void dispach()
    {
        if (motorCallBack)
            motorCallBack(this);
    }
};

FastAccelStepperEngine MotorController::engine = FastAccelStepperEngine();

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
        motor.moveTo(50);
        // Acciones cuando se activa
    }
    else if (m->getState() == MotorController::LIMIT_RIGHT)
    {
        Serial.println("Limit RIGTH");
        Serial.println(motor.getDirection());
        motor.moveTo(-50);
        Serial.println(motor.getDirection());
    }
    else if (m->getState() == MotorController::MOTION_END)
    {
        Serial.println("motion end");
        motor.moveTo(-50);
    }
}

void setup()
{
    Serial.begin(115200);

    // 200steps/revolution   stepping 1/8 reduction 60:10 screw pitch 2mm
    const float steps_mm = 200 * 8 * 6 / 2;
    motor.setConfigMotor(steps_mm, 2, 4, true);

    motor.setConfigHome(20, 40);
    motor.setCurrentPosition();

    motor.begin();
    motor.setOnMotorEvent(handleStateChange); // Asignar callback
    // motor.seekLimitSwitch();
    motor.moveTo(50);
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