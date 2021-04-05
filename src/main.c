#include <parallel_evolution.h>
#include <gaul.h>   // the genetic algorithm library
#include <stddef.h>
#include <float.h>
#include <sys/random.h>

#define ERROR_TOPOLOGY_CREATE 1
#define ERROR_TOPOLOGY_PARSE 2

#define MODULE_APP "genetic_algorithm-app"

extern double (*objective_function_p)(double*);           /* função de fitness (minimização) */

typedef struct genetic_algorithm {
    population *population;
    algorithm_stats_t stats;
    int max_generations;
} genetic_algorithm_t;

/* this struct is the "global namespace" of genetic_algorithm algorithm */
genetic_algorithm_t genetic_algorithm;

boolean assign_score(population *pop, entity *indiv)
{
    boolean ret;
    double objective;

    // GAUL maximizes the fitness, the problem is of minimization, so:
    objective = objective_function_p(indiv->chromosome[0]);
    ret = ga_entity_set_fitness(indiv, 1 / (1 + objective));   // fitness in the interval (0, 1]

    genetic_algorithm.stats.fitness_evals += 1;
    if (objective < genetic_algorithm.stats.best_fitness)
        genetic_algorithm.stats.best_fitness = objective;

    return ret;
}

void genetic_algorithm_init(config_t *config)
{
    population *pop;
    int i, population_size, allele_min, allele_max, number_of_dimensions;
    double aux;

    parallel_evolution_config_lookup_int(config,
            "genetic_algorithm.max_generations",
            &genetic_algorithm.max_generations);
    parallel_evolution_config_lookup_int(config,
            "genetic_algorithm.population_size",
            &population_size);
    number_of_dimensions = parallel_evolution_get_number_of_dimensions();

    pop = ga_genesis_double(
            population_size,            /* const int              population_size */
            1,                          /* const int              num_chromo */
            number_of_dimensions,       /* const int              len_chromo */
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

    allele_min = parallel_evolution_get_limit_min(0);
    allele_max = parallel_evolution_get_limit_max(0);
    for (i = 1; i < number_of_dimensions; ++i) {
        aux = parallel_evolution_get_limit_min(i);
        if (aux < allele_min)
            allele_min = aux;

        aux = parallel_evolution_get_limit_max(i);
        if (aux > allele_max)
            allele_max = aux;
    }
    ga_population_set_allele_min_double(pop, allele_min);
    ga_population_set_allele_max_double(pop, allele_max);

    genetic_algorithm.population = pop;
    genetic_algorithm.stats.best_fitness = DBL_MAX; // it's a minimization problem
}

void genetic_algorithm_run_iterations(int generations)
{
    genetic_algorithm.stats.iterations += ga_evolution(genetic_algorithm.population, generations);
}

void genetic_algorithm_insert_migrant(migrant_t *migrant)
{
    entity *new, *old;
    population *pop;
    int i;

    pop = genetic_algorithm.population;

    // create new entity
    new = (entity *)malloc(sizeof(entity));

    /* Allocate chromosome structures. */
    new->chromosome = (void **)malloc(sizeof(double *));
    new->chromosome[0] = (double *)malloc(parallel_evolution_get_number_of_dimensions() * sizeof(double));
    //  pop->chromosome_constructor(pop, new);

    /* Physical characteristics currently undefined. */
    new->data=NULL;

    /* No fitness evaluated yet. */
    new->fitness = GA_MIN_FITNESS;

    // asign chromossome
    for (i = 0; i < migrant->var_size; ++i) {
        ((double *)new->chromosome[0])[i] = migrant->var[i];
    }
    assign_score(pop, new);

    // insert into population
    old = ga_get_entity_from_rank(pop, pop->size - 1);
    if (new->fitness > old->fitness) {
        ga_entity_blank(pop, old);
        ga_entity_copy(pop, old, new);
        ga_population_score_and_sort(pop);
    }
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
    population *pop;
    int i;

    pop = genetic_algorithm.population;

    if (population_create(pop2send, pop->size) != SUCCESS)
        return FAIL;

    for (i = 0; i < pop->size; ++i) {
        (*pop2send)->individuals[i]->var = (double *)(pop->entity_iarray[i]->chromosome[0]);
        (*pop2send)->individuals[i]->var_size = parallel_evolution_get_number_of_dimensions();
    }
    (*pop2send)->stats = &(genetic_algorithm.stats);

    return SUCCESS;
}

algorithm_stats_t *genetic_algorithm_get_stats()
{
    return &genetic_algorithm.stats;
}

int main(int argc, char *argv[])
{
    algorithm_t *genetic_algorithm;
    int ret;
    unsigned int seed;

    algorithm_create(&genetic_algorithm,
            genetic_algorithm_init,             // a wrapper around ga_genesis_double()
            genetic_algorithm_run_iterations,   // make a wrapper around ga_evolution()
            genetic_algorithm_insert_migrant,   // a wrapper around ga_replace_by_fitness(population *pop, entity *child);
            genetic_algorithm_pick_migrant,     // a wrapper around ga_get_entity_from_rank(pop,0)
            genetic_algorithm_ended,
            genetic_algorithm_get_population,   // the population that will be sent to the main node
            genetic_algorithm_get_stats);
    parallel_evolution_set_algorithm(genetic_algorithm);

    random_init();
    getrandom(&seed, sizeof(unsigned int), 0);
    random_seed(seed);
    ret = parallel_evolution_run(&argc, &argv);

    algorithm_destroy(&genetic_algorithm);

    return ret;
}
