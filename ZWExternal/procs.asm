.code

ZwQueryVirtualMemory proc
	mov r10, rcx
	mov eax, 23h
	syscall
	ret
ZwQueryVirtualMemory endp

ZwOpenProcess proc
	mov r10, rcx
	mov eax, 26h
	syscall
	ret
ZwOpenProcess endp

ZwWriteVirtualMemory proc
	mov r10, rcx
	mov eax, 3Ah
	syscall
	ret
ZwWriteVirtualMemory endp

ZwReadVirtualMemory proc
	mov r10, rcx
	mov eax, 3Fh
	syscall
	ret
ZwReadVirtualMemory endp

ZwProtectVirtualMemory proc
	mov r10, rcx
	mov eax, 50h
	syscall
	ret
ZwProtectVirtualMemory endp

end