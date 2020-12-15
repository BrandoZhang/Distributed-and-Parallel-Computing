#ifndef _MODELS
#define _MODELS

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "const.h"

#define legal(x, n) ( (x)>=0 && (x)<(n) )

typedef struct TemperatureField
{
	int x, y;  // #rows, #columns
	double **t;  // pointer to a column of temperatures
	double *storage;
}TemperatureField;

void deleteField(TemperatureField *field)
{
	free(field->t);
	free(field->storage);
	//free(field);
}

void newField(TemperatureField *field, int x, int y, int sourceX, int sourceY)
{
	TemperatureField temp = *field;
	field->storage = malloc(sizeof(double) * x * y);
	field->t = malloc(sizeof(double*) * x);
	field->x = x;
	field->y = y;
	int i, j;
	for (i = 0; i < x; ++i)
		field->t[i] = &field->storage[i*y];
	if (sourceX)
	{
		double scaleFactorX = (double)sourceX / x;
		double scaleFactorY = (double)sourceY / y;
		for (i = 0; i < x; ++i)
			for (j = 0; j < y; ++j)
				field->t[i][j] = temp.t[(int)(i*scaleFactorX)][(int)(j*scaleFactorY)];
		deleteField(&temp);
	}
	else memset(field->storage, 0, sizeof(double)*x*y);
}

void maintainFireplace(TemperatureField *field)
{
	/* Maintain the fireplace to FIRE_TEMP */
	int i;
	for (i = field->y * 0.3; i < field->y * 0.7; i++)
		field->t[0][i] = FIRE_TEMP;
}

void maintainField(TemperatureField *field, int x)
{
	int i;
	/* Maintain the walls to ROOM_TEMP */
	for (i = 0; i < field->y; i++)
	{
		field->t[0][i] = ROOM_TEMP;
		field->t[x - 1][i] = ROOM_TEMP;
	}
	for (i = 0; i < field->x; i++)
	{
		field->t[i][0] = ROOM_TEMP;
		field->t[i][field->y - 1] = ROOM_TEMP;
	}
	/* Maintain the fireplace to FIRE_TEMP */
	for (i = field->y * 0.3; i < field->y * 0.7; i++)
		field->t[0][i] = FIRE_TEMP;
}

void initField(TemperatureField *field)
{
	/* Initialize the field to room temperature with a fireplace */
	int i, j;
	for (i = 0; i < field->x; i++)
		for (j = 0; j < field->y; j++)
			field->t[i][j] = ROOM_TEMP;
	maintainFireplace(field);
}

#endif
