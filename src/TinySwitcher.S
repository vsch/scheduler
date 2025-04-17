.global initContext
.global isInAsyncContext
.global resumeContext
.global yieldContext

.section .bss
    .lcomm pCurrentContext, 2          ; Declare pCurrentContext as a 2-byte pointer
    .lcomm loopSP,2                     ; Declare loopSP as a 2-byte variable

.section .text

; Offsets for AsyncContext structure members
.equ CONTEXT_STACK_USED, 0            ; Offset for stackUsed
.equ CONTEXT_STACK_MAX, 1             ; Offset for stackMax
.equ CONTEXT_STACK_MAX_USED, 2        ; Offset for stackMaxUsed
.equ CONTEXT_PENTRY, 3                ; Offset for pEntry (2 bytes)
.equ CONTEXT_PENTRY_ARG, 5            ; Offset for pEntryArg (2 bytes)
.equ CONTEXT_PSTACK_OFFSET, 7         ; Offset to stack
.equ __SP_L__, 0x3D                   ; SP L io address
.equ __SP_H__, 0x3E                   ; SP H io address

; Function: initContext
; Equivalent C: Initialize the context structure with provided values
initContext:
    ; r24:r25 contains pContext
    ; r22:r23 contains pEntry
    ; r20:r21 contains pEntryArg
    ; r18 contains contextSize which is stackMax + CONTEXT_PSTACK_OFFSET

    movw r30, r24                      ; Move pContext to Z (r31:r30)
    ; lds r30, pCurrentContext         ; Load low byte of pCurrentContext into Z (r31:r30)
    ; lds r31, pCurrentContext+1       ; Load high byte of pCurrentContext into Z

    ; Initialize pEntry
    std Z+CONTEXT_PENTRY, r22          ; Store low byte of pEntry
    std Z+CONTEXT_PENTRY+1, r23        ; Store high byte of pEntry

    ; Initialize pEntryArg
    std Z+CONTEXT_PENTRY_ARG, r20      ; Store low byte of pEntryArg
    std Z+CONTEXT_PENTRY_ARG+1, r21    ; Store high byte of pEntryArg

    ; No longer need to Initialize pStack, it is at the end of the context structure
    ; Initialize stackMax
    subi r18, CONTEXT_PSTACK_OFFSET
    std Z+CONTEXT_STACK_MAX, r18       ; Store stackMax

    ; Initialize stackUsed and stackMaxUsed to 0
    clr r1                             ; Clear r1 (already set to 0 but for clarity)
    std Z+CONTEXT_STACK_USED, r1       ; Initialize stackUsed to 0
    std Z+CONTEXT_STACK_MAX_USED, r1   ; Initialize stackMaxUsed to 0

    ret                                ; Return

; Function: uint8_t isAsyncContext()
; Output: r24 contains the result (1 if pContext == pCurrentContext, 0 otherwise)
; Clobbers: r0

isInAsyncContext:
    ; Load pCurrentContext into (r24:r0)
    lds r24, pCurrentContext           ; Load low byte of pCurrentContext
    lds r0, pCurrentContext+1          ; Load high byte of pCurrentContext

    ; test pCurrentContext (r24:r0) for NULL
    or r24, r0                         ; or low byte with high byte
    breq .equal                        ; NULL, r30 already 0 return it
    ldi r24, 1                         ; Set result to 1, in async context

.equal:
    ret                                ; Return

; this is a stub function which will be returned to when yieldContext returns or the pEntry function
; call returns without yielding or from yieldContext to restore the registers of the caller
restoreNoClobberRegs:
    pop r29                             ; Restore registers from stack of the restored context
    pop r28
    pop r17
    pop r16
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    ret                                ; Return to task execution point after its call to yieldContext

; Function: resumeContext
; Equivalent C: Restore the task's stack and resume execution in the new context
resumeContext:
    ; save caller's no clobber registers, because we can clobber them because from here
    ; will return to the yielded context's function body, with its own values for these registers
    ; r24:r25 contains pContext
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r28
    push r29

    ; we make the restoreNoClobberRegs the return address so it restores the caller's no clobber registers
    ; if the function returns without yielding
    ldi r22, lo8(restoreNoClobberRegs)  ; Load low byte of restoreNoClobberRegs
    ldi r23, hi8(restoreNoClobberRegs)  ; Load high byte of restoreNoClobberRegs, PC bit 0 is always 0

    ; Shift the address right by 1 (divide by 2) since AVR stores the address in WORD offsets
    lsr r23                             ; Logical shift right high byte
    ror r22                             ; Rotate right low byte through carry

    push r22                            ; Push restoreNoClobberRegs address onto stack
    push r23                            ; Push restoreNoClobberRegs address onto stack

    ; all stack data from this point will be saved by yieldContext, if it is called
    in r18, __SP_L__                    ; Read SP low byte
    in r19, __SP_H__                    ; Read SP high byte
    sts loopSP, r18                     ; Store SP low byte in loopSP
    sts loopSP+1, r19                   ; Store SP high byte in loopSP
    ; this is the marker for the start of switchable context

    movw r30, r24                        ; Move pContext to Z (r31:r30)
    sts pCurrentContext, r30             ; Store low byte of pCurrentContext from Z (r31:r30)
    sts pCurrentContext+1, r31           ; Store high byte of pCurrentContext from Z

    ; Load pStack
    movw r28, r30
    adiw r28, CONTEXT_PSTACK_OFFSET    ; Y = pStack (pContext + sizeof(*pContext)

    ; Load stackUsed
    ldd r22, Z+CONTEXT_STACK_USED        ; Load stackUsed

    ; if no stack is used, we will skip restoring the stack and proceed to calling the entry function
    cp r22, r1                          ; Compare stackUsed with 0
    brne .stackUsed                     ; If stackUsed is 0, skip restoring the stack

    ; just jump to pEntry, the return address is already on the stack
    ldd r22, Z+CONTEXT_PENTRY            ; Load low byte of pEntry
    ldd r23, Z+CONTEXT_PENTRY+1          ; Load high byte of pEntry
    ldd r24, Z+CONTEXT_PENTRY_ARG        ; Load low byte of pEntryArg
    ldd r25, Z+CONTEXT_PENTRY_ARG+1      ; Load high byte of pEntryArg
    movw r30, r22                        ; Move pEntry to Z (r31:r30)
    ijmp                                 ; jump to the function pointed to by Z (pEntry), with Argument

.stackUsed:
    ; Add stackUsed to pStack
    clr r1                              ; Clear r1 (already set to 0 but for clarity)
    add r28, r22                        ; Add stackUsed to pStack (low byte)
    adc r29, r1                         ; Add carry to pStack (high byte), Y is now pStack+stackUsed

.pushFromBuffer:
    ld r23, -Y                          ; Load byte from task stack buffer
    push r23                            ; Push byte onto stack
    dec r22                             ; Decrement stackUsed
    brne .pushFromBuffer                ; Repeat until stackUsed is zero

    ; clear stack used, because it was already restored
    std Z+CONTEXT_STACK_USED, r1         ; Clear stackUsed

    ; Return to task execution point after its call to yieldContext
    rjmp restoreNoClobberRegs

; Function: yieldContext
; called from within the body, or the body of its called functions, of a function's
; context which was started using resumeContext
; this will store the current context and return to the caller of resumeContext
yieldContext:
    ; save caller's no clobber registers, these become part of the
    ; context to be saved
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r28
    push r29

    lds r30, pCurrentContext            ; Load low byte of pCurrentContext into Z (r31:r30)
    lds r31, pCurrentContext+1          ; Load high byte of pCurrentContext into Z

    ; Load pStack
    lds r22, loopSP                     ; Load low byte of loopSP
    lds r23, loopSP+1                   ; Load high byte of loopSP

    ; Read current stack pointer
    in r24, __SP_L__                    ; Read SP low byte
    in r25, __SP_H__                    ; Read SP high byte
    sub r22, r24                        ; Calculate stackUsed
    sbc r23, r25                        ; Subtract SP high byte
    std Z+CONTEXT_STACK_USED, r22       ; Store stackUsed, we are assuming that the high byte is 0

    ; Update stackMaxUsed if necessary
    ldd r2, Z+CONTEXT_STACK_MAX_USED    ; Load stackMaxUsed
    cp r2, r22                          ; Compare stackMaxUsed with stackUsed
    brsh .notMaxUsed                    ; Skip if stackUsed <= stackMaxUsed
    std Z+CONTEXT_STACK_MAX_USED, r22   ; Update stackMaxUsed

.notMaxUsed:
    ; Load pStack
    movw r28, r30
    adiw r28, CONTEXT_PSTACK_OFFSET    ; Y = pStack (pContext + sizeof(*pContext)

.popToBuffer:
    pop r23                             ; Pop byte from stack
    st Y+, r23                          ; Store byte in task stack buffer
    dec r22                             ; Decrement stackUsed
    brne .popToBuffer                   ; Repeat until stackUsed is zero

    ; now the stack contains the return address, which is the restoreNoClobberRegs function
    ; it will restore the no clobber registers and return to the caller of resumeContext

    ; clear pCurrentContext
    clr r1                              ; Clear r1, which is already set to 0 but for clarity

    sts pCurrentContext, r1
    sts pCurrentContext+1, r1           ; clear pCurrentContext

    ret
