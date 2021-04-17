#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

//definice chybových kódů
#define PARAMS_ERR 1

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

//struktura na uložení parametrů
typedef struct Params
{
    int NE;
    int NR;
    int TE;
    int TR;
} Params;

//funkce vypisující chybová hlášení, uvolňující paměť a ukončující běh programu
void error_exit(int error_code)
{
    switch (error_code)
    {
    case PARAMS_ERR: //chyba v zadání parametrů
        fprintf(stderr, "Wrong parameters!");
        exit (1);
        break;
    default:
        break;
    }
}

//funkce na zkontrolování, zda parametry vyhovují zadání
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

    for(int i = 0; i < PARAMS_CNT; i++)
    {
        if (*ptrs[i] != '\0')
        {
            error_exit(PARAMS_ERR);
        }
    }
}

//funkce na načtení parametrů do struktury
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

int main(int argc, char* argv[])
{
    //načte parametry
    Params params = load_params(argc, argv);
    (void)params;
    return 0;
}