// gcc -Wall -Wextra -Werror -g -pthread SabinaAskerova.c -o a.out
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdnoreturn.h>


#define DELAI_MAX        1000           // en ms

#define MAX_STR         1000            // "Thread 99999 lu 99999\n"

#define CHK(op)         do { if ((op) == -1) { raler (#op); } } while (0)
#define NCHK(op)        do { if ((op) == NULL) { raler (#op); } } while (0)
#define TCHK(op)        do { if ((errno=(op)) > 0) { raler (#op); } } while (0)

noreturn void
raler (const char *msg)                 // Une fonction simple pour les erreurs
{
    perror (msg) ;
    exit (1) ;
}

/******************************************************************************
 * Le buffer circulaire utilisé par le thread principal pour
 * envoyer des données aux threads fils.
 * Voir les fonctions buffer_* ci-dessous pour l'utilisation du buffer.
 * Note : il est possible d'ajouter des éléments à la structure, mais
 * pas de modifier les fonctions buffer_* existantes.
 */

struct buffer
{
    int r, w ;          // indices dans le buffer
    int n ;             // nombre d'entrées utilisées dans le tableau
    int sz ;            // taille du tableau
    int *tab ;          // tableau des valeurs
} ;

/*
 * Fonctions pour gérer le buffer :
 * buffer_init : alloue et initialise un buffer circulaire de taille sz
 * buffer_destroy : libère la mémoire utilisée pour le buffer circulaire
 * buffer_isempty : renvoie 1 si le buffer est vide
 * buffer_isfull : renvoie 1 si le buffer est plein
 * buffer_get : extrait une valeur dans le buffer, supposé non vide
 * buffer_put : ajoute une valeur dans le buffer, supposé non plein
 */

struct buffer *
buffer_init (int sz)                    // modification interdite
{
    struct buffer *b ;
    NCHK (b = malloc (sizeof *b)) ;
    b->n = b->r = b->w = 0 ;
    NCHK (b->tab = calloc (sz, sizeof b->tab[0])) ;
    b->sz = sz ;
    return b ;
}

void
buffer_destroy (struct buffer *b)       // modification interdite
{
    free (b->tab) ;
    free (b) ;
}

int
buffer_isempty (struct buffer *b)       // modification interdite
{
    return b->n == 0 ;
}

int
buffer_isfull (struct buffer *b)        // modification interdite
{
    return b->n == b->sz ;
}

int
buffer_get (struct buffer *b)           // modification interdite
{
    // pré-condition : le buffer n'est pas vide
    int v = b->tab [b->r] ;
    b->r = (b->r + 1) % b->sz ;
    b->n-- ;
    return v ;
}

void
buffer_put (struct buffer *b, int v)    // modification interdite
{
    // pré-condition : le buffer n'est pas plein
    b->tab [b->w] = v ;
    b->w = (b->w + 1) % b->sz ;
    b->n++ ;
}

/*
 * Fin de la gestion du buffer
 *****************************************************************************/

struct args{
    unsigned int thrnum;
    struct buffer *buf ;
    pthread_mutex_t * mtx;
    pthread_cond_t * cond;
    pthread_cond_t * cond_main;
    pthread_barrier_t * bar;
};

void *
un_thread (void *arg)
{
    struct args * myarg = arg;
    int thrnum ;                // mon numéro de thread
    char msg [MAX_STR] ;        // message affiché
    int v ;
    struct buffer *b ;

    thrnum = myarg->thrnum;
    b = myarg->buf;
    pthread_mutex_t * mtx = myarg->mtx;
    pthread_cond_t * cond = myarg->cond;
    pthread_cond_t * cond_main = myarg->cond_main;
    pthread_barrier_t * bar = myarg->bar;

    for (;;)                    // boucle infinie
    {
        // attendre une valeur dans le buffer
        TCHK(pthread_mutex_lock(mtx));
        while (buffer_isempty (b)){
            TCHK(pthread_cond_wait(cond, mtx));
        }
        v = buffer_get (b) ;    // extraire une valeur du buffer
        TCHK(pthread_mutex_unlock(mtx));

        TCHK(pthread_mutex_lock(mtx));
        TCHK(pthread_cond_signal(cond_main));
        TCHK(pthread_mutex_unlock(mtx));

        // Affichage de debug pour voir ce qui se passe (à laisser)
        (void) snprintf (msg, sizeof msg, "Thread %d lu %d\n", thrnum, v) ;
        CHK (write (1, msg, strlen (msg))) ;

        if (v == -1)
            break ;             // sortir de la boucle pour terminer le thread

        sleep (v) ;             // en parallèle avec les autres threads
    }

    // attendre que tout le monde ait fini pour l'affichage
    TCHK(pthread_barrier_wait(bar));
    CHK (write (1, "*", 1)) ;   // terminer par un \n pour l'esthétique

    return NULL ;
}

int
main (int argc, char *argv [])
{
    int nthreads, szbuf ;

    if (argc < 3 || (nthreads = atoi (argv [1])) <= 0
                || (szbuf = atoi (argv [2])) <= 0)
    {
        fprintf (stderr, "usage: %s nthreads szbuf v1 ... vn\n", argv [0]) ;
        fprintf (stderr, "\tavec nthreads > 0 et szbuf > 0\n") ;
        exit (1) ;
    }

    pthread_t tid [nthreads] ;
    struct args targ[nthreads];

    // Créer un buffer circulaire de taille szbuf
    struct buffer *buf ;
    buf = buffer_init (szbuf);
    pthread_mutex_t mtx;
    TCHK(pthread_mutex_init(&mtx, NULL));
    pthread_cond_t cond;
    TCHK (pthread_cond_init (&cond, NULL));
    pthread_cond_t cond_main;
    TCHK (pthread_cond_init (&cond_main, NULL));
    pthread_barrier_t bar;
    TCHK(pthread_barrier_init(&bar, NULL, nthreads));


    // Démarrer les threads
    for (int t = 0 ; t < nthreads ; t++)
    {
        targ[t].thrnum = t;
        targ[t].buf = buf;
        targ[t].cond = &cond;
        targ[t].cond_main = &cond_main;
        targ[t].mtx = &mtx;
        targ[t].bar = &bar;
        
        TCHK (pthread_create (& tid [t], NULL, un_thread, &targ[t])) ;
    }

    // Parcourir les valeurs fournies en argument (il peut ne pas y en avoir)
    for (int i = 3 ; i < argc ; i++)
    {
        int v = atoi (argv [i]) ;       // valeur à transmettre
        if (v < 0)
        {
            fprintf (stderr, "valeur < 0 : stop\n") ;
            break ;                     // passer directement à la fin
        }

         // attendre qu'il y ait de la place dans le buffer
        TCHK(pthread_mutex_lock(&mtx));
        while (buffer_isfull (buf)){
            TCHK(pthread_cond_wait(&cond_main, &mtx));
        }
            
        buffer_put (buf, v) ;
        TCHK(pthread_mutex_unlock(&mtx));
        TCHK(pthread_mutex_lock(&mtx));
        TCHK(pthread_cond_broadcast(&cond));
        TCHK(pthread_mutex_unlock(&mtx));
    }

    // Pour que tous les threads s'arrêtent, on envoie -1 à chacun d'eux
    for (int t = 0 ; t < nthreads ; t++)
    {
        // attendre qu'il y ait de la place dans le buffer
        TCHK(pthread_mutex_lock(&mtx));
        while (buffer_isfull (buf))
            TCHK(pthread_cond_wait(&cond_main, &mtx));;
        buffer_put (buf, -1) ;
        TCHK(pthread_mutex_unlock(&mtx));

        TCHK(pthread_mutex_lock(&mtx));      
        TCHK(pthread_cond_broadcast(&cond));
        TCHK(pthread_mutex_unlock(&mtx));
    }

    // Attendre la terminaison des thread
    for (int t = 0 ; t < nthreads ; t++)
    {
        TCHK (pthread_join (tid [t], NULL)) ;
    }
    TCHK (pthread_mutex_destroy (&mtx));
    TCHK (pthread_cond_destroy (&cond));
    TCHK (pthread_cond_destroy (&cond_main));
    TCHK (pthread_barrier_destroy (&bar));
    CHK (write (1, "\nfin\n", 5)) ;     // pour terminer en beauté

    buffer_destroy (buf) ;
    exit (0) ;
}
