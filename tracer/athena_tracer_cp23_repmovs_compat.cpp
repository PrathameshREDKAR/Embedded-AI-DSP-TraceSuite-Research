#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "pin.H"

#define NUM_DST 2
#define NUM_SRC 4

struct trace_instr
{
    uint64_t ip;
    uint8_t is_branch;
    uint8_t branch_taken;
    uint8_t destination_registers[NUM_DST];
    uint8_t source_registers[NUM_SRC];
    uint64_t destination_memory[NUM_DST];
    uint64_t source_memory[NUM_SRC];
};

static_assert(sizeof(trace_instr) == 64, "trace record must be 64 bytes");
static_assert(offsetof(trace_instr, ip) == 0, "bad ip offset");
static_assert(offsetof(trace_instr, is_branch) == 8, "bad branch offset");
static_assert(offsetof(trace_instr, branch_taken) == 9, "bad taken offset");
static_assert(offsetof(trace_instr, destination_registers) == 10, "bad dst-reg offset");
static_assert(offsetof(trace_instr, source_registers) == 12, "bad src-reg offset");
static_assert(offsetof(trace_instr, destination_memory) == 16, "bad dst-mem offset");
static_assert(offsetof(trace_instr, source_memory) == 32, "bad src-mem offset");

struct static_info
{
    uint64_t ip;
    uint8_t is_branch;
    uint8_t dst[NUM_DST];
    uint8_t src[NUM_SRC];
    UINT32 dropped_dst_regs;
    UINT32 dropped_src_regs;
};

static FILE* trace_file = 0;
static FILE* meta_file = 0;

static BOOL tracing = FALSE;
static trace_instr current;

static UINT64 record_count = 0;
static UINT64 dropped_src_regs = 0;
static UINT64 dropped_dst_regs = 0;
static UINT64 dropped_loads = 0;
static UINT64 dropped_stores = 0;

static BOOL roi_begin_found = FALSE;
static BOOL roi_end_found = FALSE;

static uint8_t NormalizeReg(REG reg)
{
    REG full = REG_FullRegName(reg);
    unsigned value = (unsigned)full;

    if (value == 0 || value > 255)
        return 0;

    return (uint8_t)value;
}

static VOID EnableTracing()
{
    tracing = TRUE;
}

static VOID DisableTracing()
{
    tracing = FALSE;
}

static VOID BeginInstruction(static_info* info)
{
    if (!tracing)
        return;

    memset(&current, 0, sizeof(current));

    current.ip = info->ip;
    current.is_branch = info->is_branch;

    memcpy(current.destination_registers, info->dst, NUM_DST);
    memcpy(current.source_registers, info->src, NUM_SRC);

    dropped_dst_regs += info->dropped_dst_regs;
    dropped_src_regs += info->dropped_src_regs;
}

static VOID SetBranchTaken(BOOL taken)
{
    if (!tracing)
        return;

    current.branch_taken = taken ? 1 : 0;
}

static VOID SetAlwaysTaken()
{
    if (!tracing)
        return;

    current.branch_taken = 1;
}

static VOID RecordRead(ADDRINT addr)
{
    if (!tracing)
        return;

    for (UINT32 i = 0; i < NUM_SRC; ++i)
    {
        if (current.source_memory[i] == 0)
        {
            current.source_memory[i] = (uint64_t)addr;
            return;
        }
    }

    dropped_loads++;
}

static VOID RecordWrite(ADDRINT addr)
{
    if (!tracing)
        return;

    for (UINT32 i = 0; i < NUM_DST; ++i)
    {
        if (current.destination_memory[i] == 0)
        {
            current.destination_memory[i] = (uint64_t)addr;
            return;
        }
    }

    dropped_stores++;
}

static VOID EndInstruction()
{
    if (!tracing)
        return;

    fwrite(&current, sizeof(current), 1, trace_file);
    record_count++;
}

static VOID Instruction(INS ins, VOID*)
{
    static_info* info = new static_info;
    memset(info, 0, sizeof(*info));

    info->ip = (uint64_t)INS_Address(ins);

    BOOL conditional =
        (INS_Category(ins) == XED_CATEGORY_COND_BR);

    BOOL control =
        INS_IsBranch(ins) || INS_IsCall(ins) || INS_IsRet(ins);

    info->is_branch = control ? 1 : 0;

    UINT32 dst_count = 0;
    UINT32 max_w = INS_MaxNumWRegs(ins);

    BOOL rep_movs =
        INS_HasRealRep(ins) &&
        ((INS_Opcode(ins) == XED_ICLASS_MOVSB) ||
         (INS_Opcode(ins) == XED_ICLASS_MOVSW) ||
         (INS_Opcode(ins) == XED_ICLASS_MOVSD) ||
         (INS_Opcode(ins) == XED_ICLASS_MOVSQ));

    for (UINT32 i = 0; i < max_w; ++i)
    {
        REG r = INS_RegW(ins, i);
        REG full = REG_FullRegName(r);

        if (rep_movs &&
            full == REG_FullRegName(LEVEL_BASE::REG_RCX))
            continue;
        uint8_t id = NormalizeReg(r);

        if (id == 0)
            continue;

        BOOL duplicate = FALSE;
        for (UINT32 j = 0; j < dst_count && j < NUM_DST; ++j)
            if (info->dst[j] == id)
                duplicate = TRUE;

        if (duplicate)
            continue;

        if (dst_count < NUM_DST)
            info->dst[dst_count] = id;
        else
            info->dropped_dst_regs++;

        dst_count++;
    }

    UINT32 src_count = 0;
    UINT32 max_r = INS_MaxNumRRegs(ins);

    for (UINT32 i = 0; i < max_r; ++i)
    {
        REG r = INS_RegR(ins, i);
        uint8_t id = NormalizeReg(r);

        if (id == 0)
            continue;

        BOOL duplicate = FALSE;
        for (UINT32 j = 0; j < src_count && j < NUM_SRC; ++j)
            if (info->src[j] == id)
                duplicate = TRUE;

        if (duplicate)
            continue;

        if (src_count < NUM_SRC)
            info->src[src_count] = id;
        else
            info->dropped_src_regs++;

        src_count++;
    }

    INS_InsertCall(
        ins, IPOINT_BEFORE,
        (AFUNPTR)BeginInstruction,
        IARG_PTR, info,
        IARG_END);

    if (control)
    {
        if (conditional)
        {
            INS_InsertCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)SetBranchTaken,
                IARG_BRANCH_TAKEN,
                IARG_END);
        }
        else
        {
            INS_InsertCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)SetAlwaysTaken,
                IARG_END);
        }
    }

    UINT32 memops = INS_MemoryOperandCount(ins);

    for (UINT32 mem = 0; mem < memops; ++mem)
    {
        if (INS_MemoryOperandIsRead(ins, mem))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)RecordRead,
                IARG_MEMORYOP_EA, mem,
                IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, mem))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR)RecordWrite,
                IARG_MEMORYOP_EA, mem,
                IARG_END);
        }
    }

    INS_InsertCall(
        ins, IPOINT_BEFORE,
        (AFUNPTR)EndInstruction,
        IARG_END);
}

static VOID ImageLoad(IMG img, VOID*)
{
    RTN begin = RTN_FindByName(img, "roi_begin");

    if (RTN_Valid(begin))
    {
        roi_begin_found = TRUE;
        RTN_Open(begin);
        RTN_InsertCall(
            begin, IPOINT_AFTER,
            (AFUNPTR)EnableTracing,
            IARG_END);
        RTN_Close(begin);
    }

    RTN end = RTN_FindByName(img, "roi_end");

    if (RTN_Valid(end))
    {
        roi_end_found = TRUE;
        RTN_Open(end);
        RTN_InsertCall(
            end, IPOINT_BEFORE,
            (AFUNPTR)DisableTracing,
            IARG_END);
        RTN_Close(end);
    }
}

static VOID Fini(INT32, VOID*)
{
    fflush(trace_file);
    fclose(trace_file);

    fprintf(meta_file, "record_size=64\n");
    fprintf(meta_file, "records=%llu\n",
            (unsigned long long)record_count);
    fprintf(meta_file, "bytes=%llu\n",
            (unsigned long long)(record_count * 64ULL));
    fprintf(meta_file, "roi_begin_found=%d\n",
            roi_begin_found ? 1 : 0);
    fprintf(meta_file, "roi_end_found=%d\n",
            roi_end_found ? 1 : 0);
    fprintf(meta_file, "dropped_src_regs=%llu\n",
            (unsigned long long)dropped_src_regs);
    fprintf(meta_file, "dropped_dst_regs=%llu\n",
            (unsigned long long)dropped_dst_regs);
    fprintf(meta_file, "dropped_loads=%llu\n",
            (unsigned long long)dropped_loads);
    fprintf(meta_file, "dropped_stores=%llu\n",
            (unsigned long long)dropped_stores);

    fclose(meta_file);
}

int main(int argc, char* argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv))
        return 1;

    trace_file = fopen("athena_trace.bin", "wb");
    if (!trace_file)
        return 2;

    meta_file = fopen("athena_trace.meta", "w");
    if (!meta_file)
        return 3;

    IMG_AddInstrumentFunction(ImageLoad, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
