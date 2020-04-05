/*
   // Sistemas Operativos, DEI/IST/ULisboa 2017-18
   // Projeto SO - exercicio 1 12/11/2017
   // Miguel Coelho 87687
   // Diogo Sousa 87651
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "matrix2d.h"


/*--------------------------------------------------------------------
 | Struct && Global Variables
   ---------------------------------------------------------------------*/

typedef struct barreira {
        pthread_mutex_t b_mutex;
        pthread_cond_t b_cond;
        int b_flags[2]; //Variaveis de estado
        int b_exit, alrm_flag, int_flag; //Variavel utilizada para verificar a terminacao dinamica
}*Barreira;


typedef struct taskArg {
        int _id, _iteracoes, _ntasks, _N, _linhasFatia;
        double _maxD;
        char * _fileName;
}*TaskArg;

struct sigaction action;
sigset_t set;

TaskArg _TaskArg;
Barreira _Barreira;

DoubleMatrix2D *matrix;
DoubleMatrix2D *matrix_aux;

int* _nextIt;       //Array do estado de cada thread em cada iteracao
int periodoS;       //periodo do alarm
double maxD;        //Desvio máximo
FILE* fichP = NULL; //Pointer para o ficheiro de salvaguarda
char* backupName;   //Nome do ficheiro de backup = Nome de fichP + "~"
int* iter;

/*--------------------------------------------------------------------
 | Function: initBarreira
   ---------------------------------------------------------------------*/

void initBarreira(int ntasks){

        _Barreira = (Barreira) malloc(sizeof(struct barreira));
        if(pthread_mutex_init(&_Barreira->b_mutex, NULL) != 0) {
                fprintf(stderr, "\nErro ao inicializar mutex da barreira\n");
                exit(EXIT_FAILURE);
        }

        if(pthread_cond_init(&_Barreira->b_cond, NULL) != 0) {
                fprintf(stderr, "\nErro ao inicializar variável de condição da barreira\n");
                exit(EXIT_FAILURE);
        }

        _Barreira->b_flags[0] = ntasks-1;
        _Barreira->b_flags[1] = ntasks-1;

        _Barreira->b_exit = 0;
        _Barreira->alrm_flag = 0;
        _Barreira->int_flag = 0;
}


/*--------------------------------------------------------------------
 | Function: destroyBarreira
   ---------------------------------------------------------------------*/

void destroyBarreira(){

        if(pthread_mutex_destroy(&_Barreira->b_mutex) != 0) {
                fprintf(stderr, "\nErro ao destruir mutex da barreira\n");
                exit(EXIT_FAILURE);
        }

        if(pthread_cond_destroy(&_Barreira->b_cond) != 0) {
                fprintf(stderr, "\nErro ao destruir variável de condição da barreira\n");
                exit(EXIT_FAILURE);
        }

        free(_Barreira);
}


/*--------------------------------------------------------------------
 | Function: waitBarreira
   ---------------------------------------------------------------------*/

void waitBarreira(int i, int ntasks, char *fileName){
        int k;

        if(pthread_mutex_lock(&_Barreira->b_mutex) != 0) {
                fprintf(stderr, "\nErro ao bloquear mutex\n");
                exit(EXIT_FAILURE);
        }

        if(_Barreira->b_flags[i%2] == 0) {

                if(pthread_cond_broadcast(&_Barreira->b_cond) != 0) {
                        fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
                        exit(EXIT_FAILURE);
                }

                if( _Barreira->alrm_flag == 1) { //Se entrou no handler de alarm, inicia a salvaguarda
                        int status;
                        if ( (waitpid(0, &status, WNOHANG)) != 0) { //Inicia o save apenas se não existirem filhos vivos

                                int pid = fork();
                                if(pid == 0) {
                                        fichP = fopen(backupName,"w");
                                        dm2dPrintToFile(matrix_aux, fichP); //guarda para o backup
                                        fclose(fichP);
                                        rename(backupName, fileName); //se o programa nao for terminado, actualiza o save real
                                        exit(EXIT_SUCCESS);
                                }

                                else if(pid < 0) {
                                        fprintf(stderr, "\nErro ao fazer o fork\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        _Barreira->alrm_flag = 0; //reset á flag até a proximo alarm
                }

                if (_Barreira->int_flag == 1) { //Se entrou no handler de SigInt
                        fichP = fopen(backupName,"w"); //Inicia o save de backup
                        dm2dPrintToFile(matrix_aux, fichP);
                        fclose(fichP);
                        rename(backupName, fileName); //Repoe o save real

                        free(_TaskArg); //Free's da memoria
                        free(_nextIt);
                        free(backupName);
                        dm2dFree(matrix);
                        dm2dFree(matrix_aux);

                        exit(EXIT_SUCCESS); //e termina o programa
                }


                for(k = 0; k < ntasks; k++) { // Verificar o estado de cada thread nesta iteracao
                        if(_nextIt[k] == 0)
                                break;

                }

                if(k == ntasks) { //Se todas as threads querem terminar
                        _Barreira->b_exit = 1; //Alterar variavel de terminacao dinamica
                }

                _Barreira->b_flags[(i+1)%2] = ntasks-1; //Actualiza o valor da variavel de estado da iteracao seguinte
                dm2dCopy(matrix, matrix_aux); // Actualiza a cópia da matriz para serem efectuados os calculos da temperatura na iteracao seguinte	}
        }

        else{
                while (_Barreira->b_flags[i%2] > 0 ) {
                        _Barreira->b_flags[i%2]--; //Decrementa a variavel de estado para assinalar a espera da thread
                        if(pthread_cond_wait(&_Barreira->b_cond, &_Barreira->b_mutex) != 0) {
                                fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }

        if(pthread_mutex_unlock(&_Barreira->b_mutex) != 0) {
                fprintf(stderr, "\nErro ao desbloquear mutex\n");
                exit(EXIT_FAILURE);
        }
}


/*--------------------------------------------------------------------
 | Function: parse_integer_or_exit
   ---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
        int value;

        if(sscanf(str, "%d", &value) != 1) {
                fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
                exit(1);
        }
        return value;
}


/*--------------------------------------------------------------------
 | Function: parse_double_or_exit
   ---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
        double value;

        if(sscanf(str, "%lf", &value) != 1) {
                fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
                exit(1);
        }
        return value;
}


/*--------------------------------------------------------------------
 | Function: sigHandler
   ---------------------------------------------------------------------*/

void sigHandler(int num){

        signal(SIGALRM, sigHandler);
        signal(SIGINT, sigHandler);

        switch (num) {
        case SIGALRM:
                _Barreira->alrm_flag = 1;
                alarm(periodoS);
                break;
        case SIGINT:
                _Barreira->int_flag = 1;
                break;
        }
}


/*--------------------------------------------------------------------
 | Function: fnThread
   ---------------------------------------------------------------------*/

void *fnThread(void *a) {
        int i, j, k;
        double value;
        TaskArg arg  = (TaskArg) a;        //Inicializacao dos argumentos de cada thread
        int N = arg->_N;
        int linhasFatia = arg->_linhasFatia;
        int id = arg->_id;
        int iteracoes = arg->_iteracoes;
        double maxDIt = arg->_maxD;
        int ntasks = arg->_ntasks;
        char* fileName = arg->_fileName;

        for (i = 0; i < iteracoes; i++) {
                //Calculo das temperaturas numa iteracao
                for (k = ((linhasFatia*id)-(linhasFatia-1)); k < ((linhasFatia *(id+1))-(linhasFatia-1)); k++) // onde linhasFatia é o numero de linhas que cada thread deve calcular
                        for (j = 1; j < (N + 2) - 1; j++) { // onde N + 2 é o numero de colunas
                                value = (dm2dGetEntry(matrix, k-1, j) + dm2dGetEntry(matrix, k+1, j) +
                                         dm2dGetEntry(matrix, k, j-1) + dm2dGetEntry(matrix, k, j+1)) / 4.0;

                                if((value - dm2dGetEntry(matrix, k, j)) > maxDIt) { //Actualiza o valor da diferenca maxima desta thread para esta iteracao
                                        maxDIt = (value - dm2dGetEntry(matrix, k, j));

                                }
                                dm2dSetEntry(matrix_aux, k, j, value);
                        }


                if(maxDIt < maxD) { //Actualizacao do estado da thread
                        _nextIt[id-1] = 1;

                }

                waitBarreira(i, ntasks, fileName);
                if(_Barreira->b_exit == 1) //Terminar os calculos caso todas as threads verifiquem a condicao de terminacao dinamica
                {
                        *iter = i;
                        pthread_exit(NULL);
                }

                maxDIt = 0; //Repor o valor da maior diferenca em caso de realizar nova iteracao

        }

        return 0;
}


/*--------------------------------------------------------------------
 | Function: main
   ---------------------------------------------------------------------*/

int main (int argc, char **argv) {

        if(argc != 11) {
                fprintf(stderr, "\nNumero invalido de argumentos.\n");
                fprintf(stderr, "Uso: Projecto N tEsq tSup tDir tInf iteracoes num_tarefas maxD nome_ficheiro periodicidade\n\n");
                return 1;
        }

        /* argv[0] = program name */
        int N = parse_integer_or_exit(argv[1], "N");
        double tEsq = parse_double_or_exit(argv[2], "tEsq");
        double tSup = parse_double_or_exit(argv[3], "tSup");
        double tDir = parse_double_or_exit(argv[4], "tDir");
        double tInf = parse_double_or_exit(argv[5], "tInf");
        int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
        int ntasks = parse_integer_or_exit(argv[7], "num_tarefas");
        maxD = parse_double_or_exit(argv[8], "MaxD");
        char* fichS = argv[9];
        periodoS = parse_integer_or_exit(argv[10], "Periodicidade");


        backupName = (char*) malloc((strlen(fichS)+1+1) * sizeof(char)); //criacao do nome do ficheiro de backup
        strcpy(backupName,fichS);                                  //nome original + "~"
        strcat(backupName,"~");

        fprintf(stderr, "\nArgumentos:\n"
                " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d ntasks=%d MaxD=%.1f Nome_ficheiro=%s Periodicidade=%d\n",
                N, tEsq, tSup, tDir, tInf, iteracoes, ntasks, maxD, fichS, periodoS);


        if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || (N % ntasks) != 0 || maxD < 0 || periodoS < 0) {
                fprintf(stderr, "\nErro: Argumentos invalidos.\n"
                        " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, numero de tasks multiplo de N, MaxD >= 0, Periodicidade >=0\n\n");
                return 1;
        }

        int i;
        int linhasFatia = N / ntasks; //Numero de linhas a calcular por cada thread


        _nextIt = (int*) malloc(ntasks * sizeof(int)); //aloca memoria para o array de votacao (para a terminacao dinamica)
        iter = (int*) malloc(sizeof(int)); //aloca memoria para o array de votacao (para a terminacao dinamica)

        for (int i = 0; i < ntasks; i++) //inicializa os votos a 0
                _nextIt[i] = 0;

        pthread_t *tid;
        tid = (pthread_t*) malloc(ntasks * sizeof(pthread_t));

        _TaskArg = (TaskArg) malloc(sizeof(struct taskArg) * ntasks); // Aloca memoria para os argumentos das slaves

        matrix = dm2dNew(N + 2, N + 2);
        matrix_aux = dm2dNew(N + 2, N + 2);

        initBarreira(ntasks);

        if(periodoS > 0) { //Se a salvaguarda estiver activa, executa o primeiro alarm
                alarm(periodoS);
        }


        if (matrix == NULL) {
                fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a matriz.\n\n");
                return -1;
        }

        FILE* fchIn = fopen(fichS, "r");

        if (fchIn != NULL) { //Se o ficheiro já existir, carrega a matriz em memoria
                DoubleMatrix2D* r = readMatrix2dFromFile(fchIn, N+2, N+2);
                dm2dCopy(matrix,r);
                dm2dCopy(matrix_aux, matrix);
                dm2dFree(r);
                fclose(fchIn);

        }
        else{ //Se não inicializa a matriz com os valores dados

                dm2dSetLineTo (matrix, 0, tSup);
                dm2dSetLineTo (matrix, N + 1, tInf);
                dm2dSetColumnTo(matrix, 0, tEsq);
                dm2dSetColumnTo(matrix,N + 1, tDir);
                dm2dCopy(matrix_aux, matrix);

        }

        if(sigemptyset(&set) != 0) {
                fprintf(stderr, "\nErro ao inicializar o set\n");
                exit(EXIT_FAILURE);
        }

        if(sigaddset(&set, SIGINT) != 0) {
                fprintf(stderr, "\nErro ao adicionar o signal\n");
                exit(EXIT_FAILURE);
        }

        if(sigaddset(&set, SIGALRM) != 0) {
                fprintf(stderr, "\nErro ao adicionar o signal\n");
                exit(EXIT_FAILURE);
        }

        if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) { //bloqueia os signals(para as slaves)
                fprintf(stderr, "\nErro ao bloquear os signals\n");
                exit(EXIT_FAILURE);
        }



        for (i = 0; i < ntasks; i++) {    //Cria Tarefas e inicializa os seus argumentos
                _TaskArg[i]._id = i+1;
                _TaskArg[i]._iteracoes = iteracoes;
                _TaskArg[i]._ntasks = ntasks;
                _TaskArg[i]._N = N;
                _TaskArg[i]._linhasFatia = linhasFatia;
                _TaskArg[i]._maxD = maxD;
                _TaskArg[i]._fileName = fichS;
                if (pthread_create (&tid[i], NULL, fnThread, &_TaskArg[i]) != 0) {
                        printf("Erro ao criar tarefa.\n");
                        return 1;
                }
        }

        if(  pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0) {//desbloqueia a recepcao de signals para a main
                fprintf(stderr, "\nErro ao desbloquear os signals\n");
                exit(EXIT_FAILURE);
        }


        if(sigemptyset(&action.sa_mask) != 0) {
                fprintf(stderr, "\nErro ao inicializar o set\n");
                exit(EXIT_FAILURE);
        }

        action.sa_handler = sigHandler; //atribui a funçao de handle para SIGALM e SIGINT

        if(sigaction(SIGINT, &action, NULL) != 0) {
                fprintf(stderr, "\nErro ao alterar a ação do signal\n");
                exit(EXIT_FAILURE);
        }

        if(sigaction(SIGALRM, &action, NULL) != 0) {
                fprintf(stderr, "\nErro ao alterar ação do signal\n");
                exit(EXIT_FAILURE);
        }

        for (i = 0; i < ntasks; i++) {
                if (pthread_join (tid[i], NULL)) {
                        printf("Erro ao esperar por tarefa.\n");
                        return 2;
                }
        }

        dm2dPrint(matrix); //Print da matriz de resultado
        printf("Iterações: %d\n", *iter);
        //Free's e apagar ficheiros
        free(tid);
        free(_TaskArg);
        free(_nextIt);
        free(backupName);
        dm2dFree(matrix);
        dm2dFree(matrix_aux);
        destroyBarreira();
        unlink(fichS);

        return 0;
}
