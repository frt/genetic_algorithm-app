#include "topology_parser.h"
#include <stdio.h>

status_t parse_a_line(topology_t *topology, FILE *fp)
{
	int node_id, adjacent_node_id;
	int ret;
	int c;

	/* get node id */
	ret = fscanf(fp, " %d", &node_id);
	if (ret != 1)
		return FAIL;
	ret = topology_add_node(topology, node_id);
	if (ret != SUCCESS)
		return FAIL;

	/* get adjacencies id */
	while (1) {
		c = fgetc(fp);
		if (c == ' ' || c == '\t')
			continue;

		if (c == '\n' || c == EOF)
			break;

		ungetc(c, fp);

		ret = fscanf(fp, "%d", &adjacent_node_id);
		if (ret != 1)
			return FAIL;

		ret = topology_add_adjacency(topology, node_id, adjacent_node_id);
		if (ret != SUCCESS)
			return FAIL;
	}
	return SUCCESS;
}

status_t topology_parser_parse(topology_t *topology, const char *filename)
{
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL)
		return FAIL;

	while (1) {
		if (ferror(fp)) {
			fclose(fp);
			return FAIL;
		}
		if (feof(fp)) {
			fclose(fp);
			return SUCCESS;
		}
		parse_a_line(topology, fp);
	}
}
