/* Copyright 2021 <Stoica Rares-Tiberiu> */
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "utils.h"

#define MAX_BUCKET_SIZE 10000

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

/* Functia care aloca memorie pentru un nou server. */
server_memory* init_server_memory() {
	server_memory *server = malloc(sizeof(server_memory));
	server->info.id = 0;
	server->info.hash = 0;
	/** Se initializeaza un hashtable cu functia de hash pentru chei
	 * si cu funtia de comparare pentru string-uri.
	 */
	server->ht = ht_create(MAX_BUCKET_SIZE, hash_function_key,
						   compare_function_strings);

	return server;
}

/** Stocarea intr-un anumit server reprezinta adaugarea in hashtable perechea
 * key-value.
 */
void server_store(server_memory* server, char* key, char* value) {
	DIE(!server, "Server doesen't exist!\n");

	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
}

/** Eliminarea unui pordus dintr-un anumit server reprezinta eliminarea din
 * hashtable a produsului corespunzator cheii key.
 */
void server_remove(server_memory* server, char* key) {
	DIE(!server, "Server doesen't exist!\n");

	ht_remove_entry(server->ht, key);
}

/** Afisarea valorii corespunzatoare cheii key reprezinta obtinerea acesteia
 * din hashtable.
 */
char* server_retrieve(server_memory* server, char* key) {
	DIE(!server, "Server doesen't exist!\n");

	return ht_get(server->ht, key);
}

/* Eliberarea memoriei unui server */
void free_server_memory(server_memory* server) {
	DIE(!server, "Server doesen't exist!\n");

	ht_free(server->ht);
    free(server);
}
