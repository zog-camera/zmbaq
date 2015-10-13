================
Copied from https://github.com/JVanDamme/ProgressiveDelaunay
================
About
================
Progressive Delaunay is an algorithm that triangulates a set of points in 2D environments. This algorithm was designed to achieve a lower memory footprint. The result is a table named DCEL which contains all of ready to draw edges.

Memory structures
================
There are 3 important structs to take into account, because you can imagine what `Vector3` and `Vector2` structs are. These structs will allow you to define and move through the current set of triangles. The first one is named `face` and contains all faces (triangles) and a reference to a single edge, which is used to form the triangle. The second one is named `vertex` and contains the coordinates of every point and a reference to an edge which passes through. The last one, called `DCEL`, contains all the edges as the references of the faces of both sides and bounding vertices. 

You can see a representation in the following figure.

![alt tag](https://github.com/JVanDamme/ProgressiveDelaunay/blob/master/img/DCEL.jpg)

Delaunay correctness
================
The Delaunay property must hold at each step of the algorithm, each time that one point `p` is inserted in the triangulation, the algorithm recursively checks, for each triangle `t` incident to `p` whether or not `p` lies in the circumcircle of the triangle adjacent to `t` through the edge opposite to `p`. If it does, the common edge of the two triangles is replaced by the edge connecting `p` with the opposite vertex in the other triangle. In the figure, `p` is interior to the circumcircle of the triangle `abc`: in this case, the edge `ab` is replaced by the edge `pc`. This test is repeated for all the triangles incident to `p` which appear after the edge flips. A new point can be inserted in the triangulation only when `p` does not lie in the interior of any of the circumcircles of the triangles that are adjacent to the triangles incident to `p`.

![alt tag](https://github.com/JVanDamme/ProgressiveDelaunay/blob/master/img/correctness.jpg)

Circumference lying check
================
The points of the triangle `abc` are projected onto paraboloid `z = x^2 + y^2`. The plane formed by the new projected vectors `ab` and `ac` is made and point `p` is projected too. If `p` lies inside the circle formed by `abc`, it will be inside the belly of the paraboloid appeared through the plane, which means that `p` lies in the inferior (or equal) half-space determined by the plane. Then, is inside of the circle if and only if `det(x, a, b, c) <= 0`, because this opperation gets the volume of the tetrahedron formed by the projected vectors `ab`, `ac` and `ax` according to the right hand rule. 

It looks clearer in the following figure.

![alt tag](https://github.com/JVanDamme/ProgressiveDelaunay/blob/master/img/paraboloid.jpg)

Point location: walking in a triangulation
================
The method used to locating points in triangulations is a rectillinear walk. From a known vertex `q` the algorithm visits all triangles intersected by the segment `pq`, where `p` is the new point to add. In this implementation the known vertex is named `pivot`.

![alt tag](https://github.com/JVanDamme/ProgressiveDelaunay/blob/master/img/walk.jpg)

Notes
================
- Just use the function `void triangulate(const vector<Vector3>& ps)` where `ps` is an array of points.
- The way to access to `Vector3` or `Vector2` through the operator `[]` is only a patch because these structs allows to access directly by name of variable. But it's a patch that may helps to port easily to some vector classes implementations, i.e. Glut or CGAL.
- Collinear or any 4 concyclic points aren't supported yet.
