#include "stdint.h"
#include "stdbool.h"
#include "HalUart.h"
#include "HalTimer.h"
#include "stdio.h"
#include "stdlib.h"
#include "Kernel.h"
#include "event.h"


static void Hw_init(void);
static void Timer_test(void);
static void Printf_test(void);
static void Kernel_init(void);
static uint32_t shared_value;
static void Test_critical_section(uint32_t p, uint32_t taskId);


void User_task0(void);
void User_task1(void);
void User_task2(void);


void main(void){
    Hw_init();
    Kernel_init();
    
    Timer_test();


}

static void Hw_init(void){

    Hal_interrupt_init();
    Hal_uart_init();
    Hal_timer_init();

}

static void Kernel_init(void){
    uint32_t taskId;

    Kernel_task_init();
    Kernel_event_flag_init();
    Kernel_msgQ_init();
    Kernel_sem_init(1);

    taskId = Kernel_task_create(User_task0);
    if (NOT_ENOUGH_TASK_NUM == taskId){
        putstr("Task0 creation fail\n");
    }

    taskId = Kernel_task_create(User_task1);
    if (NOT_ENOUGH_TASK_NUM == taskId){
        putstr("Task1 creation fail\n");
    }

    taskId = Kernel_task_create(User_task2);
    if (NOT_ENOUGH_TASK_NUM == taskId){
        putstr("Task2 creation fail\n");
    }

    Kernel_start();


}

static void Printf_test(void){

    char* str = "printf pointer test";
    char* nullptr = 0;
    uint32_t i = 5;

    debug_printf("%s\n", "Hello, Printf!");
    debug_printf("output string pointer : %s\n", str);
    debug_printf("%s is null pointer, %u number\n", nullptr, 10);
    debug_printf("%u = 5\n", i);
    debug_printf("dec = %u, hex = %x\n", 0xff, 0xff);
    debug_printf("print zero %u\n", 0);
}

static void Timer_test(void){
    while(true){
        debug_printf("current count : %u\n", Hal_timer_get_1ms_counter());
        delay(1000);
    }
}


static void Test_critical_section(uint32_t p, uint32_t taskId){

    Kernel_lock_sem();

    debug_printf("User Task #%u Send= %u\n", taskId, p);
    shared_value = p;
    Kernel_yield();
    delay(1000);
    debug_printf("User Task #%u Shared Value=%u\n",taskId,shared_value);

    Kernel_unlock_sem();

}

void User_task0(void){
    uint32_t local = 0;
    debug_printf("User Task #0 SP = 0x%x\n", &local);

    uint8_t cmdBuf[16];
    uint32_t cmdBufIdx = 0;
    uint8_t uartch = 0;


    while(true){
        KernelEventFlag_t handle_event = Kernel_wait_events(KernelEventFlag_UartIn | KernelEventFlag_CmdOut);
        switch (handle_event)
        {
        case KernelEventFlag_UartIn:
            Kernel_recv_msg(KernelMsgQ_Task0, &uartch, 1);
            if(uartch == '\r'){
                cmdBuf[cmdBufIdx] = '\0';

                Kernel_send_msg(KernelMsgQ_Task1, &cmdBufIdx, 1);
                Kernel_send_msg(KernelMsgQ_Task1, cmdBuf, cmdBufIdx);
                Kernel_send_events(KernelEventFlag_CmdIn);

                cmdBufIdx = 0;
            } else{
                cmdBuf[cmdBufIdx] = uartch;
                cmdBufIdx++;
                cmdBufIdx %= 16;
            }
            break;

        case KernelEventFlag_CmdOut:
            Test_critical_section(5, 0);
            break;
        }
        Kernel_yield();
    }
}

void User_task1(void){
    uint32_t local = 0;
    debug_printf("User Task #1 SP = 0x%x\n", &local);

    uint8_t cmd[16] = {0};
    uint8_t cmdlen = 0;


    while(true){
        KernelEventFlag_t handle_event = Kernel_wait_events(KernelEventFlag_CmdIn);
        switch (handle_event)
        {
        case KernelEventFlag_CmdIn:
            memclr(cmd,16);
            Kernel_recv_msg(KernelMsgQ_Task1, &cmdlen, 1);
            Kernel_recv_msg(KernelMsgQ_Task1, cmd, cmdlen);
            debug_printf("\nRecv Cmd: %s\n", cmd);
            break;
        }
        Kernel_yield();
    }
}

void User_task2(void){
     uint32_t local = 0;    
     debug_printf("User Task #2 SP = 0x%x\n", &local);

    while(true){
        Test_critical_section(3, 2);
        Kernel_yield();
    }
}