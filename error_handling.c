/* oscp.c  -- C translation of your C++ OS-simulator code
   Compile: gcc oscp.c -o oscp.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

char M[300][4], IR[4], GR[4];
bool C;
int PTR;
struct PCB
{
    char job[4], TTL[4], TLL[4];
} pcb;
int VA, RA;
int TTC, LLC, TTL, TLL;
int EM;
int SI, TI, PI;
int ttl, tll;
FILE *fin, *fout;
char linebuf[1024];
int random_arr[30];
int l_read = -1;
int IC;
int pte;
int check1 = 0;
int InValid = 0;

void initialization();
void load();
void Pagetable();
void allocate_blocks();
void startExecution();
void executeProgram();
void AddMap();
void Examine();
void MOS();
int Errormsg();
void Terminate();
void read_op();
void write_op();
void printMem();

void initialization()
{
    SI = 0; TI = 0; PI = 0;
    TTC = 0; LLC = 0; TTL = 0; TLL = 0;
    EM = 0; VA = 0; RA = 0; IC = 0; PTR = 0;
    InValid = 0;
    for (int i = 0; i < 30; i++) random_arr[i] = 0;
    for (int i = 0; i < 4; i++) {
        IR[i] = '&';
        GR[i] = '_';
    }
    for (int i = 0; i < 300; i++) for (int j = 0; j < 4; j++) M[i][j] = '_';
}

void printMem(){
    for(int i=0;i<300;i++){
        printf("M[%d] ", i);
        for(int j=0;j<4;j++){
            putchar(M[i][j]);
        }
        putchar('\n');
    }
}

void load()
{
    while (fgets(linebuf, sizeof(linebuf), fin) != NULL)
    {
        size_t len = strlen(linebuf);
        if (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
            /* remove newline(s) */
            while (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
                linebuf[len-1] = '\0';
                len--;
            }
        }

        if (len >= 4 && linebuf[0] == '$' && linebuf[1] == 'A' && linebuf[2] == 'M' && linebuf[3] == 'J')
        {
            if (check1 > 0) {
                printMem();
            }
            initialization();
            check1++;

            /* read job id, TTL, TLL from fixed positions */
            /* ensure line has enough characters */
            if (len >= 16) {
                for (int i = 4, j = 0; i < 8; i++, j++) pcb.job[j] = linebuf[i];
                for (int i = 8, j = 0; i < 12; i++, j++) pcb.TTL[j] = linebuf[i];
                for (int i = 12, j = 0; i < 16; i++, j++) pcb.TLL[j] = linebuf[i];

                ttl = ((pcb.TTL[0]-'0')*1000 + (pcb.TTL[1]-'0')*100 + (pcb.TTL[2]-'0')*10 + (pcb.TTL[3]-'0'));
                tll = ((pcb.TLL[0]-'0')*1000 + (pcb.TLL[1]-'0')*100 + (pcb.TLL[2]-'0')*10 + (pcb.TLL[3]-'0'));
            } else {
                /* fallback if line shorter */
                ttl = 0; tll = 0;
            }
            Pagetable();
            allocate_blocks();
        }
        else if (len >= 4 && linebuf[0] == '$' && linebuf[1] == 'D' && linebuf[2] == 'T' && linebuf[3] == 'A')
        {
            printf("Job Execution Started\n");
            startExecution();
        }
    }
}

void Pagetable()
{
    int i, j;
    PTR = (rand() % 29) * 10;
    random_arr[PTR/10] = 1;

    for (i = PTR; i < PTR + 10 && i < 300; i++)
    {
        for (j = 0; j < 4; j++)
        {
            M[i][j] = '*';
        }
    }
}

void allocate_blocks()
{
    int check = 0;
    int i, j, pos = 0;
    int k = 0;
    char str[16];
    while (check != 1)
    {
        pos = (rand() % 29) * 10;
        while (random_arr[pos/10] != 0)
            pos = (rand() % 29) * 10;
        random_arr[pos/10] = 1;

        sprintf(str, "%d", pos);

        /* store pos (two-digit) in page table entry at PTR */
        if (pos < 100)
        {
            M[PTR][2] = '0';
            M[PTR][3] = str[0];
        }
        else
        {
            /* pos >= 100, use first two digits */
            M[PTR][2] = str[0];
            M[PTR][3] = str[1];
        }

        /* read next line from input file which contains program/data for this block */
        if (fgets(linebuf, sizeof(linebuf), fin) == NULL) break;

        size_t len = strlen(linebuf);
        if (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
            while (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
                linebuf[len-1] = '\0';
                len--;
            }
        }

        k = 0;
        /* place characters 4 per memory word */
        for (i = 0; k < (int)len && (pos + i) < 300; i++)
        {
            for (j = 0; j < 4 && k < (int)len; j++)
            {
                M[pos + i][j] = linebuf[k];
                k++;
                /* check next char for 'H' like original code */
                if (k < (int)len && linebuf[k] == 'H')
                {
                    check = 1;
                    /* put H in next memory location and mark rest as '0' */
                    if ((pos + i + 1) < 300) {
                        M[pos + i + 1][0] = 'H';
                        M[pos + i + 1][1] = '0';
                        M[pos + i + 1][2] = '0';
                        M[pos + i + 1][3] = '0';
                    }
                } else {
                    /* continue */
                }
            }
        }
    }
}

void startExecution()
{
    IC = 0;
    executeProgram();
}

void executeProgram()
{
    int no;
    char v[3];
    int i, j, k;
    j = 0;

    /* build page table entry value from PTR */
    v[0] = M[PTR][2];
    v[1] = M[PTR][3];
    v[2] = '\0';
    no = ((v[0]-'0')*10) + (v[1]-'0');

    /* reset IR so loop works */
    for (int idx = 0; idx < 4; idx++) IR[idx] = '\0';

    while (IR[0] != 'H')
    {
        int mem_index = (no * 10) + IC;
        if (mem_index < 0 || mem_index >= 300) {
            PI = 2;
            if (TTC >= ttl) TI = 2; else TI = 0;
            MOS();
            return;
        }
        for (k = 0; k < 4; k++)
        {
            IR[k] = M[mem_index][k];
        }

        /* validate operand digits */
        if (!isdigit((unsigned char)IR[2]) || !isdigit((unsigned char)IR[3]) || isalpha((unsigned char)IR[2]) || isalpha((unsigned char)IR[3]))
        {
            PI = 2;
            if (TTC >= ttl) TI = 2; else TI = 0;
            MOS();
            return;
        }

        VA = ((IR[2]-'0')*10) + (IR[3]-'0');
        AddMap();
        Examine();
        /* loop will continue or MOS may terminate */
    }
}

void AddMap()
{
    int pos;
    char str[16];
    pte = PTR + (VA / 10);
    if (pte < 0 || pte >= 300) {
        PI = 2;
        TI = 0;
        MOS();
        return;
    }

    if (M[pte][3] == '*')
    {
        PI = 3;
        pos = (rand() % 29) * 10;
        while (random_arr[pos/10] != 0) pos = (rand() % 29) * 10;
        random_arr[pos/10] = 1;
        sprintf(str, "%d", pos);
        if (pos / 100 == 0)
        {
            M[pte][2] = '0';
            M[pte][3] = str[0];
        }
        else
        {
            M[pte][2] = str[0];
            M[pte][3] = str[1];
        }
    }
    else
        PI = 0;

    int p = (M[pte][2] - '0') * 10 + (M[pte][3] - '0');
    RA = (p * 10) + (VA % 10);
    if (RA > 300)
    {
        PI = 2;
        TI = 0;
        MOS();
    }
}

void Examine()
{
    if (IR[0] == 'G')
    {
        IC = IC + 1;
        if (IR[1] == 'D')
        {
            SI = 1;
            TTC = TTC + 2;
            if (TTC < ttl) TI = 0; else TI = 2;
            MOS();
        }
        else
        {
            PI = 1;
            if (TTC > ttl) TI = 2;
            else TI = 0;
            MOS();
        }
    }
    else if (IR[0] == 'P')
    {
        IC = IC + 1;
        if (IR[1] == 'D')
        {
            LLC = LLC + 1;
            TTC = TTC + 1;
            SI = 2;
            if (TTC < ttl)
            {
                TI = 0;
                if (PI == 3) {
                    InValid = 1;
                }
            }
            else TI = 2;
        }
        else
        {
            PI = 1;
            if (TTC >= ttl) TI = 2; else TI = 0;
        }
        MOS();
    }
    else if (IR[0] == 'L')
    {
        IC = IC + 1;
        if (IR[1] == 'R') {
            if (PI == 3) {
                InValid = 1;
                TI = 0;
                MOS();
            } else {
                for (int j = 0; j < 4; j++) GR[j] = M[RA][j];
                TTC++;
            }
            if (TTC > ttl) {
                PI = 3;
                TI = 2;
                MOS();
            }
        } else {
            PI = 1;
            if (TTC >= ttl) TI = 2;
            TI = 0;
            MOS();
        }
    }
    else if (IR[0] == 'S')
    {
        IC = IC + 1;
        if (IR[1] == 'R') {
            for (int j = 0; j < 4; j++) M[RA][j] = GR[j];
            TTC = TTC + 2;
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1;
            if (TTC > ttl) TI = 2;
            TI = 0;
            MOS();
        }
    }
    else if (IR[0] == 'C')
    {
        IC = IC + 1;
        if (IR[1] == 'R') {
            if (PI == 3) {
                InValid = 1;
                TI = 0;
                MOS();
            } else {
                if (M[RA][0] == GR[0] && M[RA][1] == GR[1] && M[RA][2] == GR[2] && M[RA][3] == GR[3])
                    C = true;
                else
                    C = false;
                TTC++;
            }
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1;
            if (TTC > ttl) TI = 2;
            TI = 0;
            MOS();
        }
    }
    else if (IR[0] == 'B')
    {
        IC = IC + 1;
        if (IR[1] == 'T') {
            if (PI == 3) {
                InValid = 1;
                TI = 0;
                MOS();
            } else {
                if (C == true) IC = VA;
                TTC++;
            }
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1;
            if (TTC > ttl) TI = 2;
            TI = 0;
            MOS();
        }
    }
    else if (IR[0] == 'H')
    {
        IC = IC + 1;
        TTC++;
        if (TTC > ttl) {
            TI = 2;
            PI = 3;
            MOS();
        } else {
            SI = 3;
            MOS();
        }
    }
    else
    {
        PI = 1;
        if (TTC > ttl) TI = 2;
        TI = 0;
        MOS();
    }
}

void MOS()
{
    if (PI == 1)
    {
        if (TI == 0) {
            EM = 4;
            fprintf(fout, "OpCode Error\n");
            Terminate();
        } else if (TI == 2) {
            EM = 3;
            fprintf(fout, "Time Limit Exceeded\n");
            EM = 4;
            fprintf(fout, "Operation Code Error\n");
            Terminate();
        }
    }
    else if (PI == 2)
    {
        if (TI == 0)
        {
            EM = 5;
            fprintf(fout, "Operand Error\n");
            Terminate();
        }
        else if (TI == 2)
        {
            EM = 3;
            fprintf(fout, "Time Limit Exceeded\n");
            EM = 5;
            fprintf(fout, "Operand Error\n");
            Terminate();
        }
    }
    else if (PI == 3)
    {
        if (TI == 0)
        {
            if (InValid == 1)
            {
                EM = 6;
                fprintf(fout, "Invalid Page Fault\n");
                Terminate();
            } else {
                /* page allocated earlier by AddMap, continue */
            }
        }
        else if (TI == 2)
        {
            EM = 3;
            fprintf(fout, "Time Limit Exceeded\n");
            Terminate();
        }
    }

    if (SI == 1)
    {
        if (TI == 0) read_op();
        else if (TI == 2) {
            EM = 3;
            fprintf(fout, "Time Limit Exceeded\n");
            Terminate();
        }
    }

    if (SI == 2)
    {
        if (TI == 0) write_op();
        else if (TI == 2) {
            write_op();
            EM = 3;
            fprintf(fout, "Time Limit Exceeded\n");
            Terminate();
        }
    }

    if (SI == 3)
    {
        EM = 0;
        fprintf(fout, "No Error\n");
        Terminate();
    }
}

void Terminate()
{
    fprintf(fout, "Job ID : %c%c%c%c  TTL=%d  TLL=%d\n", pcb.job[0], pcb.job[1], pcb.job[2], pcb.job[3], ttl, tll);
    fprintf(fout, "PTR = %d IC = %d EM = %d IR = ", PTR, IC, EM);
    for (int i = 0; i < 4; i++) fputc(IR[i], fout);
    fputc('\n', fout);
    fprintf(fout, "SI = %d  TI =%d  PI=%d\n", SI, TI, PI);
    fprintf(fout, "TTC = %d  LLC=%d\n", TTC, LLC);
    fprintf(fout, "-----------------------------------------------------------------------------------\n");
    fprintf(fout, "-----------------------------------------------------------------------------------\n");

    /* After job termination original code calls load() again and exit(0)
       We'll call load() to potentially start next job and then exit.
       To avoid deep recursion, simply return to caller after calling load.
    */
    load();
    exit(0);
}

void read_op()
{
    /* read next line from input; if $END then Out of Data */
    if (fgets(linebuf, sizeof(linebuf), fin) == NULL) {
        EM = 1;
        fprintf(fout, "Out of Data\n");
        Terminate();
        return;
    }
    size_t len = strlen(linebuf);
    if (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
        while (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r')) {
            linebuf[len-1] = '\0';
            len--;
        }
    }
    if (len >= 4 && linebuf[0] == '$' && linebuf[1] == 'E' && linebuf[2] == 'N' && linebuf[3] == 'D')
    {
        EM = 1;
        fprintf(fout, "Out of Data\n");
        Terminate();
        return;
    }

    int i = 0;
    int k = 0;
    while (k < (int)len && (RA + i) < 300)
    {
        for (int j = 0; j < 4 && k < (int)len; j++)
        {
            M[RA + i][j] = linebuf[k];
            k++;
        }
        i++;
    }
}

void write_op()
{
    char buff[4096];
    int ra = RA;
    int i, j, k;
    k = 0;
    if (LLC > tll)
    {
        EM = 2;
        fprintf(fout, "Line Limit Exceeded\n");
        Terminate();
    }
    while (true)
    {
        for (i = 0; i < 4; i++)
        {
            if (M[ra][i] == '_' || M[ra][i] == '\0') break;
            buff[k++] = M[ra][i];
            /* avoid overflow */
            if (k >= (int)sizeof(buff) - 1) break;
        }
        if (i < 4 && (M[ra][i] == '_' || M[ra][i] == '\0')) break;
        ra++;
        if (ra >= 300) break;
    }
    buff[k] = '\0';
    fprintf(fout, "%s\n\n\n", buff);
}

int main()
{
    srand((unsigned int)time(NULL));
    fin = fopen("input1.txt", "r");
    if (!fin) {
        fprintf(stderr, "Could not open input1.txt\n");
        return 1;
    }
    fout = fopen("output1.txt", "w");
    if (!fout) {
        fprintf(stderr, "Could not open output1.txt\n");
        fclose(fin);
        return 1;
    }

    load();

    fclose(fin);
    fclose(fout);
    return 0;
}