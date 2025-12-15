#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INSTR 100
#define MEM_SIZE 100

// =============================
// CPU STRUCTURE
// =============================
struct CPU {
    int PC;             // Program Counter
    char IR[20];        // Instruction Register
    int ACC;            // Accumulator
    int REG[3];         // General Purpose Registers A,B,C
    int MODE;           // 0=USER, 1=SUPERVISOR
    int RUN;            // 1=Running, 0=Halted
    int CLOCK;          // Clock cycles
} cpu;

// =============================
// MEMORY & INTERRUPTS
// =============================
char memory[MEM_SIZE][20];
int instrCount = 0;

// Interrupt Vector Table (IVT)
void (*IVT[4])(); // 0=READ, 1=WRITE, 2=EXIT, 3=INVALID

// =============================
// FUNCTION DECLARATIONS
// =============================
void loadProgram();
void CPU_Run();
void fetch();
void decodeExecute();
void interruptHandler(int);
void switchToKernel();
void switchToUser();
void svc_read();
void svc_write();
void svc_exit();
void svc_invalid();
void displayCPU();
void drawLine();

// =============================
// MAIN
// =============================
int main() {
    system("clear||cls");
    printf("\n============================\n");
    printf("  MULTIPROGRAMMING OS STAGE I\n");
    printf("  CPU & SUPERVISOR CALL SIMULATOR\n");
    printf("============================\n\n");

    loadProgram();

    // Initialize CPU
    cpu.PC = 0;
    cpu.ACC = 0;
    cpu.MODE = 0;  // Start in USER mode
    cpu.RUN = 1;
    cpu.CLOCK = 0;
    cpu.REG[0] = 10;
    cpu.REG[1] = 20;
    cpu.REG[2] = 5;

    // Initialize IVT
    IVT[0] = svc_read;
    IVT[1] = svc_write;
    IVT[2] = svc_exit;
    IVT[3] = svc_invalid;

    printf("\nStarting execution in USER MODE...\n");
    drawLine();

    CPU_Run();

    drawLine();
    printf("\n===== PROGRAM TERMINATED =====\n");
    printf("Total clock cycles executed: %d\n", cpu.CLOCK);
    printf("====================================\n");
    return 0;
}

// =============================
// LOAD PROGRAM
// =============================
void loadProgram() {
    printf("Enter number of instructions (max %d): ", MAX_INSTR);
    scanf("%d", &instrCount);
    getchar();

    printf("\nAvailable Instructions:\n");
    printf(" LOAD A | ADD B | SUB C | MUL A\n");
    printf(" SVC 1 (READ) | SVC 2 (WRITE) | SVC 3 (EXIT)\n");
    printf("-----------------------------------------\n");

    for (int i = 0; i < instrCount; i++) {
        printf("Instruction %02d: ", i + 1);
        fgets(memory[i], sizeof(memory[i]), stdin);
        memory[i][strcspn(memory[i], "\n")] = '\0';
    }
    printf("\nProgram loaded successfully into memory.\n");
    drawLine();
}

// =============================
// CPU EXECUTION LOOP
// =============================
void CPU_Run() {
    while (cpu.RUN && cpu.PC < instrCount) {
        fetch();
        decodeExecute();
        cpu.CLOCK++;
    }
}

// =============================
// FETCH
// =============================
void fetch() {
    strcpy(cpu.IR, memory[cpu.PC]);
    printf("\n[USER MODE] Fetching Instruction #%d -> \"%s\"\n", cpu.PC + 1, cpu.IR);
}

// =============================
// DECODE & EXECUTE
// =============================
void decodeExecute() {
    if (strncmp(cpu.IR, "SVC", 3) == 0) {
        int callNo;
        sscanf(cpu.IR, "SVC %d", &callNo);
        interruptHandler(callNo - 1);  // Convert to 0-based index
    } 
    else if (strncmp(cpu.IR, "LOAD", 4) == 0) {
        char reg;
        sscanf(cpu.IR, "LOAD %c", &reg);
        if (reg == 'A') cpu.ACC = cpu.REG[0];
        else if (reg == 'B') cpu.ACC = cpu.REG[1];
        else if (reg == 'C') cpu.ACC = cpu.REG[2];
        printf("[USER MODE] ACC <- %d (Loaded from %c)\n", cpu.ACC, reg);
    }
    else if (strncmp(cpu.IR, "ADD", 3) == 0) {
        char reg;
        sscanf(cpu.IR, "ADD %c", &reg);
        cpu.ACC += (reg == 'A') ? cpu.REG[0] : (reg == 'B') ? cpu.REG[1] : cpu.REG[2];
        printf("[USER MODE] ACC = %d (After ADD %c)\n", cpu.ACC, reg);
    }
    else if (strncmp(cpu.IR, "SUB", 3) == 0) {
        char reg;
        sscanf(cpu.IR, "SUB %c", &reg);
        cpu.ACC -= (reg == 'A') ? cpu.REG[0] : (reg == 'B') ? cpu.REG[1] : cpu.REG[2];
        printf("[USER MODE] ACC = %d (After SUB %c)\n", cpu.ACC, reg);
    }
    else if (strncmp(cpu.IR, "MUL", 3) == 0) {
        char reg;
        sscanf(cpu.IR, "MUL %c", &reg);
        cpu.ACC *= (reg == 'A') ? cpu.REG[0] : (reg == 'B') ? cpu.REG[1] : cpu.REG[2];
        printf("[USER MODE] ACC = %d (After MUL %c)\n", cpu.ACC, reg);
    }
    else {
        printf("[ERROR] Unknown instruction '%s'.\n", cpu.IR);
        interruptHandler(3);
    }

    cpu.PC++;
    displayCPU();
}

// =============================
// INTERRUPT HANDLER
// =============================
void interruptHandler(int type) {
    printf("\n>>> SOFTWARE INTERRUPT (SVC %d) Triggered <<<\n", type + 1);
    switchToKernel();

    if (type >= 0 && type < 4)
        IVT[type]();
    else
        svc_invalid();

    if (cpu.RUN)
        switchToUser();
}

// =============================
// MODE SWITCHING
// =============================
void switchToKernel() {
    cpu.MODE = 1;
    printf("[SYSTEM] Switching to SUPERVISOR MODE...\n");
}

void switchToUser() {
    cpu.MODE = 0;
    printf("[SYSTEM] Returning to USER MODE...\n");
}

// =============================
// SUPERVISOR CALL HANDLERSy
// =============================
void svc_read() {
    char buffer[50];
    printf("[SVC 1] READ operation requested.\n");
    printf("OS: Enter input data: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("OS: Data '%s' read into system buffer.\n", buffer);
}

void svc_write() {
    printf("[SVC 2] WRITE operation requested.\n");
    printf("OS: Output from ACC = %d\n", cpu.ACC);
}

void svc_exit() {
    printf("[SVC 3] EXIT operation requested.\n");
    printf("OS: Releasing resources and halting user program.\n");
    cpu.RUN = 0;
}

void svc_invalid() {
    printf("[SVC ?] Invalid supervisor call detected. System halt.\n");
    cpu.RUN = 0;
}

// =============================
// CPU STATE DISPLAY
// =============================
void displayCPU() {
    printf("CPU STATE => [PC=%d | ACC=%d | MODE=%s | CLOCK=%d]\n",
           cpu.PC, cpu.ACC, cpu.MODE ? "SUPERVISOR" : "USER", cpu.CLOCK);
}

void drawLine() {
    printf("-----------------------------------------\n");
}