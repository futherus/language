/* System commands of processor */

/** Registers for arguments
 *  NOT WRITABLE
 */
////DARGS 

/** Temporary registers
 *  Meant to be used during command execution
 */
////DTMPS

/** Pointer to register with system argument
 *  FOR SYSCMD ONLY
 */
////REG_PTR

/** Register with relative stack size
 *  NOT WRITABLE
 */
////REG_SZ   

/** Register for integer calculation
 *  Meant to be used during command execution
 */
////REG_INT  

/** Instruction pointer of processor
 *  FOR SYSCMD ONLY
 *  Used for jumps and function calls
 */
////IP       

/** \brief Copies src to dst
 *  \param DST [out] Pointer to destination location
 *  \param SRC [in]  Pointer to source location
 */
////COPY(DST, SRC)

/// \brief Prints to output
////PRINT(format, ...) 

/// \brief Scans from input
////SCAN(format, ...)  

/// \brief Behaves like print, but only if #define'd CPU_VERBOSE
////VERBOSE(format, ...)

/// \brief Array of RAM, double
////RAM  

/// \brief Array of VRAM, unsigned char
////VRAM

/** \brief Pushes argument to stack
 *  \param ARG [in]  Arg value
 */
////PUSH(ARG) 

/** \brief Pops to argument from stack
 *  \param ARG [out] Pointer to arg
 */
////POP(ARG) 

DEF_SYSCMD(hal,  15619821718256421904ULL, 0,
{
    return PROCESSING_NOERR;
})

/*  Can get only one-literal arg.
    Used for decomposing arguments.
    Can be used for immensive reading from memory. */
DEF_SYSCMD(push, 12441716782781469529ULL, 1, 
{
    PUSH(*REG_PTR);
})

/*  Can get only one-literal arg.
    Used for decomposing arguments.
    The ONLY command able to write to memory. */
DEF_SYSCMD(pop,  15612228490953510534ULL, 1,
{
    POP(REG_PTR);
})

DEF_SYSCMD(add,  15626593610373248958ULL, 0,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    PUSH(DTMPS[1] + DTMPS[0]);
})

DEF_SYSCMD(sub,  15609385153883524641ULL, 0,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    PUSH(DTMPS[1] - DTMPS[0]);
})

DEF_SYSCMD(mul,  15615019051465342581ULL, 0,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    PUSH(DTMPS[1] * DTMPS[0]);    
})
DEF_SYSCMD(div,  15623727183559070478ULL, 0,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    PUSH(DTMPS[1] / DTMPS[0]);
})

DEF_SYSCMD(jmp,  15618106480116791414ULL, 1,
{
    COPY(&IP, REG_PTR);
})

DEF_SYSCMD(ja,     590637888332048622ULL, 1,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    if(DTMPS[1] > DTMPS[0])
        COPY(&IP, REG_PTR);
})

DEF_SYSCMD(jae,   15618119674256329743ULL, 1,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    if(DTMPS[1] >= DTMPS[0])
        COPY(&IP, REG_PTR);
})

DEF_SYSCMD(je,     590637888332048618ULL, 1,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    if(DTMPS[1] == DTMPS[0])
        COPY(&IP, REG_PTR);
})

DEF_SYSCMD(jne,  15618105380605163062ULL, 1,
{
    POP(&DTMPS[0]);
    POP(&DTMPS[1]);

    if(DTMPS[1] != DTMPS[0])
        COPY(&IP, REG_PTR);
})

DEF_SYSCMD(call,  2069086815125772341ULL, 1,
{
    COPY(&DTMPS[0], &REG_SZ);
    COPY(&DTMPS[1], &IP);
    

    PUSH(DTMPS[0]);  // push reg_sz
    PUSH(DTMPS[1]);  // push ip

    COPY(&IP, REG_PTR);
})

DEF_SYSCMD(ret,  15610251569046365794ULL, 0,
{
    POP(&DTMPS[0]);  // pop ip
    POP(&DTMPS[1]);  // pop reg_sz

    COPY(&REG_SZ, &DTMPS[1]);
    COPY(&IP,     &DTMPS[0]);
})
