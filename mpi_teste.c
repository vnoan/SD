#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM_MAT 1500
//Variáveis Globais
int world_rank, world_size;
double t_inicio, t_fim;


float** Gera_Matriz(int linha, int col){
    float **matriz;
    matriz = (float**) malloc(col * sizeof(float*));
    for(int i = 0; i < col; i++){
        matriz[i] = (float*) malloc(linha * sizeof(float));
    }
    return matriz;

}

float** Preenche_Matriz(float** matriz, int linha, int col){
    for(int i = 0; i < col; i++){
        for(int j = 0; j < linha; j++){
            matriz[i][j] = (rand() % 10) + 1;
        }
    }
    return matriz;
}

void Libera_Matriz(float** matriz, int col){
    for(int i = 0; i < col; i++){
        free(matriz[i]);
    }
}

void Imprime_Matriz(float** matriz, int linha, int col){
    for(int i = 0; i < linha; i++){
        for(int j = 0; j < col; j++){
            printf("%.1f ", matriz[j][i]);
        }
        printf("\n");
    }
}

void Distribui_Tarefas(int index){
    float **m1, **m2;

    if(index == 0){ //Processo de controle
        //Gera os valores aleatórios
        int p = TAM_MAT;//(rand()%10) + 1;
        int l = TAM_MAT;//(rand()%10) + 1;
        int c = TAM_MAT;//(rand()%10) + 1;
        //Gera as matrizes
        m1 = Gera_Matriz(l,p);
        m2 = Gera_Matriz(p,c);
        //Preenche as matrizes
        m1 = Preenche_Matriz(m1,l,p);
        m2 = Preenche_Matriz(m2,p,c);
        //Imprime as matrizes
        //Gera valores auxiliares
        int t = world_size - 1; //-1 para desconsiderar o processo de controle
        //Numero de multiplicações a serem feitas
        int mult = l*c;
        printf("%d multiplicações de vetores\n\n", mult);
        //Quantas multiplicações de vetores cada thread vai fazer
        int quantas_vezes = mult / t;
        //Quantas multiplicações além das distribuidas igualmente
        int quantas_extras = mult % t;
        //Salvando o numero de multiplicações cada thread vai fazer
        int *threads = (int*)malloc(t*sizeof(int));
        for(int i = 0; i < t; i++){
            threads[i] = quantas_vezes;
        }
        for(int i = 0; i < quantas_extras; i++){
            threads[i]++;
        }
        //Anota o tempo de inicio
        t_inicio = MPI_Wtime();
        //Passando as informaçoes assicronamente para os processos
        //MPI_Request r;
        for(int i = 1, j  = 0; i < world_size, j < t; i++, j++){
            //Passando o numero de vezes que cada processo agirá
            MPI_Send(&threads[j], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        for(int i = 1, j  = 0; i < world_size, j < t; i++, j++){
            if(threads[j] != 0)
            {   //Passando o tamanho dos vetores
                MPI_Send(&p, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
        //Enviando os dados
        int aux_l = 0, aux_c = 0;
        //MPI_Request *r1 = (MPI_Request*) malloc (t*sizeof(MPI_Request));
        //MPI_Request *r2 = (MPI_Request*) malloc (t*sizeof(MPI_Request));
        float *linha = (float*)malloc(p*sizeof(float));
        float *coluna = (float*)malloc(p*sizeof(float));
        for(int i = 0; i < t; i++){
            for(int j = 0; j < threads[i]; j++){
                //Separa quais vetores serão enviados para cada processo
                for(int a = 0; a < p; a++){
                    linha[a] = m1[a][aux_l];
                    coluna[a] = m2[aux_c][a];
                }
                //Faz o controle da posição na matriz
                aux_c++;
                if(aux_c >= c){
                    aux_c = 0;
                    aux_l++;
                }
                //Envia os dados
                MPI_Send(linha, p, MPI_FLOAT, i+1, 0, MPI_COMM_WORLD);
                MPI_Send(coluna, p, MPI_FLOAT, i+1, 0, MPI_COMM_WORLD);
                //Enviar os dados de maneira assincrona é mais vantajoso. Porem nao esta funcionando
                //Quando usa o MPI_Isend não pode alterar o linha e coluna até a operação ser completada
                //MPI_Isend(&linha, p, MPI_INT, i+1, j, MPI_COMM_WORLD,&r1[i]);   //r1 e r2 serão sobrescritos
                //MPI_Isend(&coluna, p, MPI_INT, i+1, j, MPI_COMM_WORLD,&r2[i]);  //valendo apenas o ultimo request enviado

            }
        }

        //Recebe os valores de volta
        float** m_result = Gera_Matriz(l,c);
        aux_l = 0; aux_c = 0;
        for(int i = 0; i < t; i++){
            if(threads[i] != 0){    //Não deixa esperando por processos vazios
                int retorno[threads[i]];
                /*
                MPI_Wait(&r1[i],MPI_STATUS_IGNORE);
                MPI_Wait(&r2[i],MPI_STATUS_IGNORE);
                */
                MPI_Recv(&retorno, threads[i], MPI_FLOAT, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
        //Imprime_Matriz(m_result, l, c);
        free(linha);
        free(coluna);

    }
//----------------------------------------------------------
//Threads para multiplicação
    else{

        int vezes, tam;
        //Recebe quantas vezes ela agirá
        MPI_Recv(&vezes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(vezes == 0)          //Se ela nao for multiplicar, fecha o programa
            return;             //Return ou Exit?
        //Recebe o tamanho dos vetores
        MPI_Recv(&tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //Recebe os dados
        float **v1, **v2;
        v1 = (float**)malloc(vezes*sizeof(float*));
        v2 = (float**)malloc(vezes*sizeof(float*));
        for(int i = 0; i < vezes; i++){
            v1[i] = (float*)malloc(tam*sizeof(float));
            v2[i] = (float*)malloc(tam*sizeof(float));
        }
        //MPI_Request requests[vezes];
        for(int i = 0; i < vezes; i++){
            MPI_Recv(v1[i], tam, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(v2[i], tam, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            /*
            MPI_Irecv(v1[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[i]);
            MPI_Irecv(v2[i], tam, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[i]);
            printf("Valores recebidos em %d\n",index);
            */

        }
        //int resultados[vezes];
        float *resultados = (float*)malloc(vezes*sizeof(float));
        //Multiplica os vetores
        for(int i = 0; i < vezes; i++){
            resultados[i] = 0;
            //MPI_Status s;
            //MPI_Wait(&requests[i],&s);
            for(int aux = 0; aux < tam; aux++){
                resultados[i] += v1[i][aux] * v2[i][aux];
            }
         }
        //Retorna os valores para a thread inicial
        /*MPI_Request request;
        MPI_Isend(&resultados, vezes, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
        */
        MPI_Send(resultados, vezes, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
        Libera_Matriz(v1,vezes);
        Libera_Matriz(v2,vezes);

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

    t_fim = MPI_Wtime();

    if(world_rank == 0)
        printf("Tamanho: %d\tTempo de execução: %f segundos\n",TAM_MAT, t_fim - t_inicio);

    // Finalize the MPI environment.
    MPI_Finalize();
}

