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

#include "matrix2d.h"


/*--------------------------------------------------------------------
| Struct && Global Variables
---------------------------------------------------------------------*/

typedef struct barreira{
  pthread_mutex_t b_mutex;
  pthread_cond_t b_cond;
  int b_flags[2]; 
  int b_exit;
}*Barreira;


typedef struct taskArg{
  int _id, _iteracoes, _ntasks, _N, _linhasFatia;
  double _maxD;
}*TaskArg;

TaskArg _TaskArg;
Barreira _Barreira;

DoubleMatrix2D *matrix;
DoubleMatrix2D *matrix_aux;

int *_nextIt;
double maxD;


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

void waitBarreira(int i, int ntasks,int id){

  int j, k;

  

  if(pthread_mutex_lock(&_Barreira->b_mutex) != 0) {
    fprintf(stderr, "\nErro ao bloquear mutex\n");
    exit(EXIT_FAILURE);}


  if(_Barreira->b_flags[i%2] == 0){
		pthread_cond_broadcast(&_Barreira->b_cond);

		for(j=0,k = 0; k < ntasks; k++){
		 	if(_nextIt[k] == 0)
		 		break;
		 	j++;
		 }

		if(j == ntasks){
		 	_Barreira->b_exit = 1;	
		}
  		_Barreira->b_flags[(i+1)%2] = ntasks-1;
  		dm2dCopy(matrix, matrix_aux); // Actualiza a cópia da matriz para serem efectuados os calculos da temperatura
	}
  else{
    while (_Barreira->b_flags[i%2] > 0 ) {
    	_Barreira->b_flags[i%2]--;
    	pthread_cond_wait(&_Barreira->b_cond,&_Barreira->b_mutex);
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
| Function: fnThread
---------------------------------------------------------------------*/

void *fnThread(void *a) {

  int i, j, k;
  double value;

  TaskArg arg  = (TaskArg) a;              //Inicializacao dos argumentos
  int N = arg->_N;
  int linhasFatia = arg->_linhasFatia;
  int id = arg->_id; 
  int iteracoes = arg->_iteracoes;
  double maxDIt = arg->_maxD; 
  int ntasks = arg->_ntasks; 

  for (i = 0; i < iteracoes; i++) {

    //Calculo das temperaturas numa iteracao

    for (k = ((linhasFatia*id)-(linhasFatia-1)); k < ((linhasFatia *(id+1))-(linhasFatia-1)); k++) // onde linhasFatia + 2 é o numero de linhas
      for (j = 1; j < (N + 2) - 1; j++) { // onde N + 2 é o numero de colunas
    	value = (dm2dGetEntry(matrix, k-1, j) + dm2dGetEntry(matrix, k+1, j) +
        dm2dGetEntry(matrix, k, j-1) + dm2dGetEntry(matrix, k, j+1)) / 4.0;
        if((value - dm2dGetEntry(matrix, k, j)) > maxDIt){
        	maxDIt = (value - dm2dGetEntry(matrix, k, j));

        }
        dm2dSetEntry(matrix_aux, k, j, value);
      }
  
    if(maxDIt < maxD){
   		_nextIt[id-1] = 1;
    }

    waitBarreira(i, ntasks,id);
    if(_Barreira->b_exit == 1)
    {
    	pthread_exit(NULL);
    }
    maxDIt = 0;
}
  return 0;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char **argv) {

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: Projecto N tEsq tSup tDir tInf iteracoes num_tarefas maxD\n\n");
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
  maxD = parse_integer_or_exit(argv[8], "MaxD");


  fprintf(stderr, "\nArgumentos:\n"
  " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d ntasks=%d MaxD=%.1f\n",
  N, tEsq, tSup, tDir, tInf, iteracoes, ntasks, maxD);


  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || (N % ntasks) != 0 || maxD < 0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, numero de tasks multiplo de N, MaxD >= 0\n\n");
    return 1;
  }

  int i;
  int linhasFatia = N / ntasks;     //Numero de linhas interiores de uma fatia


  _nextIt = (int*) malloc(ntasks * sizeof(int));

  for (int i = 0; i < ntasks; i++)
  	_nextIt[i] = 0;

  pthread_t *tid;
  tid = (pthread_t*) malloc(ntasks * sizeof(pthread_t));

  _TaskArg = (TaskArg) malloc(sizeof(struct taskArg) * ntasks); // Aloca memoria para os argumentos das slaves

  matrix = dm2dNew(N + 2, N + 2);
  matrix_aux = dm2dNew(N + 2, N + 2);
  initBarreira(ntasks);

  


  if (matrix == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a matriz.\n\n");
    return -1;
  }

  //Inicializa a matriz
  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N + 1, tInf);
  dm2dSetColumnTo(matrix, 0, tEsq);
  dm2dSetColumnTo(matrix,N + 1, tDir);
  dm2dCopy(matrix_aux, matrix);

  for (i = 0; i < ntasks; i++) {          //Cria Tarefas e inicializa os seus argumentos
    _TaskArg[i]._id = i+1; 
    _TaskArg[i]._iteracoes = iteracoes;
    _TaskArg[i]._ntasks = ntasks; 
    _TaskArg[i]._N = N;
    _TaskArg[i]._linhasFatia = linhasFatia;
    _TaskArg[i]._maxD = 0;
    if (pthread_create (&tid[i], NULL, fnThread, &_TaskArg[i]) != 0){
      printf("Erro ao criar tarefa.\n");
      return 1;
    }
  }


  for (i = 0; i < ntasks; i++) {
    if (pthread_join (tid[i], NULL)) {
      printf("Erro ao esperar por tarefa.\n");
      return 2;
    }
  }


  dm2dPrint(matrix);
  free(tid);
  free(_TaskArg);
  dm2dFree(matrix);
  destroyBarreira();

  return 0;
}
