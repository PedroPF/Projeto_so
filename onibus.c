#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>



typedef struct args_passageiro {
    int id;
} args_passageiro_t;

typedef struct args_carros {
    int id;
} args_carros_t;

typedef struct args_ponto {
    int id;
} args_ponto_t;



void *passageiro(){

    return NULL;
}

void *carros(){

    return NULL;
}

void *ponto(){

    return NULL;
}


int main(int argc,char *argv[]){

    int S, P, C, A;

    if( argc != 5 ){
        printf("Sintaxe invalida!\n");
        return 1;
    }

    S = atoi(argv[1]);
    P = atoi(argv[2]);
    C = atoi(argv[3]);
    A = atoi(argv[4]);
    printf("S:%d P:%d C:%d A:%d\n", S, P, C, A);

    pthread_t *carros = calloc(C,sizeof(pthread_t));
    pthread_t *pontos = calloc(S,sizeof(pthread_t));
    pthread_t *passageiros = calloc(P,sizeof(pthread_t));

    int i;
    int err;
    for(i=0;i<S;i++){
        err = 0;
        while(pthread_create(pontos+i,NULL,ponto,NULL)){
            if(err >= 10){
                printf("Nao consegue criar o ponto %d, saindo do programa.\n", i);
            }
            err++;
            printf("Erro ao criar um dos Pontos, tentando novamente.\n");
        }
    }

    for(i=0;i<C;i++){
        err = 0;
        while(pthread_create(carros+i,NULL,carro,NULL)){
            if(err >= 10){
                printf("Nao consegue criar o ponto %d, saindo do programa.\n", i);
            }
            err++;
            printf("Erro ao criar um dos Pontos, tentando novamente.\n");
        }
    }

    for(i=0;i<P;i++){
        err = 0;
        while(pthread_create(passageiros+i,NULL,passageiro,NULL)){
            if(err >= 10){
                printf("Nao consegue criar o ponto %d, saindo do programa.\n", i);
            }
            err++;
            printf("Erro ao criar um dos Pontos, tentando novamente.\n");
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

    return 0;
}

