/* Copyright 2021 <Stoica Rares-Tiberiu> */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "redistribution.h"
#include "Hashtable.h"

void
redistribution_add_server(load_balancer* main, ll_node_t* it, ll_node_t* prev,
						  server_info *new_info, int server_id, int cycle_over)
{
	/** Noul server va trebuie adaugat intre prev si it, de aceeea am notat
	 * id_next id-ul lui it. Am retinut si hash-ul nodului anterior in
	 * variabila prev_hash;
	 */ 
	int id_next = ((server_info *)it->data)->id;
	unsigned int prev_hash = ((server_info *)prev->data)->hash;

	/** Parcurg array-ul de liste din hashtable-ul corespunzator serverului
	 * urmator.
	 */
	for (unsigned int j = 0; j < main->servers[id_next]->ht->hmax; j++) {
		/** Daca o lista nu este goala, inseamna ca exista perechi key-value
		 * care ar putea fi mutate pe noul server
		 */
		if (main->servers[id_next]->ht->buckets[j]->head) {
			/* Se parcurge lista in cazul in care au existat coliziuni. */
			ll_node_t *curr = NULL;
			curr = main->servers[id_next]->ht->buckets[j]->head;
			while (curr) {
				/** Se retine perechea key-value din nod si se aplica iar
				 * functia de hash pentru key, pentru a le compara.
				 */
				struct info tmp;
				tmp.key = ((struct info *)curr->data)->key;
				tmp.value = ((struct info *)curr->data)->value;
				unsigned int tmp_hash = hash_function_key(tmp.key);

				/** Se face next inainte ca acesta sa se elimine din hashtable,
				 * deoarece daca nodul curent dispare, practic pe pozitia lui
				 * ajunge ce este pe next. Daca nu se retinea inainte de a fi
				 * sters, se pierdea legatura catre urmatorul nod.
				 */
				curr = curr->next;

				/** Daca iteratorul se afla pe head si ciclul s-a terminat,
				 * asta inseamna ca noul server trebuie adaugat la final.
				 */
				if (it == main->hash_ring->head && cycle_over) {
					/** Trebuie mutate toate produsele care au hash-ul mai mare
					 * decat cel al serverului precedent si mai mic
					 * decat cel al serverului ce urmeaza a fi adaugat pe
					 * hashring. Astfel se scot din hashtable-ul urmator si se
					 * adauga in hashtable-ul noului server.
					 */ 
					if (tmp_hash > prev_hash
						&& tmp_hash < new_info->hash) {
						server_store(main->servers[server_id], tmp.key,
							tmp.value);
						server_remove(main->servers[id_next], tmp.key);
					}
				/** Daca iteratorul se afla pe head si ciclul nu s-a terminat,
				 * asta inseamna ca noul server trebuie adaugat la inceput.
				 */ 
				} else if (it == main->hash_ring->head && !cycle_over) {
					/** Trebuie mutate toate produsele care au hash-ul mai mare
					 * decat cel al serverului anterior(ultimul) sau mai mic
					 * decat cel al serverului nou, neputand sa fie amandoua
					 * adevarate in acelasi timp, nodul adaugandu-se la inceput.
					 * Astfel se scot din hashtable-ul fostului head si se adauga
					 * in hashtable-ul noului server.
					 */
					if (tmp_hash > prev_hash
							|| tmp_hash < new_info->hash) {
						server_store(main->servers[server_id], tmp.key,
							tmp.value);
						server_remove(main->servers[id_next], tmp.key);
					}
				/** Daca nu este un caz special(adaugare la final sau
				 * la inceput), pur si simplu se impune ca hash-ul produsului
				 * gasit in hashtable sa fie intre hash-ul serverului precedent
				 * si al serverului urmator
				 */
				} else if (tmp_hash < new_info->hash && tmp_hash > prev_hash) {
					server_store(main->servers[server_id], tmp.key,
								tmp.value);
					server_remove(main->servers[id_next], tmp.key);
				}
			}
		}
	}
}

void redistribution_remove_server(load_balancer* main, ll_node_t* it,
								  ll_node_t* prev, int server_id)
{
	/** Retin in doua variabile de tip server_info informatiile despre
	 * nodul ce trebuie eliminat si cel de inaintea lui pentru comoditate.
	 */
	server_info *it_data = it->data;
	server_info *prev_data = prev->data;

	/* Parcurg bucket-ul aferent serverului ce trebuie eliminat. */
	for (unsigned int j = 0; j < main->servers[server_id]->ht->hmax; j++) {
		if (main->servers[server_id]->ht->buckets[j]->head) {
			/* Se parcurge lista in cazul in care au existat coliziuni. */
			ll_node_t *curr = NULL;
			curr = main->servers[server_id]->ht->buckets[j]->head;
			while (curr) {
				/** Se retine perechea key-value din nod si se aplica iar
				 * functia de hash pentru key, pentru a le compara.
				 */
				struct info tmp;
				tmp.key = ((struct info *)curr->data)->key;
				tmp.value = ((struct info *)curr->data)->value;
				unsigned int tmp_hash = hash_function_key(tmp.key);

				/** Se face next inainte ca acesta sa se elimine din hashtable,
				 * deoarece daca nodul curent dispare, practic pe pozitia lui
				 * ajunge ce este pe next. Daca nu se retinea inainte de a fi
				 * sters, se pierdea legatura catre urmatorul nod.
				 */
				curr = curr->next;

				/** Daca nodul ce urmeaza a fi eliminat din hashring 
				 * nu este ultimul.
				 */
				if (it->next) {
					/** Trebuie mutate toate produsele care au hash-ul mai mare
					 * decat cel al serverului precedent si mai mic
					 * decat cel al serverului ce urmeaza a fi sters de pe hashring. Astfel se scot din
					 * hashtable-ul serverului eliminat si se adauga in hashtable-ul
					 * urmatorului server de pe hashring.
					 */ 
					if (tmp_hash < it_data->hash
							&& tmp_hash > prev_data->hash) {
						server_store(main->servers[((server_info *)
									it->next->data)->id], tmp.key,
									tmp.value);
						server_remove(main->servers[((server_info *)it->data)
									->id], tmp.key);
					}
				/** Daca nodul ce urmeaza a fi eliminat din hashring 
				 * este ultimul(lista nefiind circulara, daca trebuie
				 * eliminat ultimul nod, next-ul acestuia este head-ul listei,
				 * data fiind circularitatea hashringului).
				 */
				} else {
					/** Trebuie mutate toate produsele care au hash-ul mai mare
					 * decat cel al serverului precedent si mai mic
					 * decat cel al serverului ce urmeaza a fi sters de pe
					 * hashring. Astfel se scot din hashtable-ul serverului 
					 * eliminat si se adauga in hashtable-ul urmatorului server
					 * de pe hashring, de data aceasta fiind head-ul listei.
					 */ 
					if (tmp_hash > prev_data->hash &&
								tmp_hash < it_data->hash) {
						ll_node_t *head = main->hash_ring->head;

						int next_id = ((server_info *)head->data)->id;
						server_store(main->servers[next_id],
						tmp.key, tmp.value);
						server_remove(main->servers[it_data->id], tmp.key);
					}
				}
			}
		}
	}
}
