#ifndef DEBUG_H
#define DEBUG_H
#include "sys.h"
#include "usart.h"
#include "const.h"
#include "math.h"


#define USER_INPUT_CHOOSE_SIN_WAVE 1
#define USER_INPUT_CHOOSE_SQUARE_WAVE 2
#define USER_INPUT_CHOOSE_EXIT_DEBUG 3
#define USER_INPUT_INVALID 0

void Greeting(void);
//void DebugDataProcessor(IN OUT u8 isdisplayfft, IN OUT u8 isSendDebuginfo, OUT u32 adcx[NPT], IN OUT u8 *debugStatus, IN OUT u32 magout[NPT]);


//下列函数应该在内部调用 不应该出现在DebugDataProcessor 之外的任何地方
void send_menu(void);
u8 debug_message_processor(void);
void init_sin_buf_array(IN u16 frequency, OUT u32 magout[NPT]);
void init_square_buf_array(IN u16 duty,IN u16 frequency, OUT u32 magout[NPT]);
void replace_buf_array(IN u32 magout[NPT], OUT u32 adcx[NPT]);

#endif // !DEBUG_H
