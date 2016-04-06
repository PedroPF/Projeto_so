#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>
#include <time.h>


typedef int elem;

typedef struct node {
	elem e;
	struct node *prox;
} node_t;

typedef struct fifo {
	int size;
	node_t *first;
} fifo_t;

void fifo_init(fifo_t *lista){	// Inicializa as variaveis da fila
	lista->size = 0;
	lista->first = NULL;
}

int fifo_insert(elem e, fifo_t *fifo){	// Insere um elemento no fim da fila
	node_t *new = malloc(sizeof(node_t));
	if(!new){
		printf("Nao foi possivel alocar memoria!\n");
		return 0;
	}
	new->e = e;
	fifo->size++;
	if(fifo->first == NULL){
		fifo->first = new;
	} else {
		new->prox = fifo->first;
		fifo->first = new;
	}
	return 1;
}

elem fifo_pop(fifo_t *fifo){	// Remove o primeiro elemento da fila e o retorna
	elem temp;
	node_t *temp_node;
	if(fifo->size == 0){
		return -1;
	} else if(fifo->size == 1){
		temp = fifo->first->e;
		free(fifo->first);
		fifo->first = NULL;
		fifo->size--;
		return temp;
	} else {
		temp_node = fifo->first;
		while(temp_node->prox->prox){
			temp_node = temp_node->prox;
		}
		temp = temp_node->prox->e;
		free(temp_node->prox);
		temp_node->prox = NULL;
		fifo->size--;
		return temp;
	}
}

void print_fifo(fifo_t fifo){	// Imprime a fila
	node_t *temp_node = fifo.first;
	while(temp_node){
		printf("%d ",temp_node->e);
		temp_node = temp_node->prox;
	}
	printf("\n");
}

int S, P, C, A; //Número de Paradas(S), Passageiros(P), Carros(C), Assentos(A).

//Vetores para guardar threads  
pthread_t *carros;
pthread_t *pontos;
pthread_t *passageiros;
//Vetor dos mutex que garantem que apenas um onibus ocupe um ponto
pthread_mutex_t *locks_pontos;
//Vetor com as filas de cada ponto
fifo_t *filas_passageiro;
//Vetor com estados de cada passageiro
char *estados_passageiro;


/*As structs abaixo iniciadas com "args_" visam
 * simplificar a criacao e admnistraaco das threads 
 *pois as tornam funcoes com um unico argumento.  */

typedef struct args_passageiro {
	int id;                 // PID da thread
	int S;					// Numero de pontos
} args_passageiro_t;

typedef struct args_carro {
	int id;                 // PID da thread
	int ponto_incial;       // Ponto inicial deste onibus
} args_carro_t;

typedef struct args_ponto {
	int id;                 // PID da thread
} args_ponto_t;

/* As funcoes abaixo representam as linhas de codigo 
 * que as threads irao executar quando ativas*/

void *passageiro(void *args){
	args_passageiro_t *temp = (args_passageiro_t *) args;
	int pid = temp->id;
	int inicio = rand() % temp->S;
	int destino = rand() % temp->S;;
	while(destino == inicio){
		destino = rand() % temp->S;
	}
	estados_passageiro[pid] = 'w';	// w de waiting, ou seja, passageiro esperando um onibus
	fifo_insert(pid, filas_passageiro+inicio);
	//printf("Oi, sou o passageiro numero %d, meu inicio e %d e meu destino, %d\n", pid, inicio, destino);
	return NULL;
}

void *carro(void *args){
	args_carro_t *temp = (args_carro_t *) args;
	int pid = temp->id;
	int ponto_atual = temp->ponto_incial;
	int ponto_inicial;
	int ocupantes[A];

	while(pthread_mutex_trylock(locks_pontos+ponto_atual) != 0){
		if(++ponto_atual >= S){
		ponto_atual =0;
		}
	}
	ponto_inicial = ponto_atual;
	pthread_mutex_unlock(locks_pontos+ponto_atual);
	if(++ponto_atual >= S){
		ponto_atual =0;
	}
	while(pthread_mutex_trylock(locks_pontos+ponto_atual) != 0){
		if(++ponto_atual >= S){
		ponto_atual =0;
		}
	}
	// printf("carro %d:\tinicio:%d,\tfim:%d\n", pid, ponto_inicial, ponto_atual);
	return NULL;
}

void *ponto(void *args){
	args_ponto_t *temp = (args_ponto_t *) args;
	int pid = temp->id;

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
	carros = calloc(C,sizeof(pthread_t));
	pontos = calloc(S,sizeof(pthread_t));
	passageiros = calloc(P,sizeof(pthread_t));
	//Alocando memoria para o mutex que garante que apenas um onibus ocupe um ponto
	locks_pontos = calloc(S, sizeof(pthread_mutex_t));
	//Alocando memoria para os valores inicias
	args_carro_t* args_carro_init = calloc(C,sizeof(args_carro_t));
	args_ponto_t* args_ponto_init = calloc(S,sizeof(args_ponto_t));
	args_passageiro_t* args_passageiro_init = calloc(P,sizeof(args_passageiro_t));
	//Alocando memoria e inicializando a fila de cada ponto
	filas_passageiro = calloc(S,sizeof(fifo_t));
	int i;
	for(i = 0;i < S;i++){
		fifo_init(filas_passageiro+i);
	}
	//Alocando memoria para o vetor de estados dos passageiros
	estados_passageiro = calloc(P,sizeof(char));

	int temp, err;
	
	//Iniciando threads dos pontos
	for(i=0;i<S;i++){
		err = 0;
		args_ponto_init[i].id = i;
		if(pthread_mutex_init(locks_pontos+i,NULL)){
			printf("Erro ao inicializar um dos Mutex");
			return 4;
		}
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
		args_carro_init[i].ponto_incial = pontos_iniciais[i];
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
		args_passageiro_init[i].S = S;

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
	
	free(locks_pontos);

	free(args_carro_init);
	free(args_passageiro_init);
	free(args_ponto_init);

	free(pontos_iniciais);

	free(filas_passageiro);
	free(estados_passageiro);

	return 0;
}

