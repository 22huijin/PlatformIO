#include <avr/io.h>
#include <avr/iom128.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/eeprom.h> // EEPROM 라이브러리

#define EEPROM_ADDR 0 // EEPROM 시작 주소
#define ON 1
#define OFF 0

// FND에 출력할 점 선 정의 (., _)
unsigned char fnd_display[2] = {0x80, 0x08};
// 모스부호 음계 정의
const char* morse_codes[7] = {
    "-.-.",   // C
    "-..",    // D
    ".",      // E
    "..-.",   // F
    "--.",    // G
    ".-",     // A
    "-..."    // B
};
// FND에 출력할 알파벳 정의 (C, D, E, F, G, A, B)
unsigned char digit[7] = {0x39, 0x3f, 0x79, 0x71, 0x3d, 0x77, 0x7f};
// 모스 부호 음계 주파수 (C, D, E, F, G, A, B)
float freq_table[7] = {1046.502, 1174.659, 1318.510, 1396.913, 1567.982, 1760.0, 1975.533};

char input_sequence[5] = ""; // 임시 점, 선 저장
int input_index = 0;     // 현재 입력된 점, 선 인덱스
int reset_flag = OFF;    // 광센서 작동 플래그
int record_flag = OFF;
int paly_flag = OFF;

// ADC 초기화
void adc_init() {
    // ADMUX 설정 (ADC0 사용)
    ADMUX = 0x00;
    // ADCSRA 설정 (ADC 활성화, 분주비 128)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// ADC 값 읽기
unsigned int read_adc() {
    // ADSC = 1 : 변환 시작
    ADCSRA |= (1 << ADSC);
    // 변환 종료 대기
    while (ADCSRA & (1 << ADSC));

    // 변환된 값을 반환 (8비트)
    return ADC;
}

float check_special_commands(const char* sequence) {
    if (strcmp(sequence, "....") == 0) {
        record_flag = ON;
        PORTC = 0x40;
        PORTG = 0x0F;
        _delay_ms(1000);
        return -1;
    } else if (strcmp(sequence, "----") == 0) {
        paly_flag = ON;
        record_flag = OFF;
        PORTC = 0x49;
        PORTG = 0x0F;
        _delay_ms(1000);
        return -1;
    }
    return 0;
}

// 입력된 모스부호를 음계로 변환
float check_morse(const char* sequence, int index) {
    if (index == 0 && check_special_commands(sequence) != 0) {
        return -1;
    }
    for (int i = 0; i < 7; i++) {
        if (strcmp(sequence, morse_codes[i]) == 0) {
            PORTC = digit[i];
            PORTG = 1;
            _delay_ms(1000);
            return freq_table[i]; // 'C'부터 시작
        }
    }
    error();
    return -1; // 알 수 없는 입력
}

// 스위치 1: 점 입력
ISR(INT4_vect) {
    if (input_index < 4) {
        input_sequence[input_index++] = '.'; // 점 추가
        _delay_ms(100); // 안정성을 위해 잠시 지연
    }
}

// 스위치 2: 선 입력
ISR(INT5_vect) {
    if (input_index < 4) {
        input_sequence[input_index++] = '-'; // 선 추가
        _delay_ms(100); // 안정성을 위해 잠시 지연
    }
}

void update_fnd_display() {
    for (int i = 0; i < 4; i++) {
        // FND에 점 또는 선을 출력
        if (input_sequence[i] == '.') {
            PORTC = fnd_display[0]; // 점 표시
        } else if (input_sequence[i] == '-') {
            PORTC = fnd_display[1]; // 선 표시
        } else {
            PORTC = 0x00; // 비활성화
        }
        PORTG = (1 << (3 - i)); // FND 선택 (순차적으로)
        _delay_ms(1);// 안정성을 위한 지연
    }
}

void custom_delay_us(int us) {
    for(int i = 0; i < us; i++) {
        _delay_us(1);
    }
}

void play_buzzer(float hz[8]) {
    for(int i = 0 ; i < 8 ; i++){
        int us = (int)(500000 / hz[i]);
        int count = (int)(hz[i] / 2);

        for(int j = 0; j < count; j++) {
            PORTB |= (1 << PB4);
            custom_delay_us(us);
            PORTB &= ~(1 << PB4);
            custom_delay_us(us);
        }
    }
}

void error() {
    PORTC = 0xFF;   // 모든 FND 켜기
    PORTG = 0x0F;   // 모든 FND 선택

    float hz = 1000.0;
    int us = (int)(500000 / hz);
    int count = (int)(hz / 2);

    for(int i = 0; i < count; i++) {
        PORTB |= (1 << PB4);
        custom_delay_us(us);
        PORTB &= ~(1 << PB4);
        custom_delay_us(us);
    }
    _delay_ms(200);
}

// 데이터 저장 메서드
void save_data_to_eeprom(float* data) {
    for (int i = 0; i < 8; i++) {
        eeprom_update_float((float*)(EEPROM_ADDR + i * sizeof(float)), data[i]);
    }
}

// 데이터 불러오기 메서드
void load_data_from_eeprom(float* data) {
    for (int i = 0; i < 8; i++) {
        data[i] = eeprom_read_float((float*)(EEPROM_ADDR + i * sizeof(float)));
        if (isnan(data[i])){
            return;
        }
    }
}

int main(void) {
    // 입출력 설정
    DDRC = 0xFF; // FND 데이터 출력
    DDRG = 0x0F; // FND 선택 출력
    DDRA = 0xFF; // LED 출력
    DDRE = 0xCF; // 스위치 입력
    DDRB |= (1 << PB4); // 부저 선택

    // ADC 초기화
    adc_init();

    // 외부 인터럽트 설정
    EICRB = 0x2A; // INT4, INT5: Falling Edge
    EIMSK = 0x30; // INT4, INT5 Enable
    sei();        // 글로벌 인터럽트 활성화

    float results[8]; // 최종 음계 결과 저장
    int result_index = 0;

    while (1) {
        unsigned int adc_value = read_adc(); // 광센서 값 읽기

        // 광센서 값이 임계값을 넘으면 단어 구분
        if (adc_value < 600) {
            reset_flag = ON; // 초기화 플래그 설정
        }

        // reset_flag가 설정되었으면 입력 초기화
        if (reset_flag == ON) { //E문제 해결 필요
            input_sequence[input_index] = '\0'; // 입력 종료
            float note = check_morse(input_sequence, result_index); // 입력된 모스부호 확인
            if (note != -1) {
                results[result_index++] = note; // 음계 주파수 결과 저장
                PORTA |= (1 << (result_index - 1)); // LED 점등
            }
            if (paly_flag == ON){
                load_data_from_eeprom(results);
                PORTC = 0x49;
                PORTG = 0x0F;
                result_index = 8;
                paly_flag = OFF;
            }

            // FND, 입력 초기화
            for (int i = 0; i < 4; i++) {
                input_sequence[i] = 0;  // FND 값 초기화
            }
            input_index = 0;      // 입력 인덱스 초기화
            reset_flag = OFF;

            if (result_index==8){
                if(record_flag == ON){
                    record_flag = OFF;
                    save_data_to_eeprom(results); // EEPROM에 저장
                    PORTA = 0x00;
                    PORTC = 0x40;
                    PORTG = 0x0F;
                    result_index=0;
                    _delay_ms(1000);
                    continue;
                }
                play_buzzer(results);
                PORTC = 0xFF;
                PORTG = 0x0F;
                PORTA = 0x00;
                result_index=0;
                _delay_ms(10);
            }
        }

        update_fnd_display();
    }

    return 0;
}