/* Copyright 2021 <Stoica Rares-Tiberiu> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "Hashtable.h"
#include "server.h"

/* Functie de comparare a cheilor */
int
compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/* Functie care initializeaza un hashtable, alocand memorie pentru
 * toate componentele acestuia.
 */
hashtable_t *
ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*))
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	DIE(!ht, "malloc() failed!\n");

	/* Se aloca vectorul de liste */
	ht->buckets = malloc(hmax * sizeof(ht->buckets));
	DIE(!ht->buckets, "malloc failed()!\n");

	/* Se aloca memorie pentru fiecare de liste */
	for (unsigned int i = 0; i < hmax; i++) {
		ht->buckets[i] = ll_create(sizeof(struct info));
	}

	ht->compare_function = *compare_function;
	ht->hash_function = *hash_function;
	ht->hmax = hmax;
	ht->size = 0;

	return ht;
}

/* Functia care stocheaza in hashtable perechea key-value. */
void
ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size)
{
	unsigned int index = ht->hash_function(key) % ht->hmax;

	/* Daca exista deja cheia key, i se inlocuieste valoarea. */
	if (ht_has_key(ht, key)) {
		memcpy(ht_get(ht, key), value, value_size);
		return;
	}

	struct info inf;

	/* Se creaza o copie cheie-valoare in noul nod adaugat */
	inf.key = malloc(key_size);
	DIE(!inf.key, "malloc failed()!\n");
	memcpy(inf.key, key, key_size);

	inf.value = malloc(value_size);
	DIE(!inf.value, "malloc failed()!\n");
	memcpy(inf.value, value, value_size);

	/* Se adauga in nod structura cheie-valoare */
	ll_add_nth_node(ht->buckets[index], ht->buckets[index]->size,
					&inf);

	/* Se creste  */
	ht->size++;
}

/* Functie care returneaza valoarea asociata cheii key. */
void *
ht_get(hashtable_t *ht, void *key)
{
	unsigned int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *it = ht->buckets[index]->head;

	/** Se parcurge lista corespunzatoare indexului si se returneaza
	 * valoarea corelata cheii, daca exita, iar daca nu se returneaza NULL
	 */
	for (unsigned i = 0; i < ll_get_size(ht->buckets[index]); i++) {
		if(ht->compare_function(key, ((struct info *)(it->data))->key) == 0)
			return ((struct info *)(it->data))->value;
		it = it->next;
	}

	return NULL;
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable.
 * 0, altfel.
 */
int
ht_has_key(hashtable_t *ht, void *key)
{
	unsigned int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *it = ht->buckets[index]->head;
	for (unsigned i = 0; i < ll_get_size(ht->buckets[index]); i++) {
		if(ht->compare_function(key, ((struct info *)(it->data))->key) == 0)
			return 1;
		it = it->next;
	}

	return 0;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key si
 * elibereaza memoria folosita pentru aceasta.
 */
void
ht_remove_entry(hashtable_t *ht, void *key)
{
	unsigned int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *it = ht->buckets[index]->head;

	unsigned int i = 0;
	for (i = 0; i < ll_get_size(ht->buckets[index]); i++) {
		if (ht->compare_function(key, ((struct info *)(it->data))->key) == 0)
			break;
		it = it->next;
	}

	/** Se elibereaza memoria pentru perechea key-value din lista corespunzatoare
	 * din bucket.
	 */
	ll_node_t *tmp = NULL;
	tmp = ll_remove_nth_node(ht->buckets[index], i);
	free(((struct info *)(it->data))->key);
	free(((struct info *)(it->data))->value);
	free(tmp->data);
	free(tmp);

	ht->size--;
}

/*
 * Functia care elibereaza memoria folosita de toate intrarile din hashtable,
 * dupa care elibereaza si memoria folosita pentru a stoca structura hashtable.
 */
void
ht_free(hashtable_t *ht)
{
	for (unsigned int i = 0; i < ht->hmax; i++) {
		if (ht->buckets[i]->size) {
			ll_node_t *it = ht->buckets[i]->head;
			for (unsigned int j = 0; j < ht->buckets[i]->size; j++) {
				free(((struct info *)(it->data))->key);
				free(((struct info *)(it->data))->value);
				it = it->next;
			}
		}

		ll_free(&ht->buckets[i]);
	}

	free(ht->buckets);
	free(ht);
}

/* Functie care returneaza numarul de obiecte stocate in hashtable */
unsigned int
ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

/* Functie care returneaza numarul maxim de bucket-uri din hashtable */
unsigned int
ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}
