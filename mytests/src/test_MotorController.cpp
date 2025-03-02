#include <MotorController.h>

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
        motor.moveAbsolute(20);
    }
    else if (m->getState() == MotorController::LIMIT_RIGHT)
    {
        Serial.println("Limit RIGTH");
        motor.moveRelative(-20);
    }
    else if (m->getState() == MotorController::MOTION_END)
    {
        Serial.printf("motion end ldir: %d",motor.getLastDirection());
        if(motor.getLastDirection()> 0) motor.moveAbsolute(10);
        else motor.moveAbsolute(20);
    }
}

void setup()
{
    Serial.begin(115200);

    // 200steps/revolution   stepping 1/8 reduction 60:10 screw pitch 2mm
    const float steps_mm = 200 * 8 * 6 / 2;
    motor.setConfigMotor(steps_mm, 2, 4, true);

    motor.setConfigHome(20, 40);
    motor.setHome(0);
    motor.goHome();

    motor.begin();
    motor.setCallback(handleStateChange); // Asignar callback
    // motor.goToSwitch();
    motor.moveAbsolute(20);
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
}