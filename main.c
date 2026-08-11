#include <stdio.h>
#include <string.h>

#include <pspkernel.h>
#include <pspthreadman.h>
#include <pspiofilemgr.h>

#include <systemctrl.h>

PSP_MODULE_INFO("SpeakerFixGame", PSP_MODULE_KERNEL, 1, 0);
PSP_MAIN_THREAD_ATTR(0);

#define LOG_FILE "ms0:/seplugins/jacksensefix.log"

#define SYSCON_GET_HP_NID     0xE0DDFE18
#define SYSCON_SET_HP_CB_NID  0x672B79E8

typedef signed char (*SysconGetHPFunc)(void);

/*
 * Наш динамически создаваемый callback.
 *
 * 0: xori a0,a0,1
 * 1: lui  v1,HI(state)
 * 2: sb   a0,LO(state)(v1)
 * 3: jr   ra
 * 4: nop
 *
 * Не использует GP, поэтому Sony GP нам не мешает.
 */
static unsigned int hp_callback_code[5]
    __attribute__((aligned(16)));


/* --------------------------------------------------------- */
/* Logging                                                   */
/* --------------------------------------------------------- */

static void log_line(const char *text)
{
    SceUID fd;

    fd = sceIoOpen(
        LOG_FILE,
        PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND,
        0777
    );

    if (fd >= 0) {
        sceIoWrite(fd, text, strlen(text));
        sceIoClose(fd);
    }
}


/* --------------------------------------------------------- */
/* Find Syscon functions                                     */
/* --------------------------------------------------------- */

static void *find_syscon_function(unsigned int nid)
{
    void *func;

    func = (void *)sctrlHENFindFunction(
        "sceSYSCON_Driver",
        "sceSyscon_driver",
        nid
    );

    if (func != NULL)
        return func;

    func = (void *)sctrlHENFindFunction(
        "sceSyscon_Driver",
        "sceSyscon_driver",
        nid
    );

    return func;
}


/* --------------------------------------------------------- */
/* Decode J/JAL target                                       */
/* --------------------------------------------------------- */

static unsigned int decode_jump_target(
    unsigned int pc,
    unsigned int instruction
)
{
    return
        ((pc + 4) & 0xF0000000) |
        ((instruction & 0x03FFFFFF) << 2);
}


/* --------------------------------------------------------- */
/* Discover HP callback table dynamically                    */
/* --------------------------------------------------------- */

static volatile unsigned int *
discover_hp_callback_slot(void *setter_addr)
{
    volatile unsigned int *setter;
    volatile unsigned int *internal;

    unsigned int jal;
    unsigned int internal_addr;

    unsigned int lui_inst;
    unsigned int addiu_inst;

    unsigned int upper;
    signed short lower;

    unsigned int table_base;
    unsigned int callback_type;

    unsigned int slot_addr;

    char buf[256];

    setter =
        (volatile unsigned int *)setter_addr;


    /*
     * sceSysconSetHPConnectCallback:
     *
     * setter + 0x08 = jal internal_handler
     * setter + 0x0C = li a2, 4
     */

    jal = setter[2];


    /*
     * Opcode JAL == 000011b.
     */
    if ((jal >> 26) != 3) {
        log_line(
            "ERROR: setter + 8 is not JAL\n"
        );

        return NULL;
    }


    internal_addr =
        decode_jump_target(
            (unsigned int)setter_addr + 8,
            jal
        );


    sprintf(
        buf,
        "Internal handler = 0x%08X\n",
        internal_addr
    );

    log_line(buf);


    /*
     * Получаем callback type из delay slot:
     *
     * li a2,4 == addiu a2,zero,4
     */
    callback_type =
        setter[3] & 0xFFFF;


    sprintf(
        buf,
        "HP callback type = %u\n",
        callback_type
    );

    log_line(buf);


    /*
     * В исследованной Sony 6.61 internal handler:
     *
     * +0x2C: lui   t1, UPPER
     * +0x34: addiu t0,t1, LOWER
     *
     * Это даёт base таблицы callback-ов.
     */
    internal =
        (volatile unsigned int *)internal_addr;


    lui_inst =
        internal[0x2C / 4];

    addiu_inst =
        internal[0x34 / 4];


    /*
     * Проверяем:
     *
     * lui t1, xxxx
     */
    if ((lui_inst & 0xFFFF0000) != 0x3C090000) {

        sprintf(
            buf,
            "ERROR: unexpected LUI: 0x%08X\n",
            lui_inst
        );

        log_line(buf);

        return NULL;
    }


    /*
     * addiu t0,t1,xxxx
     */
    if ((addiu_inst & 0xFFFF0000) != 0x25280000) {

        sprintf(
            buf,
            "ERROR: unexpected ADDIU: 0x%08X\n",
            addiu_inst
        );

        log_line(buf);

        return NULL;
    }


    upper =
        lui_inst & 0xFFFF;

    lower =
        (signed short)(addiu_inst & 0xFFFF);


    table_base =
        (upper << 16) + lower;


    sprintf(
        buf,
        "Callback table base = 0x%08X\n",
        table_base
    );

    log_line(buf);


    /*
     * Internal handler делает:
     *
     * slot = base + type*12
     * a2   = slot + 0x50
     *
     * callback пишется по a2+0x0C.
     *
     * Итого:
     *
     * base + type*12 + 0x50 + 0x0C
     */
    slot_addr =
        table_base +
        callback_type * 12 +
        0x5C;


    sprintf(
        buf,
        "HP callback slot = 0x%08X\n",
        slot_addr
    );

    log_line(buf);


    return
        (volatile unsigned int *)slot_addr;
}


/* --------------------------------------------------------- */
/* Discover Sony HP-state from callback                      */
/* --------------------------------------------------------- */

static volatile unsigned char *
discover_hp_state(unsigned int callback)
{
    volatile unsigned int *code;

    unsigned int i0;
    unsigned int i1;
    unsigned int i2;

    unsigned int upper;
    signed short lower;

    unsigned int state_addr;

    char buf[256];


    /*
     * Sony callback, который мы уже видели:
     *
     * lui v1,xxxx
     * jr  ra
     * sb  a0,xxxx(v1)
     *
     * Никаких абсолютных адресов здесь не предполагаем.
     */
    code =
        (volatile unsigned int *)callback;

    i0 = code[0];
    i1 = code[1];
    i2 = code[2];


    /*
     * lui v1, xxxx
     */
    if ((i0 & 0xFFFF0000) != 0x3C030000) {

        sprintf(
            buf,
            "ERROR: callback LUI mismatch: 0x%08X\n",
            i0
        );

        log_line(buf);

        return NULL;
    }


    /*
     * jr ra
     */
    if (i1 != 0x03E00008) {

        sprintf(
            buf,
            "ERROR: callback JR mismatch: 0x%08X\n",
            i1
        );

        log_line(buf);

        return NULL;
    }


    /*
     * sb a0,offset(v1)
     *
     * opcode SB = 0x28
     * base = v1 (3)
     * rt   = a0 (4)
     */
    if ((i2 & 0xFFFF0000) != 0xA0640000) {

        sprintf(
            buf,
            "ERROR: callback SB mismatch: 0x%08X\n",
            i2
        );

        log_line(buf);

        return NULL;
    }


    upper =
        i0 & 0xFFFF;

    lower =
        (signed short)(i2 & 0xFFFF);


    state_addr =
        (upper << 16) + lower;


    sprintf(
        buf,
        "HP state address = 0x%08X\n",
        state_addr
    );

    log_line(buf);


    return
        (volatile unsigned char *)state_addr;
}


/* --------------------------------------------------------- */
/* Generate our GP-independent callback                      */
/* --------------------------------------------------------- */

static void build_callback(
    volatile unsigned char *state_ptr
)
{
    unsigned int state;
    unsigned int hi;
    unsigned int lo;


    state =
        (unsigned int)state_ptr;

    hi =
        (state >> 16) & 0xFFFF;

    lo =
        state & 0xFFFF;


    /*
     * xori a0,a0,1
     */
    hp_callback_code[0] =
        0x38840001;


    /*
     * lui v1,HI(state)
     */
    hp_callback_code[1] =
        0x3C030000 | hi;


    /*
     * sb a0,LO(state)(v1)
     */
    hp_callback_code[2] =
        0xA0640000 | lo;


    /*
     * jr ra
     */
    hp_callback_code[3] =
        0x03E00008;


    /*
     * nop
     */
    hp_callback_code[4] =
        0x00000000;


    /*
     * Мы только что создали исполняемый код.
     */
    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();
}


/* --------------------------------------------------------- */
/* Synchronize state                                         */
/* --------------------------------------------------------- */

static int sync_state(
    SysconGetHPFunc get_hp,
    volatile unsigned char *state_ptr
)
{
    int raw;
    int expected;


    raw =
        (int)get_hp();


    if (raw != 0 && raw != 1)
        return -1;


    /*
     * На неисправной PSP значение Syscon наоборот.
     */
    expected =
        raw ^ 1;


    if (*state_ptr ==
        (unsigned char)expected) {

        return 0;
    }


    *state_ptr =
        (unsigned char)expected;

    sceKernelDcacheWritebackAll();

    return 1;
}


/* --------------------------------------------------------- */
/* Main                                                      */
/* --------------------------------------------------------- */

int main(int argc, char *argv[])
{
    void *get_hp_addr;
    void *setter_addr;

    SysconGetHPFunc get_hp;

    volatile unsigned int *callback_slot;
    volatile unsigned char *state_ptr;

    unsigned int sony_callback;
    unsigned int our_callback;

    int sync_result;

    char buf[256];


    sceIoRemove(LOG_FILE);


    log_line(
        "================================\n"
        "SpeakerFix GAME dynamic\n"
        "================================\n"
    );


    /*
     * 1. Находим Syscon в текущем GAME environment.
     */
    get_hp_addr =
        find_syscon_function(
            SYSCON_GET_HP_NID
        );

    setter_addr =
        find_syscon_function(
            SYSCON_SET_HP_CB_NID
        );


    sprintf(
        buf,
        "GetHP  = %p\n"
        "Setter = %p\n",
        get_hp_addr,
        setter_addr
    );

    log_line(buf);


    if (
        get_hp_addr == NULL ||
        setter_addr == NULL
    ) {
        log_line(
            "ERROR: Syscon functions missing\n"
        );

        goto idle;
    }


    get_hp =
        (SysconGetHPFunc)get_hp_addr;


    /*
     * 2. Динамически находим callback slot.
     */
    callback_slot =
        discover_hp_callback_slot(
            setter_addr
        );


    if (callback_slot == NULL) {
        log_line(
            "ERROR: callback slot discovery failed\n"
        );

        goto idle;
    }


    sony_callback =
        *callback_slot;


    sprintf(
        buf,
        "Sony callback = 0x%08X\n",
        sony_callback
    );

    log_line(buf);


    /*
     * Kernel callback должен выглядеть как kernel address.
     */
    if (
        sony_callback < 0x88000000 ||
        sony_callback >= 0x88400000
    ) {
        log_line(
            "ERROR: unexpected Sony callback address\n"
        );

        goto idle;
    }


    /*
     * 3. Из самого Sony callback вычисляем HP-state.
     */
    state_ptr =
        discover_hp_state(
            sony_callback
        );


    if (state_ptr == NULL) {
        log_line(
            "ERROR: HP-state discovery failed\n"
        );

        goto idle;
    }


    /*
     * 4. Создаём свой callback с актуальным
     *    GAME HP-state адресом.
     */
    build_callback(
        state_ptr
    );


    our_callback =
        (unsigned int)hp_callback_code;


    sprintf(
        buf,
        "Our callback = 0x%08X\n",
        our_callback
    );

    log_line(buf);


    /*
     * 5. Исправляем уже существующий state.
     */
    sync_result =
        sync_state(
            get_hp,
            state_ptr
        );


    if (sync_result == 1) {
        log_line(
            "Initial HP state corrected.\n"
        );
    }


    /*
     * 6. Подменяем callback.
     */
    *callback_slot =
        our_callback;

    sceKernelDcacheWritebackAll();


    log_line(
        "GAME callback installed successfully.\n"
    );


    /*
     * 7. Watchdog.
     *
     * Если Sony переинициализирует state/callback,
     * восстанавливаем фикс.
     */
    while (1) {

        /*
         * Если callback slot снова не наш —
         * не трогаем.
         */
        if (*callback_slot == sony_callback) {

            *callback_slot =
                our_callback;

            sceKernelDcacheWritebackAll();

            log_line(
                "GAME callback restored.\n"
            );
        }


        sync_result =
            sync_state(
                get_hp,
                state_ptr
            );


        if (sync_result == 1) {

            log_line(
                "GAME HP state corrected.\n"
            );
        }


        sceKernelDelayThread(
            250000
        );
    }


idle:

    while (1) {

        sceKernelDelayThread(
            1000000
        );
    }


    return 0;
}