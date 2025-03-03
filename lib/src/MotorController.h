#ifndef MotorController_h
#define MotorController_h

#include <ESP_FlexyStepper.h>
#include <functional>

class MotorController
{
public:
    // Constructor uint8_t setp_pin, uint8_t dir_pin,uint8_t end_pin
    MotorController(uint8_t step_pin, uint8_t dir_pin, uint8_t end_pin)
    {
        stepPin = step_pin;
        direPin = dir_pin;
        endPin = end_pin;
    }

    
    // Callback para integración con otros componentes
    typedef std::function<void(MotorController *)> ChangeCallback;

    // Inicializar
    void begin()
    {
        pinMode(endPin, INPUT_PULLUP);
        currentStateSwitch = digitalRead(endPin);
        lastStateSwitch = currentStateSwitch;

        stepper.connectToPins(stepPin, direPin);

        // update
        stepper.startAsService(1);
    }

    enum State
    {
        NOT_LIMIT,
        LIMIT_RIGHT,
        LIMIT_LEFT,
        MOTION_END
    };
    // Asignar callback
    // se dispara cuando alcanza los limites y cuando se acaba la animation
    void setOnMotorEvent(ChangeCallback callback)
    {
        motorCallBack = callback;
    }

    // distance to move relative to the current position in millimeters
    void moveRelative(float dist)
    {
        stepper.setTargetPositionRelativeInMillimeters(dist * ccwMotor);
        callMotion = true;
    }
    // absolute position to move to in units of millimeter
    void moveAbsolute(float dist)
    {
        stepper.setTargetPositionInMillimeters(dist * ccwMotor);
        callMotion = true;
    }
    // set the current position of the motor in millimeters.
    void setHome(float position)
    {
        stepper.setCurrentPositionInMillimeters(position * ccwMotor);
    }
    // move to home position 0mm
    void goHome()
    {
        stepper.setTargetPositionInMillimeters(0);
        callMotion = true;
    }
    // get the current position of the motor in millimeters
    float getPosition()
    {
        return stepper.getCurrentPositionInMillimeters() * ccwMotor;
    }

    bool isRunning()
    {
        return stepper.getDirectionOfMotion() != 0;
    }

    void emergencyStop()
    {

        stepper.emergencyStop();
    }

    void stop()
    {
        stepper.setTargetPositionToStop();
        callMotion = false;
    }

    int8_t getDirection()
    {
        return stepper.getDirectionOfMotion() * ccwMotor;
    }

    void setSpeed(float value){
        stepper.setSpeedInMillimetersPerSecond(value);
    }

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

    void setConfigHome(float home_mm, float max_travel)
    {
        homePosition = home_mm;
        maxTravel = max_travel;
    }

    void seekLimitSwitch()
    {
        stepper.startJogging(ccwMotor);
        callMotion = true;
    }

    bool isMotionEnd(){
        return stepper.motionComplete();
    }

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

    // Obtener estado
    State getState()
    {
        return limit;
    }

    int8_t getLastDirection()
    {
        return lastDir;
    }

private:
    ESP_FlexyStepper stepper;
    // Callback para cambios de estado
    ChangeCallback motorCallBack;
    // Pin del switch
    uint8_t endPin;
    // Pin del step
    uint8_t stepPin;
    // Pin del direction
    uint8_t direPin;
    // Tiempo de debounce
    unsigned long debounceTime = 50;
    // Último tiempo de cambio
    unsigned long lastChange = 0;
    // Último estado estable
    int lastStateSwitch = HIGH;
    // Estado actual leído
    int currentStateSwitch = HIGH;
    // Estado actual general
    State limit = NOT_LIMIT;

    bool callMotion = false;
    int8_t lastDir = 0;

    // counterclockwise motor
    int8_t ccwMotor = -1;
    float maxTravel = 20.0;
    float homePosition = 5; 

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