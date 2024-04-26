#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>

#include "pc/platform.h"

#ifdef _WIN32

#include <io.h>

int orig_stdout_fileno;
FILE* logfile;

void init_logger() {
    if (getenv("MSYSTEM")) return; // we on mingw, we have the terminal available, dont init logfile
    char filepath[1024];
    snprintf(filepath, 1024, "%s/latest.log", sys_user_path());
    logfile = freopen(filepath, "w", stdout);
}

void close_logger() {
    if (getenv("MSYSTEM")) return; // we did nothing on init in mingw, no need to do anything in here
    fclose(logfile);
}

#else // we can print to both the file and stdout on linux

FILE* orig_stdout;
FILE* logfile;
FILE* logger;

ssize_t logger_write(void* cookie, const char* buf, size_t size) {
    fwrite(buf, size, 1, orig_stdout);
    fwrite(buf, size, 1, logfile);
    return size;
}

int logger_close(void* cookie) {
    fclose(logfile);
    return 0;
}

cookie_io_functions_t logger_funcs = {
    .write = logger_write,
    .close = logger_close,
};

void init_logger() {
    char filepath[1024];
    snprintf(filepath, 1024, "%s/latest.log", sys_user_path());
    logfile = fopen(filepath, "w");
    logger = fopencookie(NULL, "w", logger_funcs);
    orig_stdout = stdout;
    stdout = logger;
}

void close_logger() {
    fclose(logger);
    stdout = orig_stdout;
}

#endif

// Crash handler implementation
// Modification of https://github.com/AloUltraExt/sm64ex-alo/blob/master/src/pc/crash/crash_handler.c

#define ARRSIZE(x) (sizeof(x) / sizeof(*(x)))
#define PTR long long unsigned int)(uintptr_t

#ifdef _WIN32
#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include "dbghelp.h"
#else
#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>
#endif
#include <dlfcn.h>

#ifdef _WIN32
struct {
    int code;
    const char* error;
    const char* message;
} error_messages[] = {
    { EXCEPTION_ACCESS_VIOLATION,       "Segmentation Fault",       "The game tried to %s at address 0x%016llX." },
    { EXCEPTION_ARRAY_BOUNDS_EXCEEDED,  "Array Out Of Bounds",      "The game tried to access an element out of the array bounds." },
    { EXCEPTION_DATATYPE_MISALIGNMENT,  "Data Misalignment",        "The game tried to access misaligned data." },
    { EXCEPTION_BREAKPOINT,             "Breakpoint",               "The game reached a breakpoint." },
    { EXCEPTION_FLT_DENORMAL_OPERAND,   "Float Denormal Operand",   "The game tried to perform a floating point operation with a denormal operand." },
    { EXCEPTION_FLT_DIVIDE_BY_ZERO,     "Float Division By Zero",   "The game tried to divide a floating point number by zero." },
    { EXCEPTION_FLT_INEXACT_RESULT,     "Float Inexact Result",     "The game couldn't represent the result of a floating point operation as a decimal fraction." },
    { EXCEPTION_FLT_INVALID_OPERATION,  "Float Invalid Operation",  "The game tried to perform an invalid floating point operation." },
    { EXCEPTION_FLT_OVERFLOW,           "Float Overflow",           "An overflow occurred with a floating point number." },
    { EXCEPTION_FLT_STACK_CHECK,        "Float Stack Overflow",     "The game performed a floating point operation resulting in a stack overflow." },
    { EXCEPTION_FLT_UNDERFLOW,          "Float Underflow",          "An underflow occurred with a floating point number." },
    { EXCEPTION_ILLEGAL_INSTRUCTION,    "Illegal Instruction",      "The game tried to execute an invalid instruction." },
    { EXCEPTION_IN_PAGE_ERROR,          "Page Error",               "The game tried to %s at address 0x%016llX." },
    { EXCEPTION_INT_DIVIDE_BY_ZERO,     "Integer Division By Zero", "The game tried to divide an integer by zero." },
    { EXCEPTION_INT_OVERFLOW,           "Integer Overflow",         "An overflow occurred with an integer." },
    { EXCEPTION_PRIV_INSTRUCTION,       "Instruction Not Allowed",  "The game tried to execute an invalid instruction." },
    { EXCEPTION_STACK_OVERFLOW,         "Stack Overflow",           "The game performed an operation resulting in a stack overflow." },
    { 0,                                "Unknown Exception",        "An unknown exception occurred." },
};
#else
struct {
    int code;
    const char* error;
    const char* message;
} error_messages[] = {
    { SIGBUS,  "Bad Memory Access",    "The game tried to access memory out of bounds." },
    { SIGFPE,  "Floating Point Error", "The game tried to perform illegal arithmetic." },
    { SIGILL,  "Illegal Instruction",  "The game tried to execute an invalid instruction." },
    { SIGSEGV, "Segmentation Fault",   "The game tried to %s at address 0x%016llX." },
    { 0,       "Unknown Exception",    "An unknown exception occured." }
};
#endif

#ifdef _WIN32
static LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception)
#else
static void crash_handler(int signal, siginfo_t* info, ucontext_t* context)
#endif
{
    printf("=== SATURN CRASH REPORT ===\n");
#ifdef _WIN32
    if (exception && exception->ExceptionRecord) {
        PEXCEPTION_RECORD err = exception->ExceptionRecord;
        for (int i = 0; i < ARRSIZE(error_messages); i++) {
            if (error_messages[i].code == err->ExceptionCode || error_messages[i].code == 0) {
                printf("%s - ", error_messages[i].error);
                if (err->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || err->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
                    printf(error_messages[i].message, (err->ExceptionInformation[0] ? "write" : "read"), (PTR)err->ExceptionInformation[1]);
                }
                else printf("%s", error_messages[i].message);
                break;
            }
        }
    }
#else
    if (signal != 0 && info != NULL) {
        for (int i = 0; i < ARRSIZE(error_messages); i++) {
            if (error_messages[i].code == signal || error_messages[i].code == 0) {
                printf("%s - ", error_messages[i].error);
                if (signal == SIGSEGV) {
                    printf(error_messages[i].message, ((context->uc_mcontext.gregs[REG_ERR] & 0x2) != 0 ? "write" : "read"), (PTR)info->si_addr);
                }
                else printf("%s", error_messages[i].message);
                break;
            }
        }
    }
#endif
    else {
        printf("Unable to get error info");
    }
#ifdef _WIN32
    if (exception && exception->ContextRecord)
#else
    if (context->uc_mcontext.gregs[REG_RSP] != 0)
#endif
    {
#ifdef _WIN32
        PCONTEXT reg = exception->ContextRecord;
#else
        greg_t* reg = context->uc_mcontext.gregs;
#endif
        printf(
            "\nRegisters:\n"
            "RSP  0x%016llX   RBP  0x%016llX   RIP  0x%016llX\n"
            "RAX  0x%016llX   RBX  0x%016llX   RCX  0x%016llX\n"
            "RDX  0x%016llX   R08  0x%016llX   R09  0x%016llX\n"
            "R10  0x%016llX   R11  0x%016llX   R12  0x%016llX\n"
            "R13  0x%016llX   R14  0x%016llX   R15  0x%016llX\n"
            "RSI  0x%016llX   RDI  0x%016llX\n",
#ifdef _WIN32
            (PTR)reg->Rsp, (PTR)reg->Rsp, (PTR)reg->Rip,
            (PTR)reg->Rax, (PTR)reg->Rbx, (PTR)reg->Rcx,
            (PTR)reg->Rdx, (PTR)reg->R8,  (PTR)reg->R9,
            (PTR)reg->R10, (PTR)reg->R11, (PTR)reg->R12,
            (PTR)reg->R13, (PTR)reg->R14, (PTR)reg->R15,
            (PTR)reg->Rsi, (PTR)reg->Rdi
#else
            reg[REG_RSP], reg[REG_RBP], reg[REG_RIP],
            reg[REG_RAX], reg[REG_RBX], reg[REG_RCX],
            reg[REG_RDX], reg[REG_R8 ], reg[REG_R9 ],
            reg[REG_R10], reg[REG_R11], reg[REG_R12],
            reg[REG_R13], reg[REG_R14], reg[REG_R15],
            reg[REG_RSI], reg[REG_RDI]
#endif
        );
    }
    else {
        printf("\nUnable to get register info\n");
    }
#ifdef _WIN32
    void* stacktrace[256];
    USHORT num_frames;
    num_frames = CaptureStackBackTrace(0, 256, stacktrace, NULL);
    if (num_frames > 0)
#else
    void *stacktrace[256];
    int num_frames = backtrace(stacktrace, 256);
    if (num_frames > 0)
#endif
    {
        printf("Backtrace:");
        for (int i = 0; i < num_frames; i++) {
            printf("\n#%-3d ", i);
            Dl_info info;
            if (dladdr(stacktrace[i], &info) && info.dli_sname) {
                printf("%s", info.dli_sname);
                if (info.dli_saddr != 0) printf(" + 0x%lX", stacktrace[i] - info.dli_saddr);
            }
            else {
                printf("??? [0x%p]", stacktrace[i]);
            }
        }
        printf("\n");
    }
    else {
        printf("Unable to get stack trace\n");
    }
    close_logger();
    exit(1);
    return;
}

void init_crash_handler() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_handler);
#else
    struct sigaction signal_handler;
    signal_handler.sa_handler = (void*)crash_handler;
    sigemptyset(&signal_handler.sa_mask);
    signal_handler.sa_flags = SA_SIGINFO;
    sigaction(SIGBUS, &signal_handler, NULL);
    sigaction(SIGFPE, &signal_handler, NULL);
    sigaction(SIGILL, &signal_handler, NULL);
    sigaction(SIGSEGV, &signal_handler, NULL);
#endif
}