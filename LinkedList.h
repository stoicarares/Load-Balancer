/* Copyright 2021 <Stoica Rares-Tiberiu> */
#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

typedef struct ll_node_t ll_node_t;
struct ll_node_t {
	void* data;
	ll_node_t* next;
};

typedef struct linked_list_t linked_list_t;
struct linked_list_t {
	ll_node_t* head;
	unsigned int data_size;
	unsigned int size;
};

/* Creaza si initializeaza o lista cu elemente de dimensiune data_size. */
linked_list_t*
ll_create(unsigned int data_size);

/** Adauga un nou nod pe pozitita n din lista si reface lagaturile cu celelalte
 * deja existente. Este o lista generica, nodul continand date de tip void*.
 */
void
ll_add_nth_node(linked_list_t* list, unsigned int n, const void* data);

/** Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Functia intoarce un pointer spre acest
 * nod proaspat eliminat din lista. Este responsabilitatea apelantului sa
 * elibereze memoria acestui nod.
 */
ll_node_t*
ll_remove_nth_node(linked_list_t* list, unsigned int n);

/** Functia intoarce un pointer catre nodul n din lista al carei pointer este
 * trimis ca parametru.
 */
ll_node_t*
ll_get_nth_node(linked_list_t* list, unsigned int n);

/** Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */
unsigned int
ll_get_size(linked_list_t* list);

/** Functia elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */
void
ll_free(linked_list_t** pp_list);

#endif  // LINKEDLIST_H_
