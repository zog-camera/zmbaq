/**
  Origin https://github.com/JVanDamme/ProgressiveDelaunay
  Modified.
 */
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <vector>
#include <memory>

namespace JVa
{
typedef glm::dvec3 Vector3;
typedef glm::dvec2 Vector2;

enum COORD { X, Y, Z, W };

struct face
{
    int idx;
    face() : idx(-1) { }
    face(int ie)  { idx = ie; }
};

struct vertex
{
    Vector3 p;

    vertex() { }
    vertex(Vector3 ip) : p(ip) { }

    Vector2 getVector2() const {
        return Vector2(p[(glm::length_t)X], p[(glm::length_t)Y]);
    }
};

/**
@verbatim
              / eN(next edge)
             /
            + vE
   \  fL   /
eP  \     / e (edge)
     \   /
      \ /     fR (infty, if boundary)
       + vB


@endverbatim

*/
struct DCEL
{
    int e; //edge number
    int vB;//vertex1
    int vE;//vertex2
    int fL;//face left
    int fR;//face right
    int eP;//previous attached edge
    int eN;//next attached edge

    DCEL() {e = vB = vE = fL = fR = eP = eN = -1;}

    DCEL(int ie, int ivB, int ivE, int ifL, int ifR, int ieP, int ieN)
    { e = ie; vB = ivB; vE = ivE; fL = ifL; fR = ifR; eP = ieP; eN = ieN; }
};

class Result
{
public:
    typedef std::vector<face> TContainerFaces;
    typedef std::vector<vertex> TContainerVertices;
    typedef std::vector<DCEL> TContainerDCEL;

    Result(): n(0) { }
    Result(int iN) { setup(iN);}
    void setup(int iN)
    {
        n = iN;
        faces.resize   (2 * n - 2 + 3);
        vertices.resize(n + 3        );
        edges.resize   (3 * n + 3    );
    }

    int n;
    TContainerFaces    faces;
    TContainerVertices vertices;
    TContainerDCEL     edges;
};

/**
 *  Progressive triangulation function of the points placed in 'ps' array
 */
std::shared_ptr<Result> triangulate(const Vector3* ps, int len);

}//JVa
