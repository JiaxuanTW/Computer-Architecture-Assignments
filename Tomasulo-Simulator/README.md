# Tomasulo Algorithm Simulator

## Constants
Can be changed in `#define` area and `Tomasulo()` constructor function

#### Reservation Stations
* 3 Adder RSs
* 2 Multiplier RSs
* 2 Load Buffers
* 2 Store Buffers

#### Instructions
* L.D - 2 cycles
* S.D - 1 cycle
* ADD.D - 2 cycles
* SUB.D - 2 cycles
* MUL.D - 10 cycles
* DIV.D - 40 cycles

#### Registers and Memory
* 32 Integer Registers (R0, R1, ... , R31) - default value = 0, except R1 = 16
* 16 Floating-point Registers (R0, R2, ... , R30) - default value = 1.0
* 8 Double-pecision-space Memory (Total 64KB) - default value = 1.0

## Arguments
```
Tomasulo.exe input_file
```
* *input_file*:  Relative path to the instruction trace file.

## Outputs
```
Clock Cycle: 7
+-----------------------------------------+
| Instructions          Issue ExecC Write |
+-----------------------------------------+
| L.D    F6,  8(R2)     1     3     4     |
| L.D    F2,  40(R3)    2     4     5     |
| ADD.D  F4,  F2,  F6   3     7           |
| DIV.D  F8,  F0,  F4   4                 |
| MUL.D  F6,  F8,  F4   5                 |
| SUB.D  F10, F2,  F4   6                 |
| SUB.D  F14, F8,  F4   7                 |
| SUB.D  F10, F6,  F4                     |
| ADD.D  F12, F6,  F4                     |
+-----------------------------------------+
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Name   | Busy   | Op     | Vj     | Vk     | Qj     | Qk     | A      | Time   |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Add0   | Yes    | ADD.D  | 1      | 1      |        |        |        | 0      |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Add1   | Yes    | SUB.D  | 1      |        |        | Add0   |        | 2      |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Add2   | Yes    | SUB.D  |        |        | Mult0  | Add0   |        | 2      |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Mult0  | Yes    | DIV.D  | 1      |        |        | Add0   |        | 40     |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Mult1  | Yes    | MUL.D  |        |        | Mult0  | Add0   |        | 10     |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Load0  | No     |        |        |        |        |        |        |        |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Load1  | No     |        |        |        |        |        |        |        |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Store0 | No     |        |        |        |        |        |        |        |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| Store1 | No     |        |        |        |        |        |        |        |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
| F0     | F2     | F4     | F6     | F8     | F10    | F12    | F14    | F16    | F18    | F20    | F22    | F24    | F26    | F28    | F30    |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
|        |        | Add0   | Mult1  | Mult0  | Add1   |        | Add2   |        |        |        |        |        |        |        |        |
+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
```