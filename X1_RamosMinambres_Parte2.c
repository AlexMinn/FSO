// Javier Ramos - Alejandro Minambres - Practica Final Guerra Naves


// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>


// Variables Compartidas
char* Buffer;
int posicion_escritura;
int posicion_lectura;


// Semaforos
sem_t espacio_buffer;
sem_t haydatos;
sem_t mutex_lectura_buffer;
sem_t haynave;
sem_t fin_disparador;
sem_t mutex_lista;


/* Listas Enlazadas */
struct ListaEnlazada 
{
    int id;
    int hit;
    int miss;
    int health; 
    int puntuacion;
    struct ListaEnlazada* siguiente;
};
// Funcion para incluir un elemento en la ultima posición de la lista enlazada
void append(struct ListaEnlazada** lista_enlazada, int ident, int hit, int miss, int health)
{
    struct ListaEnlazada* nuevo_nodo;
    // Se crea un nodo donde se guardan los datos
    nuevo_nodo = (struct ListaEnlazada *) malloc(sizeof(struct ListaEnlazada));

    // Se guardan todos los datos
    nuevo_nodo->id = ident;
    nuevo_nodo->hit = hit;
    nuevo_nodo->miss = miss;
    nuevo_nodo->health = health;
    nuevo_nodo->puntuacion = hit-health;
    nuevo_nodo->siguiente = NULL;
    
    // Se comprueba si es el primer elemento que se está insertando
    if (*lista_enlazada == NULL)
    {
        *lista_enlazada = nuevo_nodo;
    }
    else
    {
        // Coloca la nueva lista al final
        while ((*lista_enlazada)->siguiente != NULL)
        {
            lista_enlazada = &((*lista_enlazada)->siguiente);
        }          
        // Se incluye el enlace al siguiente elemento     
        (*lista_enlazada)->siguiente = nuevo_nodo;
                    
    }
    
}
// Funcion para liberar todo el espacio utilizado por la lista enlazada (borrado)
void freeLista(struct ListaEnlazada* lista_enlazada)
{
    struct ListaEnlazada *siguiente = lista_enlazada->siguiente;
    while (siguiente != NULL)
    {
        free(lista_enlazada);
        lista_enlazada = siguiente;
        siguiente = lista_enlazada->siguiente;
    }         
}
/* Fin Listas Enlazadas */



/* Struct General Data */
// Estructura de datos donde se guarda toda la informacion que se necesita en el programa 
struct General_Data
{
    char* fichero_entrada;
    char* fichero_salida;    
    int buffersize;
    int numnaves; 
    struct ListaEnlazada* lista_enlazada;
} datos;
/* Fin Struct General Data */



/* Struct Naves Data */
// Estructura de datos donde se guarda toda la informacion que necesita cada una de las naves 
struct Naves_Data
{
    int* identificadores;
    int numnaves;
    int buffersize;
} datos_naves;
/* Fin Struct Naves Data */ 


/* Funcion isAlpha*/
// Funcion para comprobar si una cadena tiene o no caracteres
bool isAlpha(char* cadena){
    int pos = 0;
    while (cadena[pos]){
        if (isalpha(cadena[pos]) && cadena[pos] != ' '){
            return true;
        }
        pos ++;
    }

    return false;
}


/* Hilos */
// Hilo Disparador 
void* disparador(void* arg)
{
    // Se guarda la estructura con todos los datos
    struct General_Data * datos_disparador = (struct General_Data*)arg;

    //Variables locales del disparador   
    FILE *fichero_entrada = fopen(datos_disparador -> fichero_entrada, "r");
    char caracter;
    bool caracter_correcto;
    int correctos = 0;
    int incorrectos = 0;
    int leidos = 0;

    // Se lee el fichero
    while ((caracter = fgetc(fichero_entrada)) != EOF) 
    {
        caracter_correcto = false;
        leidos ++;

        // Se comprueba si el caracter es correcto
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
            // Se guarda el caracter que aparece detras del botiquin
            caracter = fgetc(fichero_entrada);

            // Se comprueba si este caracter es correcto
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

            // Se copia y actualiza la posición de lectura
            int pos = posicion_escritura;
            posicion_escritura = (posicion_escritura + 1) % datos_disparador -> buffersize;
            
            // Se guarda la informacion en el buffer
            sem_wait(&espacio_buffer);
            Buffer[pos] = caracter;
            sem_post(&haydatos);          
        } 
        else
        {
            incorrectos ++;
        }  

    }
    
    // Se guarda caracter de final
    sem_wait(&espacio_buffer);
    int pos = posicion_escritura;
    Buffer[pos] = '\0'; 
    sem_post(&haydatos);

    fclose(fichero_entrada);

    // Se imprimen los datos procesados por el disparador
    FILE *fichero_salida = fopen(datos_disparador->fichero_salida, "w");
    fprintf(fichero_salida, "El disparador ha procesado: %d tokens válidos y %d tokens inválidos, total: %d", correctos, incorrectos, leidos);
    fclose(fichero_salida);
    sem_post(&fin_disparador);

    return 0;
}



// Hilo Nave
void* naves(void* arg)
{
    // Se guarda el identificador de la nave
    int id_nave = *((int *) arg);

    // Se crean las variables locales
    int hit = 0;
    int miss = 0;
    int health = 0;
    char caracter = ' ';

    // Se recorre el buffer hasta que se encuentra el final
    while(caracter != '\0')
    {     
        sem_wait(&haydatos);   
        sem_wait(&mutex_lectura_buffer);

        // Se guarda la posicion y el caracter
        int pos = posicion_lectura;
        caracter = Buffer[pos];  

        if ((caracter == '1') || (caracter == '2') || (caracter == '3'))
        {
            // Estamos en un botiquin
            health++;
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
            sem_post(&mutex_lectura_buffer); 
                       
        }
        else if (caracter != '\0')
        {
            // Se puede saltar ese caracter
            posicion_lectura = (posicion_lectura + 1) % datos_naves.buffersize;
            sem_post(&mutex_lectura_buffer);
            sem_post(&espacio_buffer);
            if(caracter == '*')
            {
                hit++;
            }
            else if (caracter == ' ')
            {            
                miss++;
            }
        }     
        else
        {
            // Ya no quedan mas datos por leer
            sem_post(&haydatos);
            sem_post(&mutex_lectura_buffer);
        }   
    }

    // Se guarda el identificador de la nave en la lista enlazada
    sem_wait(&mutex_lista);
    append(&(datos.lista_enlazada), id_nave, hit, miss, health);
    sem_post(&mutex_lista);

    // Se indica que ya se puede leer una nave
    sem_post(&haynave);

    return 0;
}



// Hilo Juez
void* juez(void* arg)
{
    // Se espera a que el disparador termine
    sem_wait(&fin_disparador);

    struct General_Data * datos_juez = (struct General_Data*)arg;
    struct ListaEnlazada * pos_lista = datos_juez->lista_enlazada;

    // Variables locales    
    int id;
    int id_primero, hit_primero = 0, miss_primero = 0, health_primero = 0, puntuacion_primero = -1;
    int id_segundo, hit_segundo = 0, miss_segundo = 0, health_segundo = 0, puntuacion_segundo = -1;
    int totalhits = 0, totalmisses = 0, totalhp = 0, totaltoken = 0;
    FILE *fichero_salida = fopen(datos_juez->fichero_salida, "a");
    
    
    // Se recorre la lista enlazada
    for(int i=0; i<datos.numnaves; i++)
    {   
        // Se espera a que se tenga la información de una nave
        sem_wait(&haynave);

        // Se espera a acceder a la lista
        sem_wait(&mutex_lista);

        // Se imprime la informacion de la nave correspondiente
        fprintf(fichero_salida, "\nNave: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuación: %d\n",
        pos_lista->id, pos_lista->hit, pos_lista->miss, pos_lista->health,  pos_lista->puntuacion);

        // Se cuentan los tokens
        totalhits += pos_lista->hit;
        totalmisses += pos_lista->miss;
        totalhp += pos_lista->health;
        totaltoken += pos_lista->hit + pos_lista->miss + pos_lista->health;

        // Se guardan las maximas puntuaciones
        if(pos_lista->puntuacion > puntuacion_primero)
        {
            // Se tiene una maxima puntuacion
            id_segundo = id_primero;
            hit_segundo = hit_primero;
            miss_segundo = miss_primero;
            health_segundo = health_primero;
            puntuacion_segundo = puntuacion_primero;
            
            id_primero = pos_lista->id;
            hit_primero = pos_lista->hit;
            miss_primero = pos_lista->miss;
            health_primero = pos_lista->health;
            puntuacion_primero = pos_lista->puntuacion;
        }
        else if (pos_lista->puntuacion > puntuacion_segundo)
        {
            // Se tiene una segunda maxima puntuacion
            id_segundo = pos_lista->id;
            hit_segundo = pos_lista->hit;
            miss_segundo = pos_lista->miss;
            health_segundo = pos_lista->health;
            puntuacion_segundo = pos_lista->puntuacion;
        }

        // Se pasa a la siguiente posicion        
        pos_lista = pos_lista->siguiente;
        sem_post(&mutex_lista);
    }

    // Se imprime a los ganadores
    fprintf(fichero_salida, "\nNave Ganadora: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuación: %d\n",
    id_primero, hit_primero, miss_primero, health_primero,  puntuacion_primero);

    // Se imprime al subcampeon si existe
    if(datos.numnaves > 1)
    {
        fprintf(fichero_salida, "\nNave Subcampeona: %d\n\tDisparos recibidos: %d\n\tDisparos fallados: %d\n\tBotiquines obtenidos: %d\n\tPuntuación: %d\n",
        id_segundo, hit_segundo, miss_segundo, health_segundo,  puntuacion_segundo);
    }
    

    // Se imprime el resumen
    fprintf(fichero_salida, "\n================== RESUMEN ==============\n\tDisparos recibidos Totales: %d\n\tDisparos fallados Totales: %d\n\tBotiquines obtenidos Totales: %d\n\tTotal de tokens emitidos: %d\n",
    totalhits, totalmisses, totalhp, totaltoken);


    fclose(fichero_salida);
    return 0;
}
/* Fin Hilos */


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
        if (isAlpha(argv[3]))
        {
            printf("El tamano del buffer no es un entero");
            exit(-1);
        }
        if(buffersize <= 0)
        {
            printf("El valor introducido para el tamano del buffer es incorrecto\n");
            exit(-1);
        }

        // Se comprueba que el numero de naves es correcto
        numnaves = atoi(argv[4]);
        if (isAlpha(argv[4]))
        {
            printf("El numero de naves no es un entero");
            exit(-1);
        }
        if(numnaves <= 0)
        {
            printf("El valor introducido para el numero de naves es incorrecto\n");
            exit(-1);
        }
    }

    // Se inicializan los tipos de datos con toda la informacion necesaria

    // Datos recibidos
    datos.fichero_entrada = fichero_entrada;
    datos.fichero_salida = fichero_salida;
    datos.buffersize = buffersize;
    datos.numnaves = numnaves;


    // La lista Enlazada en un comienzo es nula
    datos.lista_enlazada = NULL;


    // Datos utilizados por las naves
    datos_naves.buffersize = buffersize;
    datos_naves.numnaves = numnaves;    


    // Se inicializan las variables globales
    posicion_escritura = 0;
    posicion_lectura = 0;
    Buffer = (char*)malloc(buffersize*sizeof(char));


    // Se inicializan los semaforos
    sem_init(&espacio_buffer, 0, buffersize);
    sem_init(&haydatos, 0, 0);
    sem_init(&mutex_lectura_buffer, 0, 1);
    sem_init(&haynave, 0, 0);
    sem_init(&fin_disparador, 0, 0);
    sem_init(&mutex_lista, 0, 1);

    
    // Se definen los hilos
    pthread_t Disparador;    
    pthread_t* Naves = (pthread_t*) malloc(numnaves*sizeof(pthread_t));
    pthread_t Juez;    


    // Se crea una lista de identificadores para las naves
    int* id_naves = (int*) malloc(numnaves*sizeof(int));
    for(int i = 0; i < numnaves; i++)
    {
        id_naves[i] = i;
    }


    // Se crean los hilos
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


    // Se eliminan los semaforos que se han utilizado
    sem_destroy(&espacio_buffer);
    sem_destroy(&haydatos);    
    sem_destroy(&mutex_lectura_buffer);
    sem_destroy(&haynave);
    sem_destroy(&fin_disparador);


    // Se libera toda la memoria utilizada
    free(Buffer);
    free(Naves);
    free(id_naves);
    freeLista(datos.lista_enlazada);


    return 0;
}