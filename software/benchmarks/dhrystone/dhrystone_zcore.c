/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025–2026 Pau Díaz Cuesta
 *
 * Bare-metal Z-Core port of Dhrystone 2.1 (Reinhold P. Weicker).
 * Upstream C sources / rationale: software/linux/userspace/benchmarks/original/
 * (netlib dhry-c; no OSI SPDX grant on the historical distribution).
 * This file's porting and harness code is MIT; retain original/ for provenance.
 */

#include <stdint.h>
#include "uart.h"
#include "printf.h"
#include "timer.h"


typedef enum { Ident_1, Ident_2, Ident_3, Ident_4, Ident_5 } Enumeration;

#define DHRY_STR_LEN 30

typedef int   One_Thirty;
typedef int   One_Fifty;
typedef char  Capital_Letter;
typedef int   Boolean;
typedef char  Str_30[DHRY_STR_LEN + 1];
typedef int   Arr_1_Dim[50];
typedef int   Arr_2_Dim[50][50];

typedef struct record {
    struct record *Ptr_Comp;
    Enumeration    Discr;
    union {
        struct {
            Enumeration Enum_Comp;
            int         Int_Comp;
            char        Str_Comp[DHRY_STR_LEN + 1];
        } var_1;
        struct {
            Enumeration E_Comp_2;
            char        Str_2_Comp[DHRY_STR_LEN + 1];
        } var_2;
        struct {
            char Ch_1_Comp;
            char Ch_2_Comp;
        } var_3;
    } variant;
} Rec_Type, *Rec_Pointer;


static int dhry_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void dhry_strcpy(char *dst, const char *src)
{
    while ((*dst++ = *src++))
        ;
}


static Rec_Pointer  Ptr_Glob, Next_Ptr_Glob;
static int          Int_Glob;
static Boolean      Bool_Glob;
static char         Ch_1_Glob, Ch_2_Glob;
static Arr_1_Dim    Arr_1_Glob;
static Arr_2_Dim    Arr_2_Glob;

static Rec_Type     Record_Glob, Next_Record_Glob;


static Enumeration Func_1(Capital_Letter Ch_1_Par_Val,
                          Capital_Letter Ch_2_Par_Val) __attribute__((noinline));
static Boolean     Func_2(Str_30 Str_1_Par_Ref,
                          Str_30 Str_2_Par_Ref)        __attribute__((noinline));
static Boolean     Func_3(Enumeration Enum_Par_Val)     __attribute__((noinline));

static void Proc_1(Rec_Pointer Ptr_Val_Par)             __attribute__((noinline));
static void Proc_2(One_Fifty *Int_Par_Ref)              __attribute__((noinline));
static void Proc_3(Rec_Pointer *Ptr_Ref_Par)            __attribute__((noinline));
static void Proc_4(void)                                __attribute__((noinline));
static void Proc_5(void)                                __attribute__((noinline));
static void Proc_6(Enumeration Enum_Val_Par,
                   Enumeration *Enum_Ref_Par)           __attribute__((noinline));
static void Proc_7(One_Fifty Int_1_Par_Val,
                   One_Fifty Int_2_Par_Val,
                   One_Fifty *Int_Par_Ref)              __attribute__((noinline));
static void Proc_8(Arr_1_Dim Arr_1_Par_Ref,
                   Arr_2_Dim Arr_2_Par_Ref,
                   int Int_1_Par_Val,
                   int Int_2_Par_Val)                   __attribute__((noinline));


static Enumeration Func_1(Capital_Letter Ch_1_Par_Val,
                          Capital_Letter Ch_2_Par_Val)
{
    Capital_Letter Ch_1_Loc, Ch_2_Loc;
    Ch_1_Loc = Ch_1_Par_Val;
    Ch_2_Loc = Ch_1_Loc;
    if (Ch_2_Loc != Ch_2_Par_Val)
        return Ident_1;
    else {
        Ch_1_Glob = Ch_1_Loc;
        return Ident_2;
    }
}

static Boolean Func_2(Str_30 Str_1_Par_Ref, Str_30 Str_2_Par_Ref)
{
    One_Thirty Int_Loc;
    Capital_Letter Ch_Loc = 'A';

    Int_Loc = 2;
    while (Int_Loc <= 2) {
        if (Func_1(Str_1_Par_Ref[Int_Loc],
                    Str_2_Par_Ref[Int_Loc + 1]) == Ident_1) {
            Ch_Loc = 'A';
            Int_Loc += 1;
        }
    }
    if (Ch_Loc >= 'W' && Ch_Loc < 'Z')
        Int_Loc = 7;
    if (Ch_Loc == 'R')
        return 1;
    else {
        if (dhry_strcmp(Str_1_Par_Ref, Str_2_Par_Ref) > 0) {
            Int_Loc += 7;
            Int_Glob = Int_Loc;
            return 1;
        } else
            return 0;
    }
}

static Boolean Func_3(Enumeration Enum_Par_Val)
{
    Enumeration Enum_Loc;
    Enum_Loc = Enum_Par_Val;
    if (Enum_Loc == Ident_3)
        return 1;
    else
        return 0;
}

static void Proc_1(Rec_Pointer Ptr_Val_Par)
{
    Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;

    *Ptr_Val_Par->Ptr_Comp = *Ptr_Glob;
    Ptr_Val_Par->variant.var_1.Int_Comp = 5;
    Next_Record->variant.var_1.Int_Comp = Ptr_Val_Par->variant.var_1.Int_Comp;
    Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
    Proc_3(&Next_Record->Ptr_Comp);
    if (Next_Record->Discr == Ident_1) {
        Next_Record->variant.var_1.Int_Comp = 6;
        Proc_6(Ptr_Val_Par->variant.var_1.Enum_Comp,
               &Next_Record->variant.var_1.Enum_Comp);
        Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
        Proc_7(Next_Record->variant.var_1.Int_Comp, 10,
               &Next_Record->variant.var_1.Int_Comp);
    } else {
        *Ptr_Val_Par = *Ptr_Val_Par->Ptr_Comp;
    }
}

static void Proc_2(One_Fifty *Int_Par_Ref)
{
    One_Fifty  Int_Loc;
    Enumeration Enum_Loc;

    Int_Loc = *Int_Par_Ref + 10;
    do {
        if (Ch_1_Glob == 'A') {
            Int_Loc -= 1;
            *Int_Par_Ref = Int_Loc - Int_Glob;
            Enum_Loc = Ident_1;
        }
    } while (Enum_Loc != Ident_1);
}

static void Proc_3(Rec_Pointer *Ptr_Ref_Par)
{
    if (Ptr_Glob != 0)
        *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
    Proc_7(10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
}

static void Proc_4(void)
{
    Boolean Bool_Loc;
    Bool_Loc = Ch_1_Glob == 'A';
    Bool_Glob = Bool_Loc | Bool_Glob;
    Ch_2_Glob = 'B';
}

static void Proc_5(void)
{
    Ch_1_Glob = 'A';
    Bool_Glob = 0;
}

static void Proc_6(Enumeration Enum_Val_Par, Enumeration *Enum_Ref_Par)
{
    *Enum_Ref_Par = Enum_Val_Par;
    if (!Func_3(Enum_Val_Par))
        *Enum_Ref_Par = Ident_4;
    switch (Enum_Val_Par) {
        case Ident_1: *Enum_Ref_Par = Ident_1; break;
        case Ident_2:
            if (Int_Glob > 100) *Enum_Ref_Par = Ident_1;
            else                *Enum_Ref_Par = Ident_4;
            break;
        case Ident_3: *Enum_Ref_Par = Ident_2; break;
        case Ident_4: break;
        case Ident_5: *Enum_Ref_Par = Ident_3; break;
    }
}

static void Proc_7(One_Fifty Int_1_Par_Val, One_Fifty Int_2_Par_Val,
                   One_Fifty *Int_Par_Ref)
{
    One_Fifty Int_Loc;
    Int_Loc = Int_1_Par_Val + 2;
    *Int_Par_Ref = Int_2_Par_Val + Int_Loc;
}

static void Proc_8(Arr_1_Dim Arr_1_Par_Ref, Arr_2_Dim Arr_2_Par_Ref,
                   int Int_1_Par_Val, int Int_2_Par_Val)
{
    One_Fifty Int_Loc;

    Int_Loc = Int_1_Par_Val + 5;
    Arr_1_Par_Ref[Int_Loc] = Int_2_Par_Val;
    Arr_1_Par_Ref[Int_Loc + 1] = Arr_1_Par_Ref[Int_Loc];
    Arr_1_Par_Ref[Int_Loc + 30] = Int_Loc;
    for (int Int_Index = Int_Loc; Int_Index <= Int_Loc + 1; ++Int_Index)
        Arr_2_Par_Ref[Int_Loc][Int_Index] = Int_Loc;
    Arr_2_Par_Ref[Int_Loc][Int_Loc - 1] += 1;
    Arr_2_Par_Ref[Int_Loc + 20][Int_Loc] = Arr_1_Par_Ref[Int_Loc];
    Int_Glob = 5;
}


#ifndef DHRY_ITERATIONS
#define DHRY_ITERATIONS 500000
#endif


int main(void)
{
    One_Fifty       Int_1_Loc, Int_2_Loc, Int_3_Loc;
    char            Ch_Index;
    Enumeration     Enum_Loc;
    Str_30          Str_1_Loc, Str_2_Loc;
    int             Run_Index;
    volatile int    Number_Of_Runs = DHRY_ITERATIONS;

    uart_set_baud(27);

    Next_Ptr_Glob = &Next_Record_Glob;
    Ptr_Glob      = &Record_Glob;

    Ptr_Glob->Ptr_Comp                       = Next_Ptr_Glob;
    Ptr_Glob->Discr                          = Ident_1;
    Ptr_Glob->variant.var_1.Enum_Comp        = Ident_3;
    Ptr_Glob->variant.var_1.Int_Comp         = 40;
    dhry_strcpy(Ptr_Glob->variant.var_1.Str_Comp,
                "DHRYSTONE PROGRAM, SOME STRING");

    dhry_strcpy(Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

    uprintf("\r\n");
    uprintf("=============================================================\r\n");
    uprintf(" Dhrystone 2.1 -- Z-Core RV32IM @ %u MHz\r\n",
            (unsigned)(CPU_HZ / 1000000UL));
    uprintf("=============================================================\r\n");
    uprintf(" Iterations: %d\r\n", Number_Of_Runs);
    uprintf(" Running...\r\n");

    uint64_t start_cycles = cycles64();

    for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index) {
        Proc_5();
        Proc_4();
        Int_1_Loc = 2;
        Int_2_Loc = 3;
        dhry_strcpy(Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
        Enum_Loc  = Ident_2;
        Bool_Glob = !Func_2(Str_1_Loc, Str_2_Loc);

        while (Int_1_Loc < Int_2_Loc) {
            Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
            Proc_7(Int_1_Loc, Int_2_Loc, &Int_3_Loc);
            Int_1_Loc += 1;
        }

        Proc_8(Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
        Proc_1(Ptr_Glob);

        for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index) {
            if (Enum_Loc == Func_1(Ch_Index, 'C'))
                Proc_6(Ident_1, &Enum_Loc);
        }

        Int_2_Loc = Int_2_Loc * Int_1_Loc;
        Int_1_Loc = Int_2_Loc / Int_3_Loc;
        Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
        Proc_2(&Int_1_Loc);
    }

    uint64_t end_cycles = cycles64();
    uint64_t elapsed    = end_cycles - start_cycles;

    uint64_t elapsed_us = cycles_to_us(elapsed);
    uint64_t elapsed_ms = cycles_to_ms(elapsed);

    uint32_t dhrystones_per_sec = 0;
    uint32_t dmips_x100         = 0;
    uint32_t dmips_mhz_x100    = 0;
    uint32_t cpu_mhz            = (uint32_t)(CPU_HZ / 1000000UL);

    if (elapsed_us > 0) {
        dhrystones_per_sec = (uint32_t)((uint64_t)Number_Of_Runs * 1000000ULL / elapsed_us);
        dmips_x100         = (uint32_t)((uint64_t)dhrystones_per_sec * 100ULL / 1757ULL);
        if (cpu_mhz > 0)
            dmips_mhz_x100 = dmips_x100 / cpu_mhz;
    }

    uprintf("\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Results\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Iterations         : %d\r\n", Number_Of_Runs);
    uprintf(" Elapsed cycles     : %llu\r\n", (unsigned long long)elapsed);
    uprintf(" Elapsed time       : %llu ms\r\n", (unsigned long long)elapsed_ms);
    uprintf(" Dhrystones/sec     : %u\r\n", dhrystones_per_sec);
    uprintf(" DMIPS              : %u.%02u\r\n",
            dmips_x100 / 100, dmips_x100 % 100);
    uprintf(" DMIPS/MHz          : %u.%02u\r\n",
            dmips_mhz_x100 / 100, dmips_mhz_x100 % 100);
    uprintf("=============================================================\r\n");

    while (1)
        ;
    return 0;
}
