#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Variáveis Globais
int world_rank, world_size;

int** Gera_Matriz(int linha, int col){
    int **matriz;
    matriz = (int**) malloc(col * sizeof(int*));
    for(int i = 0; i < col; i++){
        matriz[i] = (int*) malloc(linha * sizeof(int));
    }
    return matriz;

}

int** Preenche_Matriz(int** matriz, int linha, int col){
    for(int i = 0; i < col; i++){
        for(int j = 0; j < linha; j++){
            matriz[i][j] = (rand() % 10) + 1;
        }
    }
    return matriz;

}

void Imprime_Matriz(int** matriz, int linha, int col){
    for(int i = 0; i < linha; i++){
        for(int j = 0; j < col; j++){
            printf("%d ", matriz[j][i]);
        }
        printf("\n");
    }


}

void Distribui_Tarefas(int index){
    int **m1, **m2;
    if(index == 0){ //Processo de contrle
            //Gera os valores aleatórios
            int p = (rand()%10) + 1;
            int l = (rand()%10) + 1;
            int c = (rand()%10) + 1;
            //Gera as matrizes
            m1 = Gera_Matriz(l,p);
            m2 = Gera_Matriz(p,c);
            //Preenche as matrizes
            m1 = Preenche_Matriz(m1,l,p);
            m2 = Preenche_Matriz(m2,p,c);
            //Imprime as matrizes
            printf("Imprimindo as matrizes\n");
            printf("--------------\n");
            Imprime_Matriz(m1, l, p);
            printf("--------------\n");
            Imprime_Matriz(m2, p, c);
            printf("--------------\n");
            //Gera valores auxiliares
            //t = nº de threads que farão multiplicação. world_size = nº total de threads, incluindo a de controle
            int t = world_size - 1;
            //Numero de multiplicações a serem feitas
            int mult = l*c;
            printf("%d multiplicações de vetores\n", mult);
            //Quantas multiplicações de vetores cada thread vai fazer
            int quantas_vezes = mult / t;
            //Quantas multiplicações além das distribuidas igualmente
            int quantas_extras = mult % t;
            //Salvando o numero de multiplicações cada thread vai fazer
            int threads[t];
            for(int i = 0; i < t; i++){
                threads[i] = quantas_vezes;
            }
            for(int i = 0; i < quantas_extras; i++){
                threads[i]++;
            }
            //Passando as informaçoes assicronamente para os processos
            MPI_Request r;
            for(int i = 1, j  = 0; i < world_size, j < t; i++, j++){
                //MPI_Isend(&threads[j], 1, MPI_INT, i, 0, MPI_COMM_WORLD,&r);
                MPI_Send(&threads[j], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            //Passando o tamanho dos vetores
            for(int i = 1, j  = 0; i < world_size, j < t; i++, j++){
                if(threads[j] != 0)
                {
                //    MPI_Isend(&p, 1, MPI_INT, i, 0, MPI_COMM_WORLD,&r);
                    MPI_Send(&p, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("Tam enviado para %d\n",i);

                }
            }
            //
            int aux_l = 0, aux_c = 0;
            for(int i = 0; i < t; i++){
                for(int j = 0; j < threads[i]; j++){
                    //Separa quais vetores serão enviados para cada processo
                    int linha[p], coluna[p];
                    for(int a = 0; a < p; a++){
                        linha[a] = m1[a][aux_l];
                        coluna[a] = m2[aux_c][a];
                    }
                    printf("\n");
                    //Envia os dados
                    if(aux_c < c){  //Se aux_c == c entao tem que mudar de linha na matriz
                        MPI_Send(&linha, p, MPI_INT, i+1, 0, MPI_COMM_WORLD);
                        MPI_Send(&coluna, p, MPI_INT, i+1, 0, MPI_COMM_WORLD);
                        printf("Valores enviados para %d",i+1);
                        aux_c++;
                    }
                    else{
                        //Altera as linhas e colunas
                        aux_c = 0;
                        aux_l++;
                    }
                }
            }
            //Recebe os valores de volta
            int** m_result = Gera_Matriz(l,c);
            aux_l = 0; aux_c = 0;
            for(int i = 0; i < t; i++){
                if(threads[i] != 0){    //Não deixa esperando por processos vazios
                    int retorno[threads[i]];
                    MPI_Recv(&retorno, threads[i], MPI_INT, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    //Coloca na Matriz
                    for(int j = 0; j < threads[i]; j++){
                        m_result[aux_c][aux_l] = retorno[j];
                        aux_c++;
                        if(aux_c == c){
                            aux_c = 0;
                            aux_l++;
                        }
                    }
                }
            }
            Imprime_Matriz(m_result, l, c);

        }
        //------------------------------------
        //Threads para multiplicação
    else{

        int vezes, tam;
        //Recebe quantas vezes ela agirá
        MPI_Recv(&vezes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(vezes == 0)  //Se ela nao for multiplicar, fecha o programa
            return;     //Return ou Exit?
        //Recebe o tamanho dos vetores
        MPI_Recv(&tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Tam recebido em %d\n",index);
        //Recebe os dados
        int v1[vezes][tam], v2[vezes][tam];
        int resultados[vezes];
        MPI_Request requests[vezes];
        for(int i = 0; i < vezes; i++){
            /*printf("%d: %d e %d\n", index, vezes, tam);
            MPI_Recv(v1[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(v2[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            */
            MPI_Irecv(&v1[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[i]);
            MPI_Irecv(&v2[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[i]);
            printf("Valores recebidos em %d\n",index);
        }
        //Multiplica os vetores
        for(int i = 0; i < vezes; i++){
            resultados[i] = 0;
            MPI_Status s;
            MPI_Wait(&requests[i],&s);
            for(int aux = 0; aux < tam; aux++){
                resultados[i] += v1[i][aux] * v2[i][aux];
            }
         }
        //Retorna os valores para a thread inicial
        MPI_Request request;
        MPI_Isend(&resultados, vezes, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    }
}


int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // Get the name of the processor
    //char processor_name[MPI_MAX_PROCESSOR_NAME];
    //int name_len;
    //MPI_Get_processor_name(processor_name, &name_len);

    srand(time(NULL)); //Altera o seed da função srand

    Distribui_Tarefas(world_rank);

    // Finalize the MPI environment.
    MPI_Finalize();
}

