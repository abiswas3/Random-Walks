/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <unordered_map>
#include "pin.H"

FILE* trace;
static std::unordered_map<ADDRINT, char*> disas;

// This function is called before every instruction is executed
// and prints the IP
VOID printip(VOID* ip) {
  auto it = disas.find((ADDRINT)ip);
  if(it != disas.end()) {
    char* s = (it->second);
    fprintf(trace, "%p %s\n", ip, s);
  }
  else {
    fprintf(trace, "%p ???\n", ip);
  }
}
VOID ImageLoad(IMG img, VOID* v) {
  std::string s = IMG_Name(img);
  ADDRINT start = IMG_LoadOffset(img);
  fprintf(trace, "Loading %s %lx\n", s.c_str(), start);
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID* v)
{
  //printf("%lx\n", INS_Address(ins));
  if(disas.find(INS_Address(ins)) == disas.end()) {
    int l = INS_Disassemble(ins).size();
    char* c = (char*)malloc(l+1);
    memcpy(c, INS_Disassemble(ins).c_str(), l);
    c[l] = 0;
    disas[INS_Address(ins)] = c;
  }
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)printip, IARG_INST_PTR, IARG_END);
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID* v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the IPs of every instruction executed\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    trace = fopen("itrace.out", "w");
    
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
