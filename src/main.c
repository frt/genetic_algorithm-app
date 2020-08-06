#include <parallel_evolution.h>
#include <gaul.h>   // the genetic algorithm library
#include <stddef.h>

#include "topology_parser/topology_parser.h"

#define ERROR_TOPOLOGY_CREATE 1
#define ERROR_TOPOLOGY_PARSE 2

#define MODULE_APP "genetic_algorithm-app"

// forward declarations
// in ga_core.c
boolean ga_entity_setup(population *pop, entity *joe);

extern double (*objective_function)(double*);           /* função de fitness (minimização) */

typedef struct genetic_algorithm {
    population *population;
    algorithm_stats_t stats;
    int max_generations;
} genetic_algorithm_t;

/* this struct is the "global namespace" of genetic_algorithm algorithm */
genetic_algorithm_t genetic_algorithm;

boolean assign_score(population *pop, entity *individual)
{
    // GAUL maximizes the fitness, the problem is of minimization, so:
    individual->fitness = 1 / (1 + objective_function(individual->chromosome[0]));  // fitness in the interval (0, 1]
    genetic_algorithm.stats.fitness_evals += 1;
    return TRUE;
}

void genetic_algorithm_init()
{
    population *pop;

    genetic_algorithm.max_generations = 50000;

    pop = genetic_algorithm.population;
    ga_population_set_allele_min_double(pop, -12);
    ga_population_set_allele_max_double(pop, 12);
    pop = ga_genesis_double(
            250,                        /* const int              population_size */
            1,                          /* const int              num_chromo */
            50,                         /* const int              len_chromo */
            NULL,                       /* GAgeneration_hook      generation_hook */
            NULL,                       /* GAiteration_hook       iteration_hook */
            NULL,                       /* GAdata_destructor      data_destructor */
            NULL,                       /* GAdata_ref_incrementor data_ref_incrementor */
            assign_score,               /* GAevaluate             evaluate */
            ga_seed_double_random,      /* GAseed                 seed */
            NULL,                       /* GAadapt                adapt */
            ga_select_one_roulette,     /* GAselect_one           select_one */
            ga_select_two_roulette,     /* GAselect_two           select_two */
            ga_mutate_double_singlepoint_randomize, /* GAmutate  mutate */
            ga_crossover_double_singlepoints, /* GAcrossover     crossover */
            NULL,                       /* GAreplace              replace */
            NULL                        /* void *                 userdata */
            );
}

void genetic_algorithm_run_iterations(int generations)
{
    genetic_algorithm.stats.iterations += ga_evolution(genetic_algorithm.population, generations);
}

void genetic_algorithm_insert_migrant(migrant_t *migrant)
{
    entity *new;
    population *pop;
    int i;

    pop = genetic_algorithm.population;

    // create new entity
    new = (entity *)mem_chunk_alloc(pop->entity_chunk);
    ga_entity_setup(pop, new);

    // asign chromossome
    for (i = 0; i < migrant->var_size; ++i) {
        ((double *)new->chromosome[0])[i] = migrant->var[i];
    }
    assign_score(pop, new);

    // insert into population
    ga_replace_by_fitness(pop, new);
}

void genetic_algorithm_pick_migrant(migrant_t *migrant)
{
    int i;
    entity *best;

    best = ga_get_entity_from_rank(genetic_algorithm.population, 0);
    for (i = 0; i < migrant->var_size; ++i) {
        migrant->var[i] = ((double *)best->chromosome[0])[i];
    }
}

int genetic_algorithm_ended()
{
    return genetic_algorithm.stats.iterations >= genetic_algorithm.max_generations;
}

status_t genetic_algorithm_get_population(population_t **pop2send)
{
}

algorithm_stats_t *genetic_algorithm_get_stats()
{
    return &genetic_algorithm.stats;
}

int main(int argc, char *argv[])
{
    algorithm_t *genetic_algorithm;
    int ret;
    topology_t *topology;
    char *topology_file;
    char topology_file_default[] = "ring.topology";

    if (argc == 2)  // program name + args count
        topology_file = argv[1];
    else
        topology_file = topology_file_default;

    /* create the topology */
    if (topology_create(&topology) != SUCCESS) {
        parallel_evolution_log(LOG_PRIORITY_ERR, MODULE_APP, "Topology could not be created. Quit.");
        return ERROR_TOPOLOGY_CREATE;
    }

    /* parse topology from file */
    if (topology_parser_parse(topology, topology_file) != SUCCESS) {
        topology_destroy(&topology);
        parallel_evolution_log(LOG_PRIORITY_ERR, MODULE_APP, "Topology could not be parsed. This is the end...");
        return ERROR_TOPOLOGY_PARSE;
    }

    parallel_evolution_set_topology(topology);

    parallel_evolution_set_number_of_dimensions(50);
    algorithm_create(&genetic_algorithm,
            genetic_algorithm_init,             // a wrapper around ga_genesis_double()
            genetic_algorithm_run_iterations,   // make a wrapper around ga_evolution()
            genetic_algorithm_insert_migrant,   // a wrapper around ga_replace_by_fitness(population *pop, entity *child);
            genetic_algorithm_pick_migrant,     // a wrapper around ga_get_entity_from_rank(pop,0)
            genetic_algorithm_ended,
            genetic_algorithm_get_population,   // TODO
            genetic_algorithm_get_stats);
    parallel_evolution_set_algorithm(genetic_algorithm);
    parallel_evolution_set_migration_interval(100);

    ret = parallel_evolution_run(&argc, &argv);

    algorithm_destroy(&genetic_algorithm);
    topology_destroy(&topology);

    return ret;
}
