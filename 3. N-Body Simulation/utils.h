#pragma once

enum quadrant {
	/* the quadrant can be defined as below:
	 *
	 *				|
	 *		TL		|		TR
	 *______________|________________
	 *				|
	 *		BL		|		BR
	 *				|
	 */
	TR,
	TL,
	BL,
	BR
};

struct body {
	double m;
	double x, y;
	double vx, vy;
	double fx, fy;
};

struct node {
	double totalmass;
	double centerx, centery;
	double xmin, xmax;
	double ymin, ymax;
	double xmid, ymid;
	struct body * virtual_body;  // virtual body is the approximation of its leaf bodies
	struct node * top_right_tree;
	struct node * top_left_tree;
	struct node * bot_left_tree;
	struct node * bot_right_tree;
};

 enum quadrant Get_Quadrant(double x, double y, double xmin, double xmax, double ymin, double ymax);
 void Update_Center_Mass(struct node * nodep, struct body * virtual_body);
 struct node *Create_Node(struct body * virtual_body, double xmin, double xmax, double ymin, double ymax);
 void Insert_Body(struct body * insbody, struct node * nodep);
 void Calculate_force(struct node * nodep, struct body * virtual_body, double threshold);
 void Destroy_Tree(struct node * nodep);
