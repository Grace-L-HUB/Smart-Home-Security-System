#ifndef __BUZZER_H
#define __BUZZER_H

void Buzzer_Init(void);
void Buzzer_Control(uint8_t status);
void Buzzer_Beep(uint16_t duration);

#endif