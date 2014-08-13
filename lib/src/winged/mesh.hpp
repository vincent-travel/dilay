#ifndef DILAY_WINGED_MESH
#define DILAY_WINGED_MESH

#include <vector>
#include <glm/fwd.hpp>
#include "fwd-winged.hpp"
#include "macro.hpp"

class WingedFaceIntersection;
class PrimRay;
class PrimTriangle;
class Octree;
class WingedVertex;
class WingedFace;
class WingedEdge;
class Id;
class PrimSphere;
class Selection;
class Mesh;

class WingedMesh {
  public: 
    DECLARE_BIG3 (WingedMesh)
    WingedMesh   (const Id&);

    const Id&          id                () const;
    glm::vec3          vector            (unsigned int) const;
    unsigned int       index             (unsigned int) const;
    glm::vec3          normal            (unsigned int) const;
    WingedVertex*      vertex            (unsigned int);
    WingedVertex&      lastVertex        ();
    WingedEdge*        edge              (const Id&);
    WingedFace*        face              (const Id&);
    unsigned int       addIndex          (unsigned int);
    WingedVertex&      addVertex         (const glm::vec3&);
    WingedEdge&        addEdge           (const WingedEdge&);
    WingedFace&        addFace           (const WingedFace&, const PrimTriangle&);
    void               setIndex          (unsigned int, unsigned int);
    void               setVertex         (unsigned int, const glm::vec3&);
    void               setNormal         (unsigned int, const glm::vec3&);

    const Vertices&    vertices          () const;
    const Edges&       edges             () const;
    const Octree&      octree            () const;
    const Mesh&        mesh              () const;

    void               deleteEdge        (const WingedEdge&);
    void               deleteFace        (const WingedFace&);
    void               popVertex         ();

    WingedFace&        realignFace       (const WingedFace&, const PrimTriangle&, bool* = nullptr);

    unsigned int       numVertices       () const;
    unsigned int       numEdges          () const;
    unsigned int       numFaces          () const;
    unsigned int       numIndices        () const;
    bool               isEmpty           () const;

    void               writeAllIndices   (); 
    void     writeAllInterpolatedNormals (); 
    void               bufferData        ();
    void               render            (const Selection&);
    void               reset             ();
    void               setupOctreeRoot   (const glm::vec3&, float);
    void               toggleRenderMode  ();
    
    bool               intersects        (const PrimRay&, WingedFaceIntersection&);
    bool               intersects        (const PrimSphere&, std::vector <WingedFace*>&);

    void               scale             (const glm::vec3&);
    void               scaling           (const glm::vec3&);
    glm::vec3          scaling           () const;
    void               translate         (const glm::vec3&);
    void               position          (const glm::vec3&);
    glm::vec3          position          () const;
    void               rotationMatrix    (const glm::mat4x4&);
    const glm::mat4x4& rotationMatrix    () const;
    void               rotationX         (float);
    void               rotationY         (float);
    void               rotationZ         (float);
    void               normalize         ();

    SAFE_REF1 (WingedVertex, vertex, unsigned int)
    SAFE_REF1 (WingedEdge  , edge  , const Id&)
    SAFE_REF1 (WingedFace  , face  , const Id&)
  private:
    class Impl;
    Impl* impl;
};

#endif
