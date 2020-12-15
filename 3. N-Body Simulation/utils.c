#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

const double G = 6;  // gravitational costant

enum quadrant Get_Quadrant(double x, double y, double xmin, double xmax, double ymin, double ymax) {

	double midx, midy;

	midx = 0.5*(xmin + xmax);
	midy = 0.5*(ymin + ymax);

	if (x < midx) {
		if (y < midy) {
			return TL;
		}
		else {
			return BL;
		}
	}
	else {
		if (y < midy) {
			return TR;
		}
		else {
			return BR;
		}
	}
}

void Update_Center_Mass(struct node * nodep, struct body * virtual_body) {
	nodep->totalmass += virtual_body->m;
	nodep->centerx = (nodep->totalmass*nodep->centerx + virtual_body->m*virtual_body->x) / (nodep->totalmass + virtual_body->m);
	nodep->centery = (nodep->totalmass*nodep->centery + virtual_body->m*virtual_body->y) / (nodep->totalmass + virtual_body->m);
}

struct node *Create_Node(struct body * virtual_body, double xmin, double xmax, double ymin, double ymax) {

	struct node* nodep;
	nodep = malloc(sizeof(struct node));

	nodep->totalmass = virtual_body->m;
	nodep->centerx = virtual_body->x;
	nodep->centery = virtual_body->y;
	nodep->xmin = xmin;
	nodep->xmax = xmax;
	nodep->ymin = ymin;
	nodep->ymax = ymax;
	nodep->xmid = 0.5 * (xmin + xmax);
	nodep->ymid = 0.5 * (ymin + ymax);
	nodep->virtual_body = virtual_body;
	nodep->top_right_tree = NULL;
	nodep->top_left_tree = NULL;
	nodep->bot_left_tree = NULL;
	nodep->bot_right_tree = NULL;

	return nodep;
}

void Insert_Body(struct body * insbody, struct node * nodep) {

	enum quadrant insquad;
	insquad = Get_Quadrant(insbody->x, insbody->y, nodep->xmin, nodep->xmax, nodep->ymin, nodep->ymax);
	Update_Center_Mass(nodep, insbody);

	if (nodep->virtual_body != NULL) {
		if (nodep->virtual_body->x == insbody->x && nodep->virtual_body->y == insbody->y) {
			nodep->virtual_body->m += insbody->m;
			nodep->virtual_body->vx = (nodep->virtual_body->m*nodep->virtual_body->vx + insbody->m*insbody->vx) / (nodep->virtual_body->m + insbody->m);
			nodep->virtual_body->vy = (nodep->virtual_body->m*nodep->virtual_body->vy + insbody->m*insbody->vy) / (nodep->virtual_body->m + insbody->m);
		}
		else {
			enum quadrant myquad;
			myquad = Get_Quadrant(nodep->virtual_body->x, nodep->virtual_body->y, nodep->xmin, nodep->xmax, nodep->ymin, nodep->ymax);

			if (myquad == TL) {
				nodep->top_left_tree = Create_Node(nodep->virtual_body, nodep->xmin, nodep->xmid, nodep->ymin, nodep->ymid);
			}
			else if (myquad == TR) {
				nodep->top_right_tree = Create_Node(nodep->virtual_body, nodep->xmid, nodep->xmax, nodep->ymin, nodep->ymid);
			}
			else if (myquad == BL) {
				nodep->bot_left_tree = Create_Node(nodep->virtual_body, nodep->xmin, nodep->xmid, nodep->ymid, nodep->ymax);
			}
			else {
				nodep->bot_right_tree = Create_Node(nodep->virtual_body, nodep->xmid, nodep->xmax, nodep->ymid, nodep->ymax);
			}

			nodep->virtual_body = NULL;
		}
	}

	if (nodep->virtual_body == NULL) {
		if (insquad == TL) {
			if (nodep->top_left_tree == NULL) {
				nodep->top_left_tree = Create_Node(insbody, nodep->xmin, nodep->xmid, nodep->ymin, nodep->ymid);
			}
			else {
				Insert_Body(insbody, nodep->top_left_tree);
			}
		}
		else if (insquad == TR) {
			if (nodep->top_right_tree == NULL) {
				nodep->top_right_tree = Create_Node(insbody, nodep->xmid, nodep->xmax, nodep->ymin, nodep->ymid);
			}
			else {
				Insert_Body(insbody, nodep->top_right_tree);
			}
		}
		else if (insquad == BL) {
			if (nodep->bot_left_tree == NULL) {
				nodep->bot_left_tree = Create_Node(insbody, nodep->xmin, nodep->xmid, nodep->ymid, nodep->ymax);
			}
			else {
				Insert_Body(insbody, nodep->bot_left_tree);
			}
		}
		else if (insquad == BR) {
			if (nodep->bot_right_tree == NULL) {
				nodep->bot_right_tree = Create_Node(insbody, nodep->xmid, nodep->xmax, nodep->ymid, nodep->ymax);
			}
			else {
				Insert_Body(insbody, nodep->bot_right_tree);
			}
		}
	}
}

void Calculate_force(struct node * nodep, struct body * virtual_body, double theta) {
	double dx, dy, r, fx, fy, d;

	dx = nodep->centerx - virtual_body->x;
	dy = nodep->centery - virtual_body->y; 
	r = sqrt(pow(dx, 2) + pow(dy, 2));
	d = nodep->xmax - nodep->xmin;

	if ((d / r < theta || nodep->virtual_body != NULL) && virtual_body != nodep->virtual_body) {

		fx = G * nodep->totalmass*virtual_body->m*dx / pow(r + 3, 3);
		fy = G * nodep->totalmass*virtual_body->m*dy / pow(r + 3, 3);

		virtual_body->fx += fx;
		virtual_body->fy += fy;
	}
	else {
		if (nodep->top_left_tree != NULL) Calculate_force(nodep->top_left_tree, virtual_body, theta);
		if (nodep->top_right_tree != NULL) Calculate_force(nodep->top_right_tree, virtual_body, theta);
		if (nodep->bot_left_tree != NULL) Calculate_force(nodep->bot_left_tree, virtual_body, theta);
		if (nodep->bot_right_tree != NULL) Calculate_force(nodep->bot_right_tree, virtual_body, theta);
	}
}

void Destroy_Tree(struct node * nodep) {
	if (nodep != NULL) {
		if (nodep->top_left_tree != NULL) Destroy_Tree(nodep->top_left_tree);
		if (nodep->top_right_tree != NULL) Destroy_Tree(nodep->top_right_tree);
		if (nodep->bot_left_tree != NULL) Destroy_Tree(nodep->bot_left_tree);
		if (nodep->bot_right_tree != NULL) Destroy_Tree(nodep->bot_right_tree);
		free(nodep);
	}
}