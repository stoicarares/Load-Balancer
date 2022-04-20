/* Copyright 2021 <Stoica Rares-Tiberiu> */
#ifndef REDISTRIBUTION_H_
#define REDISTRIBUTION_H_

#include "load_balancer.h"
#include "LinkedList.h"

/** Functia care redistribuie corect produsele in situatia 
 * adaugarii unui nou server.
 */
void
redistribution_add_server(load_balancer* main, ll_node_t* it, ll_node_t* prev,
						  server_info *new_info, int server_id, int cycle_over);

/** Functia care redistribuie corect produsele in situatia 
 * eliminarii unui server din load balancer.
 */
void
redistribution_remove_server(load_balancer* main, ll_node_t* it,
								  ll_node_t* prev, int server_id);

#endif  // REDISTRIBUTION_H_
