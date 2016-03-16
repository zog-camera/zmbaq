/**
  Origin https://github.com/JVanDamme/ProgressiveDelaunay
  Modified.
 */
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "Triangulation.h"

namespace JVa {
using namespace std;

/**
 *  Checks the position of the point 'p' respect the segment formed by the points 'a' and 'b'.
 */
double orientation2D(const Vector2& a, const Vector2& b, const Vector2& p)
{
    return ((b[X] - a[X])*(p[Y] - a[Y]) - (p[X] - a[X])*(b[Y] - a[Y]))/2;
}

double orientation2D(const Vector3& a, const Vector3& b, const Vector3& p)
{
    return ((b[X] - a[X])*(p[Y] - a[Y]) - (p[X] - a[X])*(b[Y] - a[Y]))/2;
}

/**
 *  Checks if the test point 't' lies inside the circle formed by the points 'p', 'q' and 'r'.
 */
double orientation25D(const Vector2& p, const Vector2& q, const Vector2& r, const Vector2& t)
{
    //p q r t
    //a b c x
    //a1 a2 a3
    //b1 b2 b3
    //c1 c2 c3
    
    double a1 = (q[X] - p[X]);
    double b1 = (r[X] - p[X]);
    double c1 = (t[X] - p[X]);
    double a2 = (q[Y] - p[Y]);
    double b2 = (r[Y] - p[Y]);
    double c2 = (t[Y] - p[Y]);
    double a3 = a1*(q[X] + p[X]) + a2 * (q[Y] + p[Y]);
    double b3 = b1*(r[X] + p[X]) + b2 * (r[Y] + p[Y]);
    double c3 = c1*(t[X] + p[X]) + c2 * (t[Y] + p[Y]);
    
    return a1*(b2*c3 - c2*b3) - (a2*(b1*c3 - c1*b3)) + a3*(b1*c2 - c1*b2);
}

/**
 *  Checks if the point 'p' is inside the triangle formed by 'a', 'b' and 'c'.
 */
inline bool isInside(const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &p)
{   
    double d = orientation2D(a, b, p);
    double e = orientation2D(b, c, p);
    double f = orientation2D(c, a, p);
    
    bool s1 = d < 0;
    bool s2 = e < 0;
    bool s3 = f < 0;
    bool s4 = d > 0;
    bool s5 = e > 0;
    bool s6 = f > 0;

    return (s1 == s2 && s2 == s3) || (s4 == s5 && s5 == s6);
}

/**
 *  Get the enclosing triangle
 */
void enclosingTriangle(const Vector3* p, int len, Vector3 &a, Vector3 &b, Vector3 &c)
{
    int n = len;
    double xmin, xmax, ymin, ymax;
    
    xmin = xmax = p[0][X];
    ymin = ymax = p[0][Y];
    
    for(int i = 1; i < n; ++i)
    {
	if(p[i][X] < xmin) xmin = p[i][X];
	if(p[i][X] > xmax) xmax = p[i][X];
	if(p[i][Y] < ymin) ymin = p[i][Y];
	if(p[i][Y] > ymax) ymax = p[i][Y];
    }
    
    double offset = 1000;
    xmin = xmin - offset;
    ymin = ymin - offset;
    xmax = xmax + offset;
    ymax = ymax + offset;
    
    //a
    a = Vector3(xmin, ymin, 0);
    
    //b
    double k = xmax - xmin;
    double i = k / cos(45);
    double j = i * sin(45);
    b = Vector3(xmax + k, ymin, 0);
    
    //c
    j = ymax - ymin;
    i = j / sin(45);
    k = i * cos(45);
    c = Vector3(xmin, ymax + j, 0);
}

/**
 *  Tests segment-segment intersection
 * 
 * @Returns
 * 	2 in case of segment-segment intersection
 *	1 in case of segment-endpoint or collinear segments
 * 	0 in case of no intersection
 */
int intersects(const Vector3 &a1, const Vector3 &a2, const Vector3 &b1, const Vector3 &b2)
{
    double res1 = orientation2D(a1, a2, b1) * orientation2D(a1, a2, b2);
    double res2 = orientation2D(b1, b2, a1) * orientation2D(b1, b2, a2);

    /*
     * Segment-segment intersection
     */
    if(res1 < 0 && res2 < 0)
	return 2;
    
    /*
     * Segment-endpoint intersection or collinear
     */
    if((res1 == 0 && res2 < 0) || (res1 < 0 && res2 == 0) || (res1 == 0 && res2 == 0))
	return 1;
    
    /*
     * No intersection
     */
    return 0;
}

/**
 * Delaunay test and edge remaking
 */
inline void rend_triangle(int point, int face_rear, DCEL &edge1,
                          face* faces, vertex* v, DCEL* edges)
{
    if (edge1.fL == -1 || edge1.fR == -1)
        return;
    
    /**
     *  Select edges from the incident triangle and adyacent triangle
     */
    int face_next = (edge1.fL == face_rear) ? edge1.fR : edge1.fL;
    bool edge1_cclockwise = (edge1.fL == face_rear) ? true : false;
    
    DCEL *edge3_rear = (edge1_cclockwise) ? &edges[edge1.eP] : &edges[edge1.eN];
    bool edge3_rear_cclockwise = (edge3_rear->fL == face_rear) ? true : false;
    
    DCEL *edge2_rear = (edge3_rear_cclockwise) ? &edges[edge3_rear->eP] : &edges[edge3_rear->eN];
    bool edge2_rear_cclockwise = (edge2_rear->fL == face_rear) ? true : false;
    
    DCEL *edge3_next = (!edge1_cclockwise) ? &edges[edge1.eP] : &edges[edge1.eN];
    bool edge3_next_cclockwise = (edge3_next->fL == face_next) ? true : false;
    
    DCEL *edge2_next = (edge3_next_cclockwise) ? &edges[edge3_next->eP] : &edges[edge3_next->eN];
    bool edge2_next_cclockwise = (edge2_next->fL == face_next) ? true : false;

    /**
     * Select third point and make a circle and orientation tests
     */
    Vector2 point1 = v[edge1.vB].getVector2();
    Vector2 point2 = v[edge1.vE].getVector2();
    int point3 = (edge3_next_cclockwise) ? edge3_next->vB : edge3_next->vE;
    
    bool circle_clockwise     = orientation2D( point1, point2, v[point3].getVector2()) < 0;
    double circle_orientation = orientation25D(point1, point2, v[point3].getVector2(), v[point].getVector2());
    
    /**
     *  Modify edges structure with new edge and new links
     */
    if ( !(circle_orientation < 0 && !circle_clockwise || circle_orientation > 0 && circle_clockwise) )
        return;
    edges[edge1.e] = DCEL(edge1.e, point, point3, face_next, face_rear, edge2_rear->e, edge2_next->e);
    DCEL *edge1_new = &edges[edge1.e];

    faces[face_rear] = face(edge1_new->e);
    faces[face_next] = face(edge1_new->e);

    edge2_rear->eP = (edge2_rear_cclockwise) ? edge3_next->e : edge2_rear->eP;
    edge2_rear->eN = (edge2_rear_cclockwise) ? edge2_rear->eN : edge3_next->e;
    edge2_rear->fL = (edge2_rear_cclockwise) ? face_next : edge2_rear->fL;
    edge2_rear->fR = (edge2_rear_cclockwise) ? edge2_rear->fR : face_next;

    edge3_rear->eP = (edge3_rear_cclockwise) ? edge1_new->e : edge3_rear->eP;
    edge3_rear->eN = (edge3_rear_cclockwise) ? edge3_rear->eN : edge1_new->e;
    edge3_rear->fL = (edge3_rear_cclockwise) ? face_rear : edge3_rear->fL;
    edge3_rear->fR = (edge3_rear_cclockwise) ? edge3_rear->fR : face_rear;

    edge2_next->eP = (edge2_next_cclockwise) ? edge3_rear->e : edge2_next->eP;
    edge2_next->eN = (edge2_next_cclockwise) ? edge2_next->eN : edge3_rear->e;
    edge2_next->fL = (edge2_next_cclockwise) ? face_rear : edge2_next->fL;
    edge2_next->fR = (edge2_next_cclockwise) ? edge2_next->fR : face_rear;

    edge3_next->eP = (edge3_next_cclockwise) ? edge1_new->e : edge3_next->eP;
    edge3_next->eN = (edge3_next_cclockwise) ? edge3_next->eN : edge1_new->e;
    edge3_next->fL = (edge3_next_cclockwise) ? face_next : edge3_next->fL;
    edge3_next->fR = (edge3_next_cclockwise) ? edge3_next->fR : face_next;

    /**
     *  Adjacent triangles Delaunay correctness check and repairing if needed
     */
    rend_triangle(point, ((edge3_next_cclockwise) ? edge3_next->fL : edge3_next->fR), *edge3_next, faces, v, edges);
    rend_triangle(point, ((edge2_next_cclockwise) ? edge2_next->fL : edge2_next->fR), *edge2_next, faces, v, edges);
}

/**
 *  Progressive triangulation function of the points placed in 'ps' array
 */
std::shared_ptr<Result> triangulate(const Vector3 *ps, int len)
{
    /**
     *  Begining
     */
    int n = len;
    std::shared_ptr<Result> res = std::make_shared<Result>(n);
    face* faces = &(res->faces[0]);
    vertex* vertices = &(res->vertices[0]);
    DCEL* edges = &(res->edges[0]);;

    /**
     *  Initialization
     */
    Vector3 p1, p2, p3;
    enclosingTriangle(ps, len, p1, p2, p3);
    
    vertices[0].p = p1;
    vertices[1].p = p2;
    vertices[2].p = p3;
    
    // infinite = -1
    edges[0].e  =  0;
    edges[0].vB =  0;
    edges[0].vE =  1;
    edges[0].fL =  0;
    edges[0].fR = -1; // No face
    edges[0].eP =  2;
    edges[0].eN =  1;
    edges[1].e  =  1;
    edges[1].vB =  1;
    edges[1].vE =  2;
    edges[1].fL =  0;
    edges[1].fR = -1;
    edges[1].eP =  0;
    edges[1].eN =  2;
    edges[2].eP =  1;
    edges[2].e  =  2;
    edges[2].vB =  2;
    edges[2].vE =  0;
    edges[2].fL =  0;
    edges[2].fR = -1;
    edges[2].eN =  0;
    
    faces[0] = face(0);
    
    int faces_pointer    = 1;
    int edges_pointer    = 3;
    int vertices_pointer = 3;
    
    /**
     * Main loop
     */
    
    Vector3 vertex1, vertex2, vertex3, pivot;
    DCEL *edge1 = 0, *edge2 = 0, *edge3 = 0;
    bool edge1_cclockwise = false, edge2_cclockwise = false,
            edge3_cclockwise = false, found = false;
    int point1 = 0, point2 = 0, point3 = 0, walker = 0, rear = 0;
    
    pivot = vertices[edges[0].vB].p;
    
    for (int i = 0; i < n; ++i)
    {
        rear   = -1;
        walker = edges[0].fL;
        found  = false;

        /** Triangle location algorithm */
        while (!found)
        {
            edge1 = &edges[faces[walker].idx];
            edge1_cclockwise = (edge1->fL == walker) ? true : false;

            edge3 = (edge1_cclockwise) ? &edges[edge1->eP] : &edges[edge1->eN];
            edge3_cclockwise = (edge3->fL == walker) ? true : false;

            edge2 = (edge3_cclockwise) ? &edges[edge3->eP] : &edges[edge3->eN];
            edge2_cclockwise = (edge2->fL == walker) ? true : false;

            point1 = (edge1_cclockwise) ? edge1->vB : edge1->vE;
            point2 = (edge2_cclockwise) ? edge2->vB : edge2->vE;
            point3 = (edge3_cclockwise) ? edge3->vB : edge3->vE;

            vertex1 = vertices[point1].p;
            vertex2 = vertices[point2].p;
            vertex3 = vertices[point3].p;

            if (isInside(vertex1, vertex2, vertex3, ps[i]))
            {
                found = true;
            }
            else
            {
                /** If not found then look for segment-segment intersection */
                int aux1 = (edge1->fL == rear || edge1->fR == rear) ? 0 : intersects(vertex1, vertex2, ps[i], pivot);
                int aux2 = (edge2->fL == rear || edge2->fR == rear) ? 0 : intersects(vertex2, vertex3, ps[i], pivot);
                int aux3 = (edge3->fL == rear || edge3->fR == rear) ? 0 : intersects(vertex3, vertex1, ps[i], pivot);
                int next = (aux1 == 2)*edge1->e | (aux2 == 2)*edge2->e | (aux3 == 2)*edge3->e;

                if (next == 0)
                {
                    /** If not segment-segment then randomize the next step (if valid) */
                    int random;
                    next = -1;

                    while (next == -1)
                    {
                        random = rand()%3;
                        if (aux1 == 1 && random == 0) next = edge1->e;
                        if (aux2 == 1 && random == 1) next = edge2->e;
                        if (aux3 == 1 && random == 2) next = edge3->e;
                    }
                }

                rear   = walker;
                walker = (walker == edges[next].fL) ? edges[next].fR : edges[next].fL;
            }
        }

        /** Creating 3 new triangles inside a triangle */
        vertices[vertices_pointer] = vertex(ps[i]);

        edges[edges_pointer]   = DCEL(edges_pointer,   vertices_pointer, point1,          walker, faces_pointer+1, edges_pointer+1, edge3->e);
        edges[edges_pointer+1] = DCEL(edges_pointer+1, vertices_pointer, point2,   faces_pointer,          walker, edges_pointer+2, edge1->e);
        edges[edges_pointer+2] = DCEL(edges_pointer+2, vertices_pointer, point3, faces_pointer+1,   faces_pointer,   edges_pointer, edge2->e);

        edge1->eP = (edge1_cclockwise) ? edges_pointer : edge1-> eP;
        edge1->eN = (edge1_cclockwise) ? edge1->eN : edges_pointer;
        edge2->eP = (edge2_cclockwise) ? edges_pointer+1 : edge2->eP;
        edge2->eN = (edge2_cclockwise) ? edge2->eN : edges_pointer+1;
        edge2->fL = (edge2_cclockwise) ? faces_pointer : edge2->fL;
        edge2->fR = (edge2_cclockwise) ? edge2->fR : faces_pointer;
        edge3->eP = (edge3_cclockwise) ? edges_pointer+2 : edge3->eP;
        edge3->eN = (edge3_cclockwise) ? edge3->eN : edges_pointer+2;
        edge3->fL = (edge3_cclockwise) ? faces_pointer+1 : edge3->fL;
        edge3->fR = (edge3_cclockwise) ? edge3->fR : faces_pointer+1;

        faces[faces_pointer]   = face(edge2->e);
        faces[faces_pointer+1] = face(edge3->e);

        /** Delaunay correctness check and repairing if needed  */
        rend_triangle(vertices_pointer, walker, *edge1, faces, vertices, edges);
        rend_triangle(vertices_pointer, faces_pointer, *edge2, faces, vertices, edges);
        rend_triangle(vertices_pointer, faces_pointer+1, *edge3, faces, vertices, edges);

        /* Empty element pointer */
        edges_pointer += 3;
        faces_pointer += 2;
        vertices_pointer += 1;
    }
    
    /** Destroying every enclosing triangle edge (-1 to vB) */
    for(int i = 0; i < edges_pointer; ++i)
    {
        edge1  = &edges[i];
        point1 = edge1->vB;
        point2 = edge1->vE;

        if (point1 == 0 || point2 == 0) edge1->vB = -1;
        if (point1 == 1 || point2 == 1) edge1->vB = -1;
        if (point1 == 2 || point2 == 2) edge1->vB = -1;
    }
    return res;
}

}//namespae JVa

