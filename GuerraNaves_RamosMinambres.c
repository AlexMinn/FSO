//Javier Ramos - Alejandro Minambres

//TODO antes de atoi comprobar que todo son numeros

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

// Variables Compartidas
char* Buffer;
int posicion_escritura;
int posicion_lectura;

// Semaforos
sem_t espacio_buffer;
sem_t haydatos;
sem_t lectura_buffer;
sem_t nave_fin;

// Structs
struct General_Data
{
    char* fichero_entrada;
    char* fichero_salida;    
    int buffersize;
    int numnaves; 
    int caracteres_correctos;
    int caracteres_incorrectos;
    int caracteres_leidos;
};

struct Naves_Data
{
    int* identificadores;
    int numnaves;
    int buffersize;
    int* hit;
    int* miss;
    int* health; 
    int* puntuacion;
    bool* end;
} datos_naves;



// Hilo Disparador 
void* disparador(void* arg)
{
    struct General_Data * datos_disparador;
    datos_disparador = (struct General_Data*)arg;

    //Variables locales    
    FILE *fichero_entrada = fopen(datos_disparador -> fichero_entrada, "r");
    char caracter;
    bool caracter_correcto;
    int correctos = 0;
    int incorrectos = 0;
    int leidos = 0;

    while ((caracter = fgetc(fichero_entrada)) != EOF) 
    {
        caracter_correcto = false;
        leidos ++;

        if(caracter == '*')
        {
            caracter_correcto = true;
        }
        else if (caracter == ' ')
        {            
            caracter_correcto = true;
        }
        else if (caracter == 'b')
        {
            caracter = fgetc(fichero_entrada);  // El número de vidas
            if ((caracter == '1') || (caracter == '2') || (caracter == '3'))
            {
                caracter_correcto = true;
            }
            else
            {
                incorrectos ++;
            }
        }
        if (caracter_correcto)
        {
            correctos ++;

            int pos = posicion_escritura;
            posicion_escritura = (posicion_escritura + 1) % datos_disparador -> buffersize;

            sem_wait(&espacio_buffer);
            Buffer[pos] = caracter;
            sem_post(&haydatos);          
        } 
        else
        {
            incorrectos ++;
        }  

    }
    for(int i=0; i < datos_disparador->numnaves; i++)
    {
        sem_wait(&espacio_buffer);
        int pos = posicion_escritura;
        Buffer[pos] = '\0'; 
        posicion_escritura = (posicion_escritura + 1) % datos_disparador -> buffersize;
        sem_post(&haydatos);
    }
    fclose(fichero_entrada);


    datos_disparador->caracteres_correctos = correctos;    
    datos_disparador->caracteres_incorrectos = incorrectos;
    datos_disparador->caracteres_leidos = leidos;

    return 0;
}


// Hilo Nave
void* naves(void* arg)
{
    int id_nave = *((int *) arg);

    //Variables
    datos_naves.hit[id_nave] = 0;
    datos_naves.miss[id_nave] = 0; 
    datos_naves.health[id_nave] = 0;
    char caracter = ' ';

    while(caracter != '\0')
    {     
        sem_wait(&haydatos);   
        sem_wait(&lectura_buffer);
        int pos = posicion_lectura;
        caracter = Buffer[pos];  

        if ((caracter == '1') || (caracter == '2') || (caracter == '3'))
        {
            datos_naves.health[id_nave]++;
            caracter --;
            if(caracter == '0')
            {
                posicion_lectura = (posicion_lectura + 1) % datos_naves.buffersize;               
                // Ya se puede saltar ese caracter
                sem_post(&espacio_buffer);
            }
            else
            {
                Buffer[pos] = caracter; 
                sem_post(&haydatos);       
            }
            sem_post(&lectura_buffer); 
                       
        }
        else
        {
            posicion_lectura = (posicion_lectura + 1) % datos_naves.buffersize;
            sem_post(&lectura_buffer);
            // Ya se puede saltar ese caracter
            sem_post(&espacio_buffer);
            if(caracter == '*')
            {
                datos_naves.hit[id_nave] ++;
            }
            else if (caracter == ' ')
            {            
                datos_naves.miss[id_nave] ++;
            }
        }        
    }  
    datos_naves.end[id_nave] = true;
    sem_post(&nave_fin);
    return 0;
}


// Hilo Juez
void* juez(void* arg)
{
    struct General_Data * datos_juez;
    datos_juez = (struct General_Data*)arg;

    FILE *fichero_salida = fopen(datos_juez->fichero_salida, "w");
    
    fprintf(fichero_salida, "El disparador ha procesado: %d tokens válidos y %d tokens inválidos, total: %d", datos_juez->caracteres_correctos, datos_juez->caracteres_incorrectos, datos_juez->caracteres_leidos);

    for(int k=0; k < datos_naves.numnaves; k++)
    {
        // Se guarda la puntucion
        datos_naves.puntuacion[k] = datos_naves.hit[k] - datos_naves.health[k];
        // Se escribe en el fichero        
        fprintf(fichero_salida, "\nNave: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuacion: %d\n",
        k, datos_naves.hit[k], datos_naves.miss[k], datos_naves.health[k],  datos_naves.puntuacion[k]);
    }

    int mayor_puntuacion;
    for(int posicion = 1; posicion <= 2; posicion++)
    {
        // Se busca la puntuacion mas alta
        mayor_puntuacion = 0;
        for(int j=0; j < datos_naves.numnaves; j++)
        {
            if(datos_naves.puntuacion[mayor_puntuacion] < datos_naves.puntuacion[j])
            {
                mayor_puntuacion = j;
            }
        }
        if (posicion == 1)
        {
            fprintf(fichero_salida, "\nNave Ganadora: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuacion: %d\n",
            mayor_puntuacion, datos_naves.hit[mayor_puntuacion], datos_naves.miss[mayor_puntuacion], datos_naves.health[mayor_puntuacion],  datos_naves.puntuacion[mayor_puntuacion] );
        }
        else
        {
            fprintf(fichero_salida, "\nNave Subcampeona: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuacion: %d\n",
            mayor_puntuacion, datos_naves.hit[mayor_puntuacion], datos_naves.miss[mayor_puntuacion], datos_naves.health[mayor_puntuacion],  datos_naves.puntuacion[mayor_puntuacion] );
        }
        // Se quita esa puntuacion para buscar la segunda mejor
        datos_naves.puntuacion[mayor_puntuacion] = 0;
        posicion ++;
    }

    int totalhits = 0;
    int totalmisses = 0;
    int totalhp = 0;
    int totaltoken = 0;

    for(int i=0; i < datos_naves.numnaves; i++)
    {
        totalhits += datos_naves.hit[i];
        totalmisses += datos_naves.miss[i];
        totalhp += datos_naves.health[i];
        totaltoken = totaltoken + datos_naves.hit[i] + datos_naves.miss[i] + datos_naves.health[i];
    }

    fprintf(fichero_salida, "\n================== RESUMEN ==============\n\tDisparos recibidos Totales: %d\n\tDisparos fallados Totales: %d\n\tBotiquines obtenidos Totales: %d\n\tTotal de tokens emitidos: %d",
    totalhits, totalmisses, totalhp, totaltoken);
    
    fclose(fichero_salida);
    return 0;
}




int main(int argc, char* argv[])
{
    // Variables
    char* fichero_entrada;
    char* fichero_salida;
    int buffersize; 
    int numnaves;
    FILE *ficheros;

    if(argc != 5)
    {
        // Se comprueba que el numero de parametros sea correcto
        printf("El numero de parametros no es correcto\n");
        exit(0);
    }
    else
    {
        // Se comprueba que el fichero de entrada existe
        fichero_entrada = argv[1];
        
        if (ficheros = fopen(fichero_entrada, "r")) 
        {
            // El fichero existe
            fclose(ficheros);
        }
        else
        {
            printf("No se ha podido abrir el archivo (de entrada)\n");
            exit(-1);
        } 

        // Se comprueba que el fichero de salida existe
        fichero_salida = argv[2];
        if (ficheros = fopen(fichero_salida, "r")) 
        {
            // El fichero existe
            fclose(ficheros);
        }
        else
        {
            printf("No se ha podido abrir el archivo (de salida)\n");
            exit(-1);
        }         

        // Se comprueba que el tamano del buffer es correcto
        buffersize = atoi(argv[3]);
        if(buffersize <= 0)
        {
            printf("El valor introducido para el tamano del buffer es incorrecto\n");
            exit(-1);
        }

        // Se comprueba que el numero de naves es correcto
        numnaves = atoi(argv[4]);
        if(numnaves <= 0)
        {
            printf("El valor introducido para el numero de naves es incorrecto\n");
            exit(-1);
        }
    }

    // Se inicializan los tipso de datos con toda la informacion necesaria
    // Datos recibidos
    struct General_Data datos;
    datos.fichero_entrada = fichero_entrada;
    datos.fichero_salida = fichero_salida;
    datos.buffersize = buffersize;
    datos.numnaves = numnaves;

    // Datos utilizados por las naves
    datos_naves.buffersize = buffersize;
    datos_naves.numnaves = numnaves;
    datos_naves.hit = (int*)malloc(numnaves*sizeof(int));
    datos_naves.miss = (int*)malloc(numnaves*sizeof(int));
    datos_naves.health = (int*)malloc(numnaves*sizeof(int));    
    datos_naves.puntuacion = (int*)malloc(numnaves*sizeof(int));
    datos_naves.end = (bool*)malloc(numnaves*sizeof(bool));
    for(int i=0; i < numnaves*sizeof(bool); i++)
    {
        datos_naves.end[i] = false;
    }

    // Se inicializan las variables
    posicion_escritura = 0;
    posicion_lectura = 0;
    Buffer = (char*)malloc(buffersize*sizeof(char));

    // Se inicializan los semaforos
    sem_init(&espacio_buffer, 0, buffersize);
    sem_init(&haydatos, 0, 0);
    sem_init(&lectura_buffer, 0, 1);
    sem_init(&nave_fin, 0, 0);
    
    // Se crea el hilo disparador
    pthread_t Disparador;    
    pthread_t Naves[numnaves];  
    pthread_t Juez;    

    // Se crea una lista de identificadores
    int id_naves[numnaves];
    for(int i = 0; i < numnaves; i++)
    {
        id_naves[i] = i;
    }

    pthread_create(&Disparador, NULL, disparador, (void*)&datos); 
    for(int i = 0; i < numnaves; i++)
    {
        pthread_create(&Naves[i], NULL, naves, (void*)&id_naves[i]); 
    }    
    pthread_create(&Juez, NULL, juez, (void*)&datos);  
       

    // Se espera a que los hilos terminen    
    pthread_join(Disparador, NULL);
    for(int i = 0; i < numnaves; i++)
    {
        pthread_join(Naves[i], NULL);
    }    
    pthread_join(Juez, NULL);


    // Se eliminan las variables que se han utilizado
    sem_destroy(&espacio_buffer);
    sem_destroy(&haydatos);    
    sem_destroy(&lectura_buffer);
    sem_destroy(&nave_fin);

    free(Buffer);
    free(datos_naves.hit);
    free(datos_naves.miss);
    free(datos_naves.health);
    free(datos_naves.puntuacion);
    free(datos_naves.end);


    return 0;
}