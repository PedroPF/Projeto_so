#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>



typedef struct args_passageiro {
    int id;
} args_passageiro_t;

typedef struct args_carro {
    int id;
} args_carro_t;

typedef struct args_ponto {
    int id;
} args_ponto_t;



void *passageiro(void *args){
    args_passageiro_t *temp = (args_passageiro_t *) args;
    int pid = temp->id;
    printf("Oi, sou o passageiro numero %d\n", pid);
    return NULL;
}

void *carro(void *args){
    args_carro_t *temp = (args_carro_t *) args;
    int pid = temp->id;
    printf("Oi, sou o carro numero %d\n", pid);
    return NULL;
}

void *ponto(void *args){
    args_ponto_t *temp = (args_ponto_t *) args;
    int pid = temp->id;
    printf("Oi, sou o ponto numero %d\n", pid);
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

    args_carro_t* args_carro_init = calloc(C,sizeof(args_carro_t));
    args_ponto_t* args_ponto_init = calloc(S,sizeof(args_ponto_t));
    args_passageiro_t* args_passageiro_init = calloc(P,sizeof(args_passageiro_t));

    int i;
    int err;
    for(i=0;i<S;i++){
        err = 0;
        args_ponto_init[i].id = i;
        while(pthread_create(pontos+i,NULL,ponto,(void*)(args_ponto_init+i))){
            if(err >= 10){
                printf("Nao consegue criar o ponto %d, saindo do programa.\n", i);
            }
            err++;
            printf("Erro ao criar um dos pontos, tentando novamente.\n");
        }
    }

    for(i=0;i<C;i++){
        err = 0;
        args_carro_init[i].id = i;
        while(pthread_create(carros+i,NULL,carro,(void*)(args_carro_init+i))){
            if(err >= 10){
                printf("Nao consegue criar o carro %d, saindo do programa.\n", i);
            }
            err++;
            printf("Erro ao criar um dos carros, tentando novamente.\n");
        }
    }

    for(i=0;i<P;i++){
        err = 0;
        args_passageiro_init[i].id = i;
        while(pthread_create(passageiros+i,NULL,passageiro,(void*)(args_passageiro_init+i))){
            if(err >= 10){
                printf("Nao consegue criar o passageiro %d, saindo do programa.\n", i);
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

    return 0;
}

