#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/iom128.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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
const uint8_t fnd_display[2] = {0x06, 0x5B}; // FND 숫자 1, 2

volatile char input_sequence[5] = ""; // 임시 점/선 저장
volatile uint8_t input_index = 0;     // 현재 입력된 점/선 인덱스
volatile uint8_t reset_flag = OFF;    // 광센서 작동 플래그

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
        PORTC = fnd_display[0];             // FND에 점 표시
        PORTG = (1 << (input_index - 1));   // FND 선택
        _delay_ms(200);
    }
}

// 스위치 2: 선 입력
ISR(INT5_vect) {
    if (input_index < 4) {
        input_sequence[input_index++] = '-'; // 선 추가
        PORTC = fnd_display[1];              // FND에 선 표시
        PORTG = (1 << (input_index - 1));    // FND 선택
        _delay_ms(200);
    }
}

// 광센서: 단어 구분 및 초기화
ISR(INT6_vect) {
    reset_flag = ON; // 초기화 플래그 설정
}

int main(void) {
    // 입출력 설정
    DDRC = 0xFF; // FND 데이터 출력
    DDRG = 0x0F; // FND 선택 출력
    DDRA = 0xFF; // LED 출력
    DDRE = 0xCF; // 스위치 및 광센서 입력

    // 외부 인터럽트 설정
    EICRB = 0x2A; // INT4, INT5: Falling Edge, INT6: Any Change
    EIMSK = 0x70; // INT4, INT5, INT6 Enable
    sei();        // 글로벌 인터럽트 활성화

    char results[10]; // 최종 음계 결과 저장
    uint8_t result_index = 0;

    while (1) {
        if (reset_flag == ON) {
            // 입력 초기화
            input_sequence[input_index] = '\0'; // 입력 문자열 종료
            char note = check_morse(input_sequence); // 입력 모스부호 확인
            if (note != '?') {
                results[result_index++] = note; // 음계 저장
                results[result_index] = '\0';  // 결과 문자열 종료
                PORTA |= (1 << (result_index - 1)); // LED 점등
            }

            // FND, 입력 초기화
            PORTG = 0x00; // FND 비활성화
            PORTC = 0x00; // FND 데이터 초기화
            input_index = 0; // 입력 인덱스 초기화
            reset_flag = OFF; // 초기화 플래그 해제
        }
    }
}
