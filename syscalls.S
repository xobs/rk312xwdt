.section .text
.align 4

.global sys_exit
.func sys_exit
sys_exit:
    mov r7, #1
    svc 0
.endfunc

.global sys_write
.func sys_write
sys_write:
    push {r4-r7, lr}
    mov r7, #4
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_open
.func sys_open
sys_open:
    push {r4-r7, lr}
    mov r7, #5
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_close
.func sys_close
sys_close:
    push {r4-r7, lr}
    mov r7, #6
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_mmap
.func sys_mmap
sys_mmap:
    push {r4-r7, lr}
    mov r7, #90
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_munmap
.func sys_munmap
sys_munmap:
    push {r4-r7, lr}
    mov r7, #91
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_nanosleep
.func sys_nanosleep
sys_nanosleep:
    push {r4-r7, lr}
    mov r7, #162
    svc 0x0
    pop {r4-r7, pc}
.endfunc

.global sys_mmap2
.func sys_mmap2
sys_mmap2:
    push    {r5}
    ldr     r5, [sp, #8]
    push    {r4}
    ldr     r4, [sp, #8]
    push    {r7}

    mov r7, #192
    svc 0x0

    pop {r7}
    pop {r4}
    pop {r5}
    mov pc, lr
.endfunc
