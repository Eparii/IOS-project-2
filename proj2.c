#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

//definice chybových kódů
#define PARAMS_ERR 1
#define FILE_ERR 2
#define MAP_ERR 3
#define FORK_ERR 4
#define SEM_INIT_ERR 5

//definice zpráv sobů
#define RSTARTED 1
#define RETURN 2
#define HITCHED 3

//definice zpráv elfů
#define STARTED 1
#define NEED_HELP 2
#define GET_HELP 3
#define HOLIDAYS 4

//definice zpráv santy
#define SLEEP 1
#define HELP 2
#define CLOSING 3
#define CHRISTMAS 4

//definice rozmezí, které vyhovuje zadání
#define PARAMS_CNT 5
#define NE_MAX 999
#define NE_MIN 1
#define NR_MAX 19
#define NR_MIN 1
#define TE_MAX 1000
#define TE_MIN 0
#define TR_MAX 1000
#define TR_MIN 0

int *number = NULL, *e_waiting = NULL, *r_hitched = NULL, *e_helped = NULL, *r_back = NULL, *work_closed = NULL;
sem_t *santa = NULL, *elves = NULL, *reindeers = NULL, *writing = NULL;
FILE *file;

//struktura na uložení parametrů
typedef struct Params
{
    int NE;
    int NR;
    int TE;
    int TR;
} Params;


//funkce uvolňující sdílenou paměť, semafory a soubor
void clean()
{
    sem_unlink("xtetau00_sem_santa");
    sem_unlink("xtetau00_sem_elves");
    sem_unlink("xtetau00_sem_reindeers");
    sem_unlink("xtetau00_sem_writing");
    sem_destroy(writing);
    sem_destroy(reindeers);
    sem_destroy (elves);
    sem_destroy(santa);
    munmap(number, sizeof(*(number)));
    munmap(e_waiting, sizeof(*(e_waiting)));
    munmap(e_helped, sizeof(*(e_helped)));
    munmap(r_hitched, sizeof(*(r_hitched)));
    munmap(r_back, sizeof(*(r_back)));
    munmap(work_closed, sizeof(*(work_closed)));

    if (file != NULL) { fclose (file); }
}

//funkce vypisující chybová hlášení, uvolňující paměť a ukončující běh programu
void error_exit(int error_code)
{
    switch (error_code)
    {
    case PARAMS_ERR: //chyba v zadání parametrů
        fprintf(stderr, "Wrong parameters");
        exit (1);
        break;
    case FILE_ERR: //chyba v otevření nebo vyvoření souboru
        fprintf(stderr, "Error in creating/opening file");
        exit (1);
        break;
    case MAP_ERR: // chyba v mapování paměti
        fprintf(stderr, "Creating shared memory failed");
        clean();
        exit(1);
        break;
    case FORK_ERR:
        fprintf(stderr, "Forking process failed");
        clean();
        exit(1);
        break;
    case SEM_INIT_ERR:
        fprintf(stderr, "Semaphore initialization failed");
        clean();
        exit(1);
        break;
    default:
        break;
    }
}

//pomocna funkce pro map_variables
int* MMAP (int *pointer)
{
    pointer = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (pointer == MAP_FAILED)
    {
        error_exit(MAP_ERR);
    }
    return pointer;
}

//funkce vytvářející sdílenou paměť
void map_variables()
{
    e_waiting = MMAP(e_waiting);
    r_back = MMAP(r_back);
    number = MMAP(number);
    e_helped = MMAP(e_helped);
    r_hitched = MMAP(r_hitched);
    work_closed = MMAP(work_closed);
}

//funkce inicializujici globální proměnné
void init_variables()
{
    *e_waiting = 0;
    *r_back = 0;
    *number = 1;
    *e_helped = 0;
    *r_hitched = 0;
    *work_closed = 0;
}

//funkce pro zkontrolování, zda parametry vyhovují zadání
void check_params(Params params, char** ptrs)
{
    if (params.NE > NE_MAX || params.NR > NR_MAX || params.TE > TE_MAX || params.TR > TR_MAX)
    {
        error_exit(PARAMS_ERR);
    }
    
    if (params.NE < NE_MIN || params.NR < NR_MIN || params.TE < TE_MIN || params.TR < TR_MIN)
    {
        error_exit(PARAMS_ERR);
    }
    // params_cnt - 1 protože první parametr je cesta k souboru
    for(int i = 0; i < PARAMS_CNT - 1; i++)
    {
        if (*ptrs[i] != '\0')
        {
            error_exit(PARAMS_ERR);
        }
    }
}

//funkce pro načtení parametrů do struktury
Params load_params(int argc, char* argv[])
{
    if (argc != 5)
    {
        error_exit(PARAMS_ERR);
    }
    Params params;
    char* ptrs[PARAMS_CNT];
    params.NE = strtol(argv[1], &ptrs[0], 10);
    params.NR = strtol(argv[2], &ptrs[1], 10);
    params.TE = strtol(argv[3], &ptrs[2], 10);
    params.TR = strtol(argv[4], &ptrs[3], 10);
    check_params(params, ptrs);
    return params;
}

//funkce pro otevření souboru
void open_file()
{
    file = fopen("proj2.out", "w");
    if (file == NULL)
    {
        error_exit (FILE_ERR);
    }
}

//funkce pro inicializaci semaforů
int init_semaphores()
{
    santa = sem_open("xtetau00_sem_santa", O_CREAT | O_EXCL, 0666, 0);
    elves = sem_open("xtetau00_sem_elves", O_CREAT | O_EXCL, 0666, 0);
    reindeers = sem_open("xtetau00_sem_reindeers", O_CREAT | O_EXCL, 0666, 0);
    writing = sem_open("xtetau00_sem_writing", O_CREAT | O_EXCL, 0666, 1);
    if (santa == SEM_FAILED || elves == SEM_FAILED || reindeers == SEM_FAILED || writing == SEM_FAILED)
    {
        return -1;
    }
    return 0;
}

//funkce pro zapsání zpráv santy
void santa_message(int message)
{
    sem_wait(writing);
    switch (message)
    {
    case SLEEP:
        printf("%d: Santa: going to sleep\n", (*number)++);
        break;
    case HELP:
        printf("%d: Santa: helping elves\n", (*number)++);
        break;
    case CLOSING:
        printf ("%d: Santa: closing workshop\n", (*number)++);
        break;
    case CHRISTMAS:
        printf ("%d: Santa: Christmas started\n", (*number)++);
        break;
    }
    sem_post(writing);

}

//funkce pro zapsání zpráv elfů
void elf_message(int message, int elveid)
{
    sem_wait(writing);
    switch (message)
    {
    case STARTED:
        printf ("%d: Elf %d: started \n", (*number)++, elveid);
        break;
    case NEED_HELP:
        printf ("%d: Elf %d: need help\n", (*number)++, elveid);
        break;
    case GET_HELP:
        printf ("%d: Elf %d: get help\n", (*number)++, elveid);
        break;
    case HOLIDAYS:
        printf ("%d: Elf %d: taking holidays\n", (*number)++, elveid);
        break;
    }
    sem_post(writing);
}

void reindeer_message(int message, int reindeerid)
{
    sem_wait(writing);
    switch (message)
    {
    case RSTARTED:
        printf ("%d: RD %d: rstarted \n", (*number)++, reindeerid);
        break;
    case RETURN:
        printf ("%d: RD %d: return home\n", (*number)++, reindeerid);
        break;
    case GET_HELP:
        printf ("%d: RD %d: get hitched\n", (*number)++, reindeerid);
        break;
    }
    sem_post(writing);
}

//funkce pro ovládání procesu santa
void proc_santa()
{
    while(1)
    {
        santa_message(SLEEP);
        sem_wait(santa);
        if (*work_closed == 1)
        {
            santa_message(CLOSING);
            for (int i = 0; i < *r_back; i++) { sem_post(reindeers); } //pustí všechny soby do fáze zapřáhnutí
            sem_wait(santa); //čeká se, než zapřáhne všechny soby
            santa_message(CHRISTMAS);
            exit(0);
        }
        santa_message(HELP);
        for (int i = 0; i < 3; i++) { sem_post(elves); } //pustí 3 elfy do své dílny
        sem_wait(santa); //čeká se, než pomůže všem elfům
    }
}

//funkce na ovládání procesů elfů
void proc_elves(int NE, int TE)
{
    for (int i = 0; i < NE; i++)
    {   
        pid_t id = fork();
        if (id == 0)
        {
            elf_message(STARTED, i + 1); 
            while (1)
            {
                srand(time(NULL) * getpid()); // seed funkce random
                usleep((rand() % TE) * 1000); //usleep na náhodný čas mezi 0 a TE 
                elf_message(NEED_HELP, i + 1);
                if (*work_closed == 1) // pokud je dílna zavřená, elf odejde na dovolenou a ukončí proces
                {
                    elf_message(HOLIDAYS, i+1);
                    exit(0);
                }
                (*e_waiting)++;
                if ((*e_waiting) >= 3) // pokud jsou ve frontě alespoň 3 elfové, probudi santu
                {
                    *e_waiting -= 3; // odečte 3 elfy z čekající fronty
                    sem_post(santa);
                }
                sem_wait(elves); // elfové čekají, dokud nebudou 3, nebo nepřijde poslední sob
                if (*work_closed == 1)
                {
                    elf_message(HOLIDAYS, i+1);
                    exit(0);
                }
                elf_message(GET_HELP, i + 1); 
                (*e_helped)++;
                if (*e_helped == 3)
                {
                    *e_helped = 0;
                    sem_post(santa); // po pomoci všem 3 elfům v dílně jde santa spát (další smyčka cyklu)
                }
            }   
        }
        else if (id == -1) { error_exit (FORK_ERR); }
    }
}

//funkce na ovládání procesů sobů
void proc_reindeers (int NR, int TR)
{
    for (int i = 0; i < NR; i++)
    {   
        pid_t id = fork();
        if (id == 0)
        {
            srand(time(NULL) * getpid()); // seed funkce random
            reindeer_message(RSTARTED, i + 1); 
            usleep(((rand() % (TR)) + TR/2) * 1000 ); //usleep na náhodný čas mezi TR/2 a TR 
            reindeer_message(RETURN, i + 1);
            (*r_back)++;
            if (*r_back == NR)
            {
                *work_closed = 1;
                sem_post(santa); //poslední sob probudí santu
            }
            sem_wait(reindeers); //  sobi čekají, dokud nebudou doma všichni
            for(int j = 0; j < *e_waiting; j++) { sem_post (elves); };
            reindeer_message(HITCHED, i + 1); 
            (*r_hitched)++;
            if (*r_hitched == NR) // pokud jsou zapřáhnuti všichni sobi, začínají vánoce
            {
                sem_post(santa); // pustí santu do "christmas started"
            }
            exit(0); // ukončí proces sob
        }
        else if (id == -1) { error_exit (FORK_ERR); }
    }
}

int main(int argc, char* argv[])
{
    if (init_semaphores() == -1) //inicializuje semafory, v případě chyby uvolňuje paměť a ukončuje program
    {
        error_exit(SEM_INIT_ERR);
    }
    Params params = load_params(argc, argv); //načte parametry
    open_file(); // otevře soubor
    map_variables(); // vytvoří sdílené proměnné
    init_variables(); // inicializuje sdílené proměnné
    pid_t id = fork(); // rozdělí proces na "santa" a "hlavní proces" 
    if (id == 0) 
    {
        proc_santa(); // začne vykonávat proces santa
    }
    else if (id == -1) { error_exit (FORK_ERR); }
    else
    {
        pid_t id2 = fork(); // rozdělí hlavní proces na "elfové" a "hlavní proces"
        if (id2 == 0)
        {
            proc_elves(params.NE, params.TE); // začne vykonávat NE procesů skřítků
        }
        else if (id2 == -1) { error_exit (FORK_ERR); }
        else
        {
            pid_t id3 = fork(); // rozdělí hlavní proces na "sobi" a "hlavní proces" 
            if (id3 == 0)
            {
                proc_reindeers(params.NR, params.TR); // začně vykonávat NR procesů skřítků
            }
            else if (id == -1) { error_exit (FORK_ERR); }
        }
        while (wait (NULL)) // čeká na ukončení všech child procesů
        {
            if (errno == ECHILD) {break;}
        }
        clean(); // uvolní sdílenou paměť, semafory a soubor  
        exit (0);
    }
}