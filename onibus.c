/*
*		Grupo 9
*
*		Bruno Cepeda Henriques		8956752
*		Camila Stenico dos Santos	8530952
*		Pedro de Paiva Fernandez	8910389
*/

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
	pthread_mutex_t lock;
} fifo_t;

void fifo_init(fifo_t *lista){	// Inicializa as variaveis da fila
	pthread_mutex_init(&lista->lock, NULL);
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

void clear_fifo(fifo_t *fifo){
	node_t *temp_node1 = fifo->first;
	node_t *temp_node2 = temp_node1;
	fifo->first = NULL;
	while(temp_node1){
		temp_node1 = temp_node2->prox;
		free(temp_node2);
		temp_node2 = temp_node1;
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

/*As structs abaixo iniciadas com "args_" visam
 * simplificar a criacao e admnistraaco das threads 
 *pois as tornam funcoes com um unico argumento.  */

typedef struct args_passageiro {
	int id;                 // PID da thread
	int inicio;				// Ponto inicial do passageiro
	int destino;			// Destino do passageiro
} args_passageiro_t;

typedef struct args_carro {
	int id;                 // PID da thread
	int ponto_inicial;       // Ponto inicial deste onibus
	int ponto_atual;		// Ponto atual do onibus
} args_carro_t;

typedef struct args_ponto {
	int id;                 // PID da thread
} args_ponto_t;

/*As structs abaixo sao colecoes de informacoes
 * sobre cada ponto, carro e passageiro, visando
 * facilitar o acesso destas */

int num_pontos = 0;		// Numero de pontos ja criados
int num_carros = 0;		// Numero de carros ativos
pthread_mutex_t lock_num_pontos; 	// Mutex para garantir que so um processo modifique este valor por vez
pthread_mutex_t lock_num_carros;	// Mutex para garantir que so um processo modifique este valor por vez

typedef struct ponto {
	pthread_t thread_ponto;	// Localizador da thread
	int id;					// ID do ponto
	pthread_mutex_t lock;	// Lock do ponto, garante que somente um onibus esta em um ponto qualquer
	fifo_t fila;			// Fila relacionada ao ponto
	int id_carro;			// ID do carro parado no ponto
} ponto_t;

typedef struct passageiro {
	pthread_t thread_passageiro;	// Localizador da thread
	int id;					// ID do passageiro
	int inicio;				// Ponto inicial do passageiro
	int destino;			// Destino do passageiro
	int assento;			// Assento que o passageiro ocupa (-1 significa nenhum)
	int id_carro;			// ID do carro em que o passageiro esta (-1 significa nenhum)
	char estado;			// Estado do passageiro ('w'-esperando onibus, 'e'-entrando no onibus, 'i'-inativo, 
							//										's'-saindo do onibus, 'v'-viajando no onibus)
} passageiro_t;

typedef struct carro {
	pthread_t thread_carro;	// Localizador da thread
	pthread_mutex_t passageiro_em_movimento;	// Indica se ha um passageiro entrando ou saindo deste onibus
	pthread_cond_t pode_rodar;					// Indica quando o onibus pode sair do ponto (se todos os passageiros desceram)
	pthread_mutex_t cond_lock;					// Mutex para utilizar a condicao acima
	int id;					// ID do carro
	int *assentos;			// Vetor de assentos do carro
	int assentos_disponiveis;	// Numero de assentos disponiveis
	int ponto_inicial;		// Ponto em que o onibus comecou
	int ponto_atual;		// Ponto em que o onibus se localiza (-1 significa nenhum)
	int *pontos_a_parar;	// Vetor com o numero de quantos passageiros querem descer em cada ponto;
} carro_t;
// A segur, estao vetores que representam os pontos, passageiros e carros
ponto_t *info_pontos;
passageiro_t *info_passageiros;
carro_t *info_carros;

/* As funcoes abaixo representam as linhas de codigo 
 * que as threads irao executar quando ativas*/

void *passageiro(void *args){
	args_passageiro_t *temp = (args_passageiro_t *) args;
	passageiro_t *info = info_passageiros+temp->id;
	info->id = temp->id;
	info->inicio = temp->inicio;
	info->destino = temp->destino;
	info->id_carro = -1;
	info->estado = 'w';		// 'w' de waiting, ou seja, esperando o onibus
	info->assento = -1;

	pthread_mutex_lock(&info_pontos[info->inicio].fila.lock);
	fifo_insert(info->id, &info_pontos[info->inicio].fila);
	pthread_mutex_unlock(&info_pontos[info->inicio].fila.lock);

	while(info->estado != 'v'){
		pthread_yield();
	}

	printf("Passageiro %d: assento %d\n",info->id, info->assento);

	while(info->estado == 'v'){
		if(info_carros[info->id_carro].ponto_atual == info->destino){
			pthread_mutex_lock(&info_carros[info->id_carro].passageiro_em_movimento);
			info->estado = 's';
			info_carros[info->id_carro].assentos_disponiveis--;
			info_carros[info->id_carro].assentos[info->assento] = -1;
			info_carros[info->id_carro].pontos_a_parar[info->destino]--;
			info->estado = 'i';
			pthread_mutex_unlock(&info_carros[info->id_carro].passageiro_em_movimento);
			break;
		}
	}

	usleep(100000 * ((rand()%10)+1) );	// passageiro dorme por tempo aleatorio entre 0.1 e 1 segundos

	//printf("Oi, sou o passageiro numero %d, meu inicio e %d e meu destino, %d\n", pid, inicio, destino);
	return NULL;
}

void *carro(void *args){

	args_carro_t *temp = (args_carro_t *) args;
	carro_t *info = info_carros+temp->id;
	info->id = temp->id;
	int ponto_temp = temp->ponto_inicial;
	int i;
	info->assentos_disponiveis = A;
	info->assentos = calloc(A,sizeof(int));
	info->pontos_a_parar = calloc(S,sizeof(int));

	pthread_mutex_lock(&lock_num_carros);
	num_carros++;
	pthread_mutex_unlock(&lock_num_carros);

	for(i=0;i<A;i++){
		info->assentos[i] = -1;
	}

	while(pthread_mutex_trylock(&info_pontos[ponto_temp].lock) != 0){
		if(++ponto_temp >= S){
			ponto_temp =0;
		}
	}
	info->ponto_inicial = ponto_temp;
	info->ponto_atual = ponto_temp;

	pthread_mutex_lock(&info->cond_lock);
	while(info_pontos[info->ponto_atual].fila.size > 0 && info->assentos_disponiveis > 0){
		pthread_cond_wait(&info->pode_rodar, &info->cond_lock);

	}

	pthread_mutex_unlock(&info_pontos[ponto_temp].lock);

	printf("Onibus %d:", info->id);
	for(i=0;i<A;i++){
		printf("%d ", info->assentos[i]);
	}
	printf("\n");

	if(++ponto_temp >= S){
		ponto_temp =0;
	}
	while(pthread_mutex_trylock(&info_pontos[ponto_temp].lock) != 0){
		if(++ponto_temp >= S){
			ponto_temp =0;
		}
	}
	printf("carro %d:\tinicio:%d,\tfim:%d\n", info->id, info->ponto_inicial, ponto_temp);


	pthread_mutex_lock(&lock_num_carros);
	num_carros--;
	pthread_mutex_unlock(&lock_num_carros);

	free(info->assentos);
	free(info->pontos_a_parar);
	return NULL;
}

void *ponto(void *args){
	args_ponto_t *temp = (args_ponto_t *) args;
	ponto_t *info = info_pontos+temp->id;
	info->id = temp->id;
	info->id_carro = -1;

	pthread_mutex_lock(&lock_num_pontos);	// Garante que num_pontos so eh incrementado 1 por vez
	pthread_mutex_lock(&info->fila.lock);
	fifo_init(&info->fila);
	num_pontos++;
	pthread_mutex_unlock(&lock_num_pontos);
	pthread_mutex_unlock(&info->fila.lock);

	int i;

	while(num_carros){
		while(info->id_carro == -1){
			if(num_carros == 0){
				break;
			}
			pthread_yield();
		}

		if(num_carros == 0){
			break;
		}

		while(info_carros[info->id_carro].pontos_a_parar[info->id]){	// Espera enquanto ha passageiros para descer neste ponto
			pthread_yield();
		}

		pthread_mutex_lock(&info->fila.lock);

		int id_passageiro_temp;
		while(info->fila.size > 0 && info_carros[info->id_carro].assentos_disponiveis > 0){	// Existe pessoas na fila e vagas no onibus
			for(i=0;i<A;i++){	// Procura uma vaga no onibus
				id_passageiro_temp = info_carros[info->id_carro].assentos[i];
				if(id_passageiro_temp == -1){
					id_passageiro_temp = fifo_pop(&info->fila);
					info_passageiros[id_passageiro_temp].estado = 'e';				// Significa que o passageiro esta 'entrando'
					info_passageiros[id_passageiro_temp].id_carro = info->id_carro;	// No carro que esta parado neste ponto
					info_passageiros[id_passageiro_temp].assento = i;				// No assento i
					info_carros[info->id_carro].assentos[i] = id_passageiro_temp;
					info_carros[info->id_carro].pontos_a_parar[info->id]++;
					pthread_mutex_unlock(&info->fila.lock);
					info_passageiros[id_passageiro_temp].estado = 'v';
					break;
				}
				pthread_mutex_unlock(&info->fila.lock);
			}
			pthread_mutex_lock(&info->fila.lock);
		}
		pthread_mutex_unlock(&info->fila.lock);

		info_carros[info->id_carro].ponto_atual = -1;
		info->id_carro = -1;
		pthread_cond_signal(&info_carros[info->id_carro].pode_rodar);
	}


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
	
	int i, j;

	//Alocando memoria para threads  
	info_carros = calloc(C,sizeof(carro_t));
	info_pontos = calloc(S,sizeof(ponto_t));
	info_passageiros = calloc(P,sizeof(passageiro_t));
	//Alocando memoria para os valores inicias
	args_carro_t* args_carro_init = calloc(C,sizeof(args_carro_t));
	args_ponto_t* args_ponto_init = calloc(S,sizeof(args_ponto_t));
	args_passageiro_t* args_passageiro_init = calloc(P,sizeof(args_passageiro_t));

	int temp, err;
	
	//Iniciando threads dos pontos
	pthread_mutex_init(&lock_num_pontos, NULL);
	for(i=0;i<S;i++){
		err = 0;
		args_ponto_init[i].id = i;
		if(pthread_mutex_init(&info_pontos[i].lock,NULL)){
			printf("Erro ao inicializar um dos Mutex");
			return 4;
		}
		while(pthread_create(&info_pontos[i].thread_ponto,NULL,ponto,(void*)(args_ponto_init+i))){
			if(err >= 10){
				printf("Erro ao criar o ponto %d, saindo do programa.\n", i);
				return(3);
			}
			err++;
			printf("Erro ao criar um dos pontos, tentando novamente.\n");
		}
	}

	while(num_pontos < S){	// Garante que todos os pontos serao criados antes de gerar carros e pessoas
		pthread_yield();
	}

	//Iniciando as threads dos carros
	int *pontos_iniciais = def_pontos_inciais(S,C);

	for(i=0;i<C;i++){
		err = 0;
		args_carro_init[i].id = i;
		args_carro_init[i].ponto_inicial = pontos_iniciais[i];
		while(pthread_create(&info_carros[i].thread_carro,NULL,carro,(void*)(args_carro_init+i))){
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
		args_passageiro_init[i].destino = rand() % S;
		while(args_passageiro_init[i].destino == args_passageiro_init[i].inicio){	// Garante que o destino nao eh igual ao inicio
			args_passageiro_init[i].destino = rand() % S;
		}

		while(pthread_create(&info_passageiros[i].thread_passageiro,NULL,passageiro,(void*)(args_passageiro_init+i))){
			if(err >= 10){
				printf("Nao consegue criar o passageiro %d, saindo do programa.\n", i);
				return(3);
			}
			err++;
			printf("Erro ao criar um dos passageiros, tentando novamente.\n");
		}
	}

	for(i=0;i<S;i++){
		pthread_join(info_pontos[i].thread_ponto, NULL);
		printf("Ponto %d: ", i);
		print_fifo(info_pontos[i].fila);
		clear_fifo(&info_pontos[i].fila);
	}

	for(i=0;i<C;i++){
		pthread_join(info_carros[i].thread_carro, NULL);
		// printf("\nCarro %d:", i);
		// for(j=0;j<A;j++){
		// 	printf("%d ",info_carros[i].assentos[j]);
		// }
	}

	for(i=0;i<P;i++){
		pthread_join(info_passageiros[i].thread_passageiro, NULL);
	}
	
	//Desalocando a memoria

	free(info_carros);
	free(info_pontos);
	free(info_passageiros);

	free(args_carro_init);
	free(args_passageiro_init);
	free(args_ponto_init);

	free(pontos_iniciais);

	return 0;
}

