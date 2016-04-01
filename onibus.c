#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

int S, P, C, A; //Número de Paradas(S), Passageiros(P), Carros(C), Assentos(A).

char running = 0;

/*As structs abaixo iniciadas com "args_" visam
 * simplificar a criacao e admnistraaco das threads 
 *pois as tornam funcoes com um unico argumento.  */

typedef struct args_passageiro {
  int id;                 // PID da thread
  int inicio;             // Inicio do percurso de cada passageiro
  int destino;            // Destino final do passageiro 
  pthread_t *pontos;      // Vetor com as threads dos pontos
} args_passageiro_t;

typedef struct args_carro {
  int id;                 // PID da thread
  int A;                  // Numero de assentos
  int ponto_incial;       // Ponto inicial deste onibus
  pthread_t *pontos;      // Vetor com as threads dos pontos
  pthread_mutex_t **locks_pontos;
} args_carro_t;

typedef struct args_ponto {
  int id;                 // PID da thread
  pthread_mutex_t *lock;
} args_ponto_t;

/* As funcoes abaixo representam as linhas de codigo 
 * que as threads irao executar quando ativas*/

void *passageiro(void *args){
  args_passageiro_t *temp = (args_passageiro_t *) args;
  int pid = temp->id;
  int inicio = temp->inicio;
  int destino = temp->destino;
  //printf("Oi, sou o passageiro numero %d, meu inicio e %d e meu destino, %d\n", pid, inicio, destino);
  return NULL;
}

void *carro(void *args){
  args_carro_t *temp = (args_carro_t *) args;
  int pid = temp->id;
  int ponto_atual = temp->ponto_incial;
  int ponto_inicial;
  pthread_mutex_t **locks_pontos = temp->locks_pontos;

  while(pthread_mutex_trylock((*locks_pontos)+ponto_atual) != 0){
    if(++ponto_atual >= S){
      ponto_atual =0;
    }
  }
  ponto_inicial = ponto_atual;
  pthread_mutex_unlock((*locks_pontos)+ponto_atual);
  if(++ponto_atual >= S){
    ponto_atual =0;
  }
  while(pthread_mutex_trylock((*locks_pontos)+ponto_atual) != 0){
    if(++ponto_atual >= S){
      ponto_atual =0;
    }
  }
  printf("carro %d:\tinicio:%d,\tfim:%d\n", pid, ponto_inicial, ponto_atual);
  return NULL;
}

void *ponto(void *args){
  args_ponto_t *temp = (args_ponto_t *) args;
  int pid = temp->id;
  pthread_mutex_t *lock = temp->lock;
  //printf("Oi, sou o ponto numero %d\n", pid);
  return NULL;
}

/* Define todos os pontos iniciais aleatoriamente 
 * e devolve uma lista deles */

int *def_pontos_inciais(int num_pontos, int num_onibus){
  int *lista = calloc(num_onibus, sizeof(int));
  int i, j;
  int temp;
  srand(time(NULL));

  for(i=0;i<num_onibus;i++){
    temp = rand() % num_pontos;
    for(j=0;j<i;j++){
      if(temp == lista[j]){
        temp = rand() % num_pontos;
        j = -1;
      }
    }
    lista[i] = temp;
  }
  return lista;
}

int main(int argc,char *argv[]){
 //Recebe os argumentos do programa e trata erros 
  if( argc != 5 ){
    printf("Sintaxe invalida!\n");
    return 1;
  }
  S = atoi(argv[1]);
  C = atoi(argv[2]);
  P = atoi(argv[3]);
  A = atoi(argv[4]);
  if ( C > S || P < A || A < C ){
    printf("Valores nao validos!\n");
    return 2;
  }
  printf("S:%d C:%d P:%d A:%d\n", S, C, P, A);
//Alocando memoria para threads  
  pthread_t *carros = calloc(C,sizeof(pthread_t));
  pthread_t *pontos = calloc(S,sizeof(pthread_t));
  pthread_t *passageiros = calloc(P,sizeof(pthread_t));
//Alocando memoria para o mutex que garante a exclusao mutua entre pontos
  pthread_mutex_t *locks_pontos = calloc(S, sizeof(pthread_mutex_t));
//Alocando memoria para os valores inicias
  args_carro_t* args_carro_init = calloc(C,sizeof(args_carro_t));
  args_ponto_t* args_ponto_init = calloc(S,sizeof(args_ponto_t));
  args_passageiro_t* args_passageiro_init = calloc(P,sizeof(args_passageiro_t));

//Iniciando threads dos pontos
  int i, temp;
  int err;
  for(i=0;i<S;i++){
    err = 0;
    args_ponto_init[i].id = i;
    args_ponto_init[i].lock = locks_pontos+i;
    while(pthread_create(pontos+i,NULL,ponto,(void*)(args_ponto_init+i))){
      if(err >= 10){
        printf("Erro ao criar o ponto %d, saindo do programa.\n", i);
        return(3);
      }
      err++;
      printf("Erro ao criar um dos pontos, tentando novamente.\n");
    }
  }
//Iniciando as threads dos carros
  int *pontos_iniciais = def_pontos_inciais(S,C);

  for(i=0;i<C;i++){
    err = 0;
    args_carro_init[i].id = i;
    args_carro_init[i].A = A;
    args_carro_init[i].ponto_incial = pontos_iniciais[i];
    args_carro_init[i].pontos = pontos;
    args_carro_init[i].locks_pontos = &locks_pontos;
    while(pthread_create(carros+i,NULL,carro,(void*)(args_carro_init+i))){
      if(err >= 10){
        printf("Nao consegue criar o carro %d, saindo do programa.\n", i);
        return(3);
      }
      err++;
      printf("Erro ao criar um dos carros, tentando novamente.\n");
    }
  }
//Iniciar a thread dos passageiros
  for(i=0;i<P;i++){
    err = 0;
    args_passageiro_init[i].id = i;
    args_passageiro_init[i].inicio = rand() % S;

    do{
      temp = rand() % S;
    } while(temp == args_passageiro_init[i].inicio);
    args_passageiro_init[i].destino = temp;

    while(pthread_create(passageiros+i,NULL,passageiro,(void*)(args_passageiro_init+i))){
      if(err >= 10){
        printf("Nao consegue criar o passageiro %d, saindo do programa.\n", i);
        return(3);
      }
      err++;
      printf("Erro ao criar um dos passageiros, tentando novamente.\n");
    }
  }

  for(i=0;i<S;i++){
    pthread_join(pontos[i], NULL);
  }

  for(i=0;i<C;i++){
    pthread_join(carros[i], NULL);
  }

  for(i=0;i<P;i++){
    pthread_join(passageiros[i], NULL);
  }
//Desalocando a memoria
  free(carros);
  free(pontos);
  free(passageiros);

  free(args_carro_init);
  free(args_passageiro_init);
  free(args_ponto_init);

  free(pontos_iniciais);

  return 0;
}

