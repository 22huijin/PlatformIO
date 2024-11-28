#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/iom128.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#define NULL 0
#define ON 1
#define OFF 0

// 모스부호 음계 정의
const char* morse_codes[7] = {
    "-.-.",   // C: 선 점 선 점
    "-..",    // D: 선 점 점
    ".",      // E: 점
    "..-.",   // F: 점 점 선 점
    "--.",    // G: 선 선 점
    ".-",     // A: 점 선
    "-..."    // B: 선 점 점 점
};

// FND에 출력할 숫자 정의 (점: 1, 선: 2)
const uint8_t fnd_display[2] = {0x80, 0x08}; // FND 숫자 1, 2

volatile char input_sequence[5] = ""; // 임시 점/선 저장
volatile uint8_t input_index = 0;     // 현재 입력된 점/선 인덱스
volatile uint8_t reset_flag = OFF;    // 광센서 작동 플래그
volatile uint8_t fnd_values[4] = {0, 0, 0, 0}; // FND에 표시될 값 저장 (점/선)

// ADC 초기화
void adc_init() {
    // ADMUX 설정 (ADC0 사용)
    ADMUX = 0x00;

    // ADCSRA 설정 (ADC 활성화, 분주비 128)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// ADC 값 읽기
unsigned int read_adc() {
    // 변환 시작
    ADCSRA |= (1 << ADSC);
    // 변환 종료 대기
    while (ADCSRA & (1 << ADSC));

    // 변환된 값을 반환 (8비트)
    return ADC;
}

// 입력된 모스부호를 음계로 변환
char check_morse(const char* sequence) {
    for (uint8_t i = 0; i < 7; i++) {
        if (strcmp(sequence, morse_codes[i]) == 0) {
            return 'C' + i; // 'C'부터 시작
        }
    }
    return '?'; // 알 수 없는 입력
}

// 스위치 1: 점 입력
ISR(INT4_vect) {
    if (input_index < 4) {
        input_sequence[input_index++] = '.'; // 점 추가
        // PORTC = fnd_display[0];             // FND에 점 표시
        // PORTG = (1 << (4 - input_index));   // FND 선택
        _delay_ms(200); // 안정성을 위해 잠시 지연
    }
}

// 스위치 2: 선 입력
ISR(INT5_vect) {
    if (input_index < 4) {
        input_sequence[input_index++] = '-'; // 선 추가
        // PORTC = fnd_display[1];              // FND에 선 표시
        // PORTG = (1 << (4 - input_index));    // FND 선택
        _delay_ms(200); // 안정성을 위해 잠시 지연
    }
}

void update_fnd_display() {
    for (uint8_t i = 0; i < 4; i++) {
        // FND에 점 또는 선을 출력
        if (input_sequence[i] == '.') {
            PORTC = fnd_display[0]; // 점 표시
        } else if (input_sequence[i] == '-') {
            PORTC = fnd_display[1]; // 선 표시
        } else {
            PORTC = 0x00; // 비활성화
        }
        PORTG = (1 << (3 - i)); // FND 선택 (순차적으로)
        _delay_ms(5);            // 안정성을 위한 지연
    }
}

int main(void) {
    // 입출력 설정
    DDRC = 0xFF; // FND 데이터 출력
    DDRG = 0x0F; // FND 선택 출력
    DDRA = 0xFF; // LED 출력
    DDRE = 0xCF; // 스위치 입력

    // ADC 초기화
    adc_init();

    // 외부 인터럽트 설정
    EICRB = 0x2A; // INT4, INT5: Falling Edge
    EIMSK = 0x30; // INT4, INT5 Enable
    sei();        // 글로벌 인터럽트 활성화

    char results[10]; // 최종 음계 결과 저장
    uint8_t result_index = 0;

    while (1) {
        unsigned int adc_value = read_adc(); // 광센서 값 읽기

        // 광센서 값이 임계값을 넘으면 단어 구분 (여기서는 임계값 512 사용)
        if (adc_value < 512) {
            reset_flag = ON; // 초기화 플래그 설정
        }

        // reset_flag가 설정되었으면 입력 초기화
        if (reset_flag == ON) { //E문제 해결 필요
            input_sequence[input_index] = '\0'; // 입력 종료
            char note = check_morse(input_sequence); // 입력된 모스부호 확인
            if (note != '?') {
                results[result_index++] = note; // 음계 결과 저장
                PORTA |= (1 << (result_index - 1)); // LED 점등
            }

            // FND, 입력 초기화
            for (uint8_t i = 0; i < 4; i++) {
                input_sequence[i] = 0;  // FND 값 초기화
            }
            input_index = 0;      // 입력 인덱스 초기화
            reset_flag = OFF;
        }

        // _delay_ms(100); // 안정성을 위한 작은 지연

        update_fnd_display();
    }

    return 0;
}
