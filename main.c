#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "index_PTM.h"

#define MAX_NBRS 14


static int read_file(const char* path, uint8_t** p_buf, size_t* p_fsize)
{
	size_t fsize = 0, num_read = 0;
	uint8_t* buf = NULL;

	FILE* fin = fopen(path, "rb");
	if (fin == NULL)
		return -1;

	int ret = fseek(fin, 0, SEEK_END);
	if (ret != 0)
		goto cleanup;

	fsize = ftell(fin);
	ret = fseek(fin, 0, SEEK_SET);
	if (ret != 0)
		goto cleanup;

	buf = (uint8_t*)malloc(fsize);
	if (buf == NULL)
	{
		ret = -1;
		goto cleanup;
	}

	num_read = fread(buf, 1, fsize, fin);
	if (num_read != fsize)
	{
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (ret != 0)
	{
		free(buf);
		buf = NULL;
	}

	*p_fsize = fsize;
	*p_buf = buf;
	fclose(fin);
	return ret;
}

static void get_neighbours(double (*positions)[3], int32_t* nbrs, int max_nbrs, int i, double (*nbr)[3])
{
	memcpy(nbr[0], positions[i], 3 * sizeof(double));

	for (int j=0;j<max_nbrs;j++)
	{
		int index = nbrs[i * max_nbrs + j];
		memcpy(nbr[j+1], positions[index], 3 * sizeof(double));
	}
}

int main()
{
	size_t fsize = 0;
	int32_t* nbrs = NULL;
	double* positions = NULL;
	int ret = read_file((char*)"FeCu_positions.dat", (uint8_t**)&positions, &fsize);
	if (ret != 0)
		return -1;

	ret = read_file((char*)"FeCu_nbrs.dat", (uint8_t**)&nbrs, &fsize);
	if (ret != 0)
		return -1;

	const int max_nbrs = MAX_NBRS;
	int num_atoms = fsize / (max_nbrs * sizeof(int32_t));

	initialize_PTM();
	int8_t* types = (int8_t*)calloc(sizeof(int8_t), num_atoms);
	double* rmsds = (double*)calloc(sizeof(double), num_atoms);
	double* quats = (double*)calloc(4 * sizeof(double), num_atoms);
	int counts[6] = {0};

	double rmsd_sum = 0.0;
	for (int j=0;j<10;j++)
	for (int i=0;i<num_atoms;i++)
	{
		double nbr[max_nbrs+1][3];
		get_neighbours((double (*)[3])positions, nbrs, max_nbrs, i, nbr);

		int32_t type, alloy_type;
		double scale, rmsd;
		double q[4], F[9], F_res[3], U[9], P[9];
		index_PTM(max_nbrs + 1, nbr[0], NULL, PTM_CHECK_ALL, &type, &alloy_type, &scale, &rmsd, q, F, F_res, U, P);

		types[i] = type;
		rmsds[i] = rmsd;
		memcpy(&quats[i*4], q, 4 * sizeof(double));

		counts[type]++;
		if (type != PTM_MATCH_NONE)
			rmsd_sum += rmsd;

		if (i % 1000 == 0 || i == num_atoms - 1)
		{
			printf("counts: [");
			for (int j = 0;j<6;j++)
				printf("%d ", counts[j]);
			printf("]\n");
		}
	}

	printf("rmsd sum: %f\n", rmsd_sum);


	free(types);
	free(rmsds);
	free(quats);
	free(positions);
	free(nbrs);
	return 0;
}
