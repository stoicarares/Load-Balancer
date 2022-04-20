/* Copyright 2021 <Stoica Rares-Tiberiu> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LinkedList.h"
#include "utils.h"
#include "load_balancer.h"
#include "server.h"

linked_list_t *
ll_create(unsigned int data_size)
{
	linked_list_t* ll;

	ll = malloc(sizeof(*ll));
	DIE(ll == NULL, "linked_list malloc");

	ll->head = NULL;
	ll->data_size = data_size;
	ll->size = 0;

	return ll;
}

void
ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data)
{
	ll_node_t *prev, *curr;
	ll_node_t* new_node;

	if (list == NULL) {
		return;
	}

	/* n >= list->size inseamna adaugarea unui nou nod la finalul listei. */
	if (n > list->size) {
		n = list->size;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	new_node = malloc(sizeof(*new_node));
	DIE(new_node == NULL, "new_node malloc");
	new_node->data = malloc(list->data_size);
	DIE(new_node->data == NULL, "new_node->data malloc");
	memcpy(new_node->data, new_data, list->data_size);

	new_node->next = curr;
	if (prev == NULL) {
		/* Adica n == 0. */
		list->head = new_node;
	} else {
		prev->next = new_node;
	}

	list->size++;
}

ll_node_t *
ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (list == NULL) {
		return NULL;
	}

	/* Lista este goala. */
	if (list->head == NULL) {
		return NULL;
	}

	/* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
	if (n > list->size - 1) {
		n = list->size - 1;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (prev == NULL) {
		/* Adica n == 0. */
		list->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

ll_node_t* ll_get_nth_node(linked_list_t* list, unsigned int n)
{
	/** Daca se da o pozitie mai mare decat numarul de elemente din lista,
	 * se va returna ultimul element al listei.
	 */
	if (n > list->size - 1)
		n = list->size - 1;

	ll_node_t* node = list->head;

	for (unsigned int i = 0; i < n; ++i)
		node = node->next;

	return node;
}

unsigned int
ll_get_size(linked_list_t* list)
{
	if (list == NULL) {
		return -1;
	}

	return list->size;
}

void
ll_free(linked_list_t** pp_list)
{
	ll_node_t* currNode;

	if (pp_list == NULL || *pp_list == NULL) {
		return;
	}

	while (ll_get_size(*pp_list) > 0) {
		currNode = ll_remove_nth_node(*pp_list, 0);
		free(currNode->data);
		free(currNode);
	}

	free(*pp_list);
	*pp_list = NULL;
}
