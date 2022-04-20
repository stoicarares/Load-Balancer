/* Copyright 2021 <Stoica Rares-Tiberiu> */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "load_balancer.h"
#include "server.h"
#include "LinkedList.h"
#include "redistribution.h"
#include "utils.h"

#define MAX_SERVERS 100000

unsigned int hash_function_servers(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

load_balancer* init_load_balancer() {
	/* Se creaza load balancer-ul */
	load_balancer *main_server = malloc(sizeof(load_balancer));
	/* Se creaza si initializeaza hashring-ul */
	main_server->hash_ring = ll_create(sizeof(server_info));
	/* Se creaza array-ul de servere si se aloca memorie pentru el */
	main_server->servers = calloc(MAX_SERVERS, sizeof(server_memory *));

	return main_server;
}

char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
	DIE(!main, "Load Balancer doesen't exist!\n");
	unsigned int hash = hash_function_key(key);
	ll_node_t *it = main->hash_ring->head;

	/** Parcurgem hashringul pana ajungem la serverul din care trebuie sa
	 * extragem valoarea cheii key.
	 */
	while (it && ((server_info *)(it->data))->hash < hash) {
		it = it->next;
	}

	/** Trebuie sa ii dam parametrului server_id valoarea potrivita pentru a fi
	 * vizibil in main.
	 */
	if (!it) {
		/** Daca s-a ajuns la final, inseamna ca hash-ul cheii date ca
		 * parametru este mai mare decat hash-ul oricarui server,
		 * deci produsul asociat acesteia se regaseste pe
		 * primul server(head-ul listei).
		 */
		*server_id = ((server_info *)(main->hash_ring->head->data))->id;
	} else {
		/* Se atribuie serverului valoarea corespunzatoare. */
		*server_id = ((server_info *)it->data)->id;
	}

	/** Se apeleaza functia server_retrieve pentru a obtine valoarea asociata
	 * cheii key. 
	 */ 
	return server_retrieve(main->servers[*server_id], key);
}

void loader_store(load_balancer* main, char* key, char* value, int* server_id)
{
	DIE(!main, "Load Balancer doesen't exist!\n");
	unsigned int hash = hash_function_key(key);
	ll_node_t *it = main->hash_ring->head;

	/** Parcurgem hashringul pana ajungem la serverul pe care trebuie sa
	 * sa adaugam perechea key-value.
	 */
	while (it && ((server_info *)(it->data))->hash < hash) {
		it = it->next;
	}

	if (!it) {
		/** Daca s-a ajuns la final, inseamna ca hash-ul cheii date ca 
		 * parametru este mai mare decat hash-ul oricarui server,
		 * deci produsul asociat acesteia se stocheaza pe
		 * primul server(head-ul listei).
		 */
		server_store(main->servers[((server_info *)(main->hash_ring->
			head->data))->id], key, value);
		*server_id = ((server_info *)(main->hash_ring->head->data))->id;
	} else {
		/** Daca s-a gasit un server cu hash-ul mai mare decat al cheii key,
		 * noul produs se va stoca pe acesta.
		 */
		server_store(main->servers[((server_info *)it->data)->id],
					key, value);
		*server_id = ((server_info *)it->data)->id;
	}
}

void loader_add_server(load_balancer* main, int server_id) {
	DIE(!main, "Load Balancer doesen't exist!\n");

	/* Se initializeaza noul server. */
	server_memory *new_server = init_server_memory();
	new_server->info.id = server_id;
	new_server->info.hash = hash_function_servers(&server_id);
	/* Se adauga in array-ul de servere. */
	main->servers[server_id] = new_server;

	/* Trebuie sa adaugam pe hashring serverul cat si replicile sale. */
	for (unsigned int i = 0; i < 3; i++) {
		/** Se calculeaza eticheta si hashul fiecarui server
		 * ce urmeaza a fi adaugat pe hasring.
		 */
		int label = i * MAX_SERVERS + server_id;
		unsigned int hash = hash_function_servers(&label);

		/** Fiecare replica a unui server va retine id-ul tata in hasring,
		 * pentru a le putea accesa mai usor ulterior si hash-ul propriu.
		 */
		server_info new_info;
		new_info.hash = hash;
		new_info.id = server_id;

		/** Daca hashring-ul aste gol, nu avem cu ce sa comparam noul server,
		 * asa ca se va adauga la inceputul listei, indiferent de
		 * valoarea hash-ului propriu.
		 */
		if (!main->hash_ring->size) {
			ll_add_nth_node(main->hash_ring, 0, &new_info);
		} else {
			/** Voi cauta pozitia unde trebuie adaugat pe hashring noul server.
			 * Voi retine si nodul anterior de fiecare data pentru cazurile
			 * extreme.
			 */
			ll_node_t *it = NULL, *prev = NULL;
			it = prev = main->hash_ring->head;

			/* Pozitia pe care va fi inserat noul server. */
			int index = 0;

			/* Iterez prin hashring. */
			while (it && ((server_info *)it->data)->hash < hash) {
				prev = it;
				it = it->next;
				index++;
			}

			/** Daca indexul este 0, inseamna ca trebuie sa fie adaugat primul
			 * si va trebui sa ii atribui elemente din ultimul server,
			 * asadar prev va pointa cate ultimul element al listei.
			*/
			if (!index) {
				unsigned int size = ll_get_size(main->hash_ring);
				prev = ll_get_nth_node(main->hash_ring, size - 1);
			}

			/** Voi retine intr-o variabila logica daca s-a ajuns la finalul
			 * hasrhingului (NULL), deoarece inseamna ca se va adauga noul
			 * server pe ultima pozitie si va trebui sa redistribui elementele
			 * ce se afla intre penultimul si primul.
			 * Astfel it, fiind NULL, va pointa catre head-ul listei.
			 */
			int cycle_over = 0;
			if (!it) {
				it = main->hash_ring->head;
				cycle_over = 1;
			}

			/** Daca noul server adaugat este o replica a anteriorului,
			 * adaugarea se va face fara redistribuirea elementelor.
			 */ 
			int id_next = ((server_info *)it->data)->id;
			if (id_next == server_id){
				ll_add_nth_node(main->hash_ring, index, &new_info);
				continue;
			}
			/** Se face redistribuirea elementelor de pe serverul it->id
			 * pe serverul server_id, daca este necesar.
			 */
			redistribution_add_server(main, it, prev, &new_info, server_id,
									  cycle_over);

			/** Se adauga noul server in hashring pe pozitia index. */
			ll_add_nth_node(main->hash_ring, index, &new_info);
		}
	}
}

void loader_remove_server(load_balancer* main, int server_id) {
	DIE(!main, "Load Balancer doesen't exist!\n");

	/** Voi cauta pozitiile serverului care trebuie eliminat de pe hashring.
	 * Voi retine si nodul anterior de fiecare data pentru cazurile extreme.
	 */
	ll_node_t *it = NULL, *prev = NULL;
	it = prev = main->hash_ring->head;

	/* Pozitia pe care se va afla nodul ce va fi eliminat de pe hashring. */
	unsigned int index = 0;

	/** Se repeta de 3 ori eliminarea, deoarece trebuie sters serverul,
	 * cat si replicile sale.
	 */
	for (int i = 0; i < 3; i++) {
		/* Se itereaza prin hashring pana se gaseste un server cu id-ul cerut*/
		while (it && ((server_info *)it->data)->id != server_id) {
			prev = it;
			it = it->next;
			index++;
		}

		/** Daca nu este ultimul nod si daca id-ul sau este egal cu
		 * al urmatorului, nu este nevoie de redistribuire a produselor,
		 * ci doar se elimina nodul din hashring si i se elibereaza memoria.
		 */
		if (it->next && ((server_info *)it->data)->id ==
				((server_info *)it->next->data)->id) {
			it = it->next;
			ll_node_t *del = ll_remove_nth_node(main->hash_ring, index);
			free(del->data);
			free(del);
			continue;
		}

		/* Se face redistribuirea produselor din serverul sters. */
		redistribution_remove_server(main, it, prev, server_id);

		/** Se sterge si se elibereaza memoria nodului eliminat din
		 * lista(hashring). Se face next inainte de a-l sterge pentru
		 * a nu se pierde legatura catre urmatoarele elemente.
		 * Indexul nu se modifica, eliminandu-se un element, cautarea
		 * va continua de pe aceeasi pozitie.
		 */
		it = it->next;
		ll_node_t *del = ll_remove_nth_node(main->hash_ring, index);
		free(del->data);
		free(del);
	}

	/** Dupa eliminarea celor 3 replici din hashring, se elibereaza memoria
	 * pentru serverul aferent si pointerul din vector se trece pe NULL pentru
	 * a nu se mai elibera din nou in functia free_load_balancer().
	 */
	free_server_memory(main->servers[server_id]);
	main->servers[server_id] = NULL;
}

void free_load_balancer(load_balancer* main) {
	DIE(!main, "Load Balancer doesen't exist!\n");

	/** Se elibereaza memoria tuturor serverelor care nu au fost eliminate pana
	 * la finalul executiei programului.
	 */
	for (int i = 0; i < MAX_SERVERS; i++) {
		if (main->servers[i])
			free_server_memory(main->servers[i]);
	}

	/* Se elibereaza memoria alocata pentru hashring. */
	ll_free(&main->hash_ring);

	/* Se elibereaza memoria alocata pentru vecotrul de servere. */
	free(main->servers);

	/* Se elibereaza memoria alocata pentru load balancer */
	free(main);
}
