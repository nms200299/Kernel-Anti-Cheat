# Windows Injection 기법 조사

- 목적
  - 어떤 Windows Injection 기법이 있는지 조사합니다.
  - 다양한 Windows OS와 Bit Architecture에서 기법이 유효한지 조사합니다.
  - 이를 통해 차단할 공격 기법과 수행할 공격 기법을 파악할 수 있습니다.

---

- Windows OS별 Injection 기법 유효 여부

| Injection 기법                     | XP(sp.3) x86 | XP(sp.1) x64 | 7(sp.1) x86 | 7(sp.1) x64 | 10 22H2 x86 | 10 22H2 x64 | 11 24H2 x64 |
| ---------------------------------- | ------------ | ------------ | ----------- | ----------- | ----------- | ----------- | ----------- |
| DLL Injection (CreateRemoteThread) | O            | O            | O           | O           | O           | O           | O           |
| DLL Injection (NtCreateThread)     | X            | X            | O           | O           | O           | O           | O           |
| DLL Injection (SetWindowsHookEx)   | O            | O            | O           | O           | O           | O           | O           |
| IAT Hooking                        | O            | O            | O           | O           | O           | O           | O           |
| PE Injection (CreateRemoteThread)  | X            | X            | O           | O           | O           | O           | O           |
| PE Injection (NTCreateThreadEX)    | X            | X            | O           | O           | O           | O           | O           |
| Process Hollowing                  | O            | O            | O           | O           | O           | O           | O           |
| Reflective DLL Injection           | X            | X            | O           | △           | O           | X           | X           |
| Thread Injection                   | O            | O            | O           | O           | O           | O           | X           |
| Trampoline API Hooking             | O            | O            | O           | O           | X           | X           | X           |
